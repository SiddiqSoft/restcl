#ifndef HTTP2JSON_HPP
#define HTTP2JSON_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <regex>
#include <format>

#include "nlohmann/json.hpp"

#include "restcl_definitions.hpp"
#include "rest_request.hpp"
#include "rest_response.hpp"

namespace siddiqsoft
{
    class http2json
    {
    private:
        static auto isHttpProtocol(const std::string& fragment)
        {
            for (const auto& [i, p] : HttpProtocolVersions) {
                if (fragment.starts_with(p)) return i;
            }

            return HttpProtocolVersionType::UNKNOWN;
        }

        static auto isHttpVerb(const std::string& fragment)
        {
            for (const auto& [i, v] : HttpVerbs) {
                if (v == fragment) return i;
            }

            return HttpMethodType::UNKNOWN;
        }

        static bool parseStartLine(rest_response&               httpm,
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
                if (isHttpVerb(matchStartLine[3]) != HttpMethodType::UNKNOWN) {
                    httpm.setMethod(matchStartLine[3].str()).setUri(matchStartLine[2].str()).setProtocol(matchStartLine[3].str());
                }
                else if (isHttpProtocol(matchStartLine[1]) != HttpProtocolVersionType::UNKNOWN) {
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

        static bool storeHeaderValue(rest_response& httpm, const std::string& key, const std::string& value)
        {
            if (key.find(HF_CONTENT_LENGTH) == 0) {
                httpm.headers[key] = std::stoi(value);
            }
            else if (value.empty()) {
                httpm.headers[key] = "";
            }
            else {
                httpm.headers[key] = value;
            }

            return true;
        }

        static bool parseHeaders(rest_response&               httpm,
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
        [[nodiscard]] static auto parse(std::string& srcBuffer) -> siddiqsoft::rest_response
        {
            siddiqsoft::rest_response resp {};
            auto                      startIterator = srcBuffer.begin();

            if (parseStartLine(resp, startIterator, srcBuffer.end())) {
                // Continue to parse the headers..
                if (parseHeaders(resp, startIterator, srcBuffer.end())) {
                    // Continue to extract the body.. so far
                    // bufferStart will be left at the end of the header section
                    // so we can use this as our starting point for the body..
                    srcBuffer.erase(srcBuffer.begin(), startIterator);
                    resp.setContent(srcBuffer);
                }
            }

            return resp;
        };
    };
} // namespace siddiqsoft

#endif