/*
    restcl : Focussed REST Client for Modern C++

    BSD 3-Clause License

    Copyright (c) 2021, Siddiq Software
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its
       contributors may be used to endorse or promote products derived from
       this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, rrd, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <exception>
#ifndef REST_RESPONSE_HPP
#define REST_RESPONSE_HPP


#include <iostream>
#include <string>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/date-utils.hpp"

#include "http_frame.hpp"

namespace siddiqsoft
{
    /// @brief REST Response object
    template <typename CharT = char>
    class rest_response : public http_frame<CharT>
    {
        unsigned    _statusCode {0};
        std::string _reasonCode {};

    public:
        /// @brief Check if the response was successful
        /// @return True iff there is no IOError and the StatusCode (99,400)
        bool success() const { return (_statusCode > 99) && (_statusCode < 400); }


        /// @brief Encode the request to a byte stream ready to transfer to the remote server.
        /// @return String
        std::string encode() const override
        {
            std::string rs;

            if (!this->content->type.empty() && this->content->body.empty())
                throw std::invalid_argument("Missing content body when content type is present!");

            // Request Line
            std::format_to(std::back_inserter(rs), "{} {} {}\r\n", this->protocol, this->_statusCode, this->_reasonCode);

            // Headers..
            this->encodeHeaders_to(rs);

            // Finally the content->.
            if (!this->content->body.empty() && !this->content->type.empty()) {
                std::format_to(std::back_inserter(rs), "{}", this->content->body);
            }

            return rs;
        }

        auto statusCode() const { return _statusCode; }
        auto reasonCode() const { return _reasonCode; }

        auto status() const { return std::pair<unsigned, std::string> {_statusCode, _reasonCode}; }

    public:
        friend std::ostream& operator<<(std::ostream&, const rest_response<>&);


        /// @brief Construct a response object based on the given transport error
        /// @param err Specifies the transport error.
        rest_response<>& setStatus(const int code, const std::string& message)
        {
            _statusCode = code;
            _reasonCode = message;
            return *this;
        }


    protected:
        static bool parseStartLine(rest_response<CharT>&        httpm,
                                   std::string::iterator&       bufferStart,
                                   const std::string::iterator& bufferEnd) noexcept(false)
        {
            using namespace std;
            auto useCRLF              = true;
            auto lineEndDelimiterSize = HTTP_NEWLINE.size();

            match_results<string::iterator> matchStartLine;

            auto found = regex_search(bufferStart, bufferEnd, matchStartLine, HTTP_RESPONSE_REGEX);
            // Did we find a message..?
            if (found && (matchStartLine.size() >= 3)) {
                // The regex is very precise and there is no chance we will end up here
                // with an ill-formed (or unsupported) start-line.
                if (http_frame<CharT>::isHttpVerb(matchStartLine[3]) != HttpMethodType::METHOD_UNKNOWN) {
                    httpm.setMethod(matchStartLine[3].str()).setUri(matchStartLine[2].str()).setProtocol(matchStartLine[3].str());
                }
                else if (http_frame<CharT>::isHttpProtocol(matchStartLine[1]) != HttpProtocolVersionType::UNKNOWN) {
                    httpm.setStatus(std::stoi(matchStartLine[2].str()), matchStartLine[3].str()).setProtocol(matchStartLine[1]);
                }

                // Offset the start to the point after the start-line. Make sure to skip over any prefix!
                // We may have junk or left-over crud at the start (especially if we're using text files)
                bufferStart += (matchStartLine.length() + matchStartLine.prefix().length());
            }
            else {
                throw std::invalid_argument {std::format("{} - HTTP Startline not found.", __func__)};
            }

            return found;
        }

    protected:
        static bool storeHeaderValue(rest_response<>& httpm, const std::string& key, const std::string& value)
        {
            try {
                if (key.find(HF_CONTENT_LENGTH) == 0) {
                    httpm.headers[key] = std::stoi(value);
                }
                else if (value.empty()) {
                    httpm.headers[key] = "";
                }
                else {
                    httpm.headers[key] = value;
                }
            }
            catch (std::exception& ex) {
                std::print(std::cerr, "{} - {} : {}..ex:{}...........\n", __func__, key, value, ex.what());
                throw;
            }

            return true;
        }

    protected:
        static bool parseHeaders(rest_response<>&             httpm,
                                 std::string::iterator&       bufferStart,
                                 const std::string::iterator& bufferEnd) noexcept(false)
        {
            using namespace std::string_literals;

            bool done {false};
            bool found {false};

            // WARNING
            // The bufferStart must point to the start of the first sequence (excluding the CRLF) after the startline is processed!
            // Scan for the location of the header section end within the frame.
            // If we don't have one, then we should bail out.
            // Note that for response messages, it is likely that the bufferEnd will also be the headerEnd (no content).
            auto useCRLF             = true;
            auto headerDelimiterSize = HTTP_END_OF_HEADERS.size();
            auto lineEndSize         = HTTP_NEWLINE.size();
            auto headerEnd           = std::search(bufferStart, bufferEnd, HTTP_END_OF_HEADERS.begin(), HTTP_END_OF_HEADERS.end());

            // Assert header end delimiter must exist!
            auto headerSectionSize = size_t(bufferEnd - headerEnd);
            if (headerSectionSize < headerDelimiterSize)
                throw std::invalid_argument {std::format("{}:Cannot find header section delimiter.", __func__).c_str()};

            while (!done) {
                // Scan for the first `:`
                auto hsep = std::search(bufferStart, headerEnd, ELEM_SEPERATOR.begin(), ELEM_SEPERATOR.end());
                if (hsep != headerEnd) {
                    // Found the separator element.
                    // Key is from bufferStart until the separator
                    if (std::string key(bufferStart, hsep); !key.empty()) {
                        std::string value {};
                        auto        hval = hsep; // Store the location of the value part of the header element.

                        // Next, let's look for the end of element
                        bufferStart = hsep += ELEM_SEPERATOR.size();

                        // Skip over the leading "space" if found.
                        if (*bufferStart == ' ') bufferStart = ++hsep;

                    label_recummulate_to_unfold_buffer:
                        auto hend = useCRLF ? search(hsep, headerEnd, HTTP_NEWLINE.begin(), HTTP_NEWLINE.end())
                                            : search(hsep, headerEnd, ELEM_NEWLINE_LF.begin(), ELEM_NEWLINE_LF.end());
                        if (hend != headerEnd) {
                            // We found the `\r\n`;
                            // Next, check if this is a folded element
                            if ((headerEnd != (hend + lineEndSize + 1)) &&
                                ((*(hend + lineEndSize + 1) == ' ') ||
                                 ((*hend + lineEndSize + 1) == '\t'))) // peek ahead to see if we have.. folded indicator
                            {
                                // Yes, we have a folded item.
                                // build up the value..
                                value.append(hsep, hend);
                                // Advance to past the fold portion
                                hsep = hend + lineEndSize + 1;
                                // Go back to find the next section..
                                goto label_recummulate_to_unfold_buffer;
                            }
                            else {
                                value.append(hsep, hend);
                                found       = storeHeaderValue(httpm, key, value);
                                bufferStart = hend += lineEndSize;
                            }
                        }
                        else {
                            // reached the end; We're done
                            value.append(hsep, hend);
                            found       = storeHeaderValue(httpm, key, value);
                            bufferStart = headerEnd + headerDelimiterSize;
                            done        = true;
                        }
                    }
                    else {
                        // Key is empty; we're done.
                        done = true;
                    }
                }
                else {
                    // End of buffer or Could not find separator; we're done.
                    done = true;
                }
            }

            return found;
        }

    public:
        [[nodiscard]] static auto parse(std::string& srcBuffer) -> siddiqsoft::rest_response<char>
        {
            siddiqsoft::rest_response<char> resp {};
            auto                            startIterator = srcBuffer.begin();
            auto                            lastLine      = __LINE__;

            try {
                lastLine = __LINE__;
                if (parseStartLine(resp, startIterator, srcBuffer.end())) {
                    lastLine = __LINE__;
                    // Continue to parse the headers..
                    if (parseHeaders(resp, startIterator, srcBuffer.end())) {
                        lastLine = __LINE__;
                        // Continue to extract the body.. so far
                        // bufferStart will be left at the end of the header section
                        // so we can use this as our starting point for the body..
                        srcBuffer.erase(srcBuffer.begin(), startIterator);

                        if (!srcBuffer.empty()) {
                            // What type of content do we have?
                            lastLine = __LINE__;
                            if (resp.getHeaders().value(HF_CONTENT_TYPE, "").starts_with(CONTENT_APPLICATION_JSON)) {
                                lastLine = __LINE__;
                                auto doc = nlohmann::json::parse(srcBuffer);
                                lastLine = __LINE__;
                                resp.setContent(doc);
                            }
                            else {
                                lastLine = __LINE__;
                                resp.setContent(resp.getHeaders().value("Content-Type", ""), srcBuffer);
                            }
                        }
                    }
                }
            }
            catch (std::exception& ex) {
                std::print(std::cerr, "parse - while processing frame (ll:{})\n{}\n", lastLine, srcBuffer);
                throw;
            }

            return resp;
        };

        friend void to_json(nlohmann::json&, const rest_response<>&);
    };

    inline void to_json(nlohmann::json& dest, const rest_response<>& src)
    {
        dest = nlohmann::json {
                {"response", {{"statusCode", src._statusCode}, {"statusMessage", src._reasonCode}, {"protocol", src.protocol}}},
                {"headers", src.headers},
                {"content", src.content}};
    }


    /// @brief Serializer to ostream for RESResponseType
    inline std::ostream& operator<<(std::ostream& os, const rest_response<>& src)
    {
        os << src.encode();
        return os;
    }
} // namespace siddiqsoft

template <>
struct std::formatter<siddiqsoft::rest_response<>> : std::formatter<std::string>
{
    auto format(const siddiqsoft::rest_response<>& sv, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};

#endif // !REST_RESPONSE_HPP
