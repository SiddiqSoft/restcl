/**
 * @file rest_response.hpp
 * @author Siddiq Software
 * @brief REST response model with HTTP status, headers, and content parsing.
 * @version 1.0
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 * Provides the rest_response class for handling HTTP responses with support for:
 * - HTTP status codes and reason phrases
 * - Header parsing including folded headers
 * - Automatic JSON content detection and parsing
 * - Custom content type handling
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
    /// @brief REST Response object for handling HTTP responses.
    /// 
    /// @details Models an HTTP response with status code, reason phrase, headers, and content.
    ///          Extends http_frame to provide response-specific functionality including
    ///          parsing from HTTP wire format, automatic JSON detection, and status validation.
    ///          Supports all HTTP status codes and automatic header folding.
    /// 
    /// @tparam CharT Character type (default: char)
    /// 
    /// @example
    /// @code
    /// // Parse response from wire format
    /// std::string rawResponse = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"id\":1}";
    /// auto resp = rest_response<>::parse(rawResponse);
    /// 
    /// if (resp.success()) {
    ///     std::cout << "Status: " << resp.statusCode() << std::endl;
    ///     auto json = resp.getContentBodyJSON();
    /// }
    /// @endcode
    template <typename CharT = char>
    class rest_response : public http_frame<CharT>
    {
        unsigned    _statusCode {0};
        std::string _reasonCode {};

    public:
        /// @brief Check if the response indicates success.
        /// 
        /// @details Determines if the HTTP response represents a successful operation.
        ///          Success is defined as status code in the range [100, 400).
        ///          This includes 1xx (informational), 2xx (success), and 3xx (redirection).
        /// 
        /// @return true if status code is between 100 and 399 (inclusive), false otherwise
        /// 
        /// @note Status codes 4xx (client error) and 5xx (server error) return false
        /// @note Status code 0 (no response) returns false
        /// 
        /// @example
        /// @code
        /// if (resp.success()) {
        ///     // Handle successful response
        /// } else {
        ///     // Handle error response
        /// }
        /// @endcode
        bool success() const { return (_statusCode > 99) && (_statusCode < 400); }


        /// @brief Encode the response to HTTP wire format.
        /// 
        /// @details Converts the response object into a complete HTTP message ready for transmission.
        ///          Format: STATUS_LINE\r\nHEADERS\r\n\r\nBODY
        ///          Example: HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{...}
        /// 
        /// @return String containing the complete HTTP response in wire format
        /// 
        /// @throws std::invalid_argument if Content-Type is set but body is empty
        /// 
        /// @note The status line format is: PROTOCOL STATUS_CODE REASON_PHRASE\r\n
        /// @note Headers are formatted as: KEY: VALUE\r\n
        /// @note Body is appended after the blank line separator (\r\n\r\n)
        /// 
        /// @example
        /// @code
        /// rest_response<> resp;
        /// resp.setStatus(200, "OK");
        /// resp.setContent({{"id", 1}, {"name", "John"}});
        /// std::string encoded = resp.encode();
        /// // Result: "HTTP/1.1 200 OK\r\n...\r\n\r\n{\"id\":1,\"name\":\"John\"}"
        /// @endcode
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

        /// @brief Get the HTTP status code.
        /// @return Unsigned integer representing the HTTP status code (e.g., 200, 404, 500)
        auto statusCode() const { return _statusCode; }

        /// @brief Get the HTTP reason phrase.
        /// @return String containing the reason phrase (e.g., "OK", "Not Found", "Internal Server Error")
        auto reasonCode() const { return _reasonCode; }

        /// @brief Get both status code and reason phrase.
        /// @return Pair containing (status_code, reason_phrase)
        auto status() const { return std::pair<unsigned, std::string> {_statusCode, _reasonCode}; }

    public:
        friend std::ostream& operator<<(std::ostream&, const rest_response<>&);


        /// @brief Set the HTTP status code and reason phrase.
        /// 
        /// @details Updates the response status information. This is typically called
        ///          during response parsing or when constructing a response object.
        /// 
        /// @param code HTTP status code (e.g., 200, 404, 500)
        /// @param message Reason phrase (e.g., "OK", "Not Found", "Internal Server Error")
        /// @return Reference to self for method chaining
        /// 
        /// @note Common status codes:
        ///       - 200: OK (success)
        ///       - 201: Created (resource created)
        ///       - 204: No Content (success, no body)
        ///       - 301/302: Redirect
        ///       - 400: Bad Request (client error)
        ///       - 401: Unauthorized (authentication required)
        ///       - 403: Forbidden (access denied)
        ///       - 404: Not Found (resource not found)
        ///       - 500: Internal Server Error
        ///       - 503: Service Unavailable
        /// 
        /// @example
        /// @code
        /// rest_response<> resp;
        /// resp.setStatus(200, "OK")
        ///     .setHeader("Content-Type", "application/json")
        ///     .setContent({{"success", true}});
        /// @endcode
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
#if defined(DEBUG)
                std::print(std::cerr, "{} - {} : {}..ex:{}...........\n", __func__, key, value, ex.what());
#endif
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
                // If we're already at the header end, we're done
                if (bufferStart >= headerEnd) {
                    done = true;
                    break;
                }

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
                        if (bufferStart < headerEnd && *bufferStart == ' ') bufferStart = ++hsep;

                    label_recummulate_to_unfold_buffer:
                        auto hend = useCRLF ? search(hsep, headerEnd, HTTP_NEWLINE.begin(), HTTP_NEWLINE.end())
                                            : search(hsep, headerEnd, ELEM_NEWLINE_LF.begin(), ELEM_NEWLINE_LF.end());
                        if (hend != headerEnd) {
                            // We found the `\r\n`;
                            // Next, check if this is a folded element
                            auto peekPos = hend + lineEndSize;
                            if ((peekPos + 1 <= headerEnd) &&
                                ((*(peekPos + 1) == ' ') ||
                                 ((*(peekPos + 1)) == '\t'))) // peek ahead to see if we have.. folded indicator
                            {
                                // Yes, we have a folded item.
                                // build up the value..
                                value.append(hsep, hend);
                                // Advance to past the fold portion
                                hsep = peekPos + 1;
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
struct std::formatter<siddiqsoft::rest_response<char>> : std::formatter<std::string>
{
    auto format(const siddiqsoft::rest_response<char>& sv, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};

#endif // !REST_RESPONSE_HPP
