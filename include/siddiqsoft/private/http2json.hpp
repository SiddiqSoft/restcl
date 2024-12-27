#ifndef HTTP2JSON_HPP
#define HTTP2JSON_HPP

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <regex>
#include <format>

#include "nlohmann/json.hpp"
#include "rest_request.hpp"
#include "rest_response.hpp"

#include "ctre.hpp"


namespace siddiqsoft
{
    static const std::regex                 HTTP_RESPONSE_REGEX {"(HTTP.*)\\s(\\d+)\\s([^\\s]*)\\s"};
    static const std::string                HTTP_NEWLINE {"\r\n"};
    static const std::string                HTTP_EMPTY_STRING {};
    static const std::string                HTTP_END_OF_HEADERS {"\r\n\r\n"};
    static const std::string                HTTP_PROTOCOLPREFIX {"HTTP/"};
    static const std::array<std::string, 4> HTTP_PROTOCOL {{"HTTP/1.0", "HTTP/1.1", "HTTP/2.0", "HTTP/3.0"}};
    static const std::array<std::string, 8> HTTP_VERBS {"GET", "POST", "PUT", "UPDATE", "DELETE", "INFO", "OPTIONS", "CONNECT"};


    class http2json
    {
    private:
        static auto isHttpProtocol(const std::string& fragment) -> const std::string&
        {
            for (const auto& p : HTTP_PROTOCOL) {
                return p.compare(fragment) == 0 ? p : HTTP_EMPTY_STRING;
            }

            return HTTP_EMPTY_STRING;
        }

        static auto isHttpVerb(const std::string& fragment) -> const std::string&
        {
            for (const auto& v : HTTP_VERBS) {
                return v.compare(fragment) == 0 ? v : HTTP_EMPTY_STRING;
            }

            return HTTP_EMPTY_STRING;
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
                if (const auto& verb = isHttpVerb(matchStartLine[3]); !verb.empty()) {
                    httpm["s"s] = {
                            {"type"s, "request"}, {"method"s, verb}, {"uri"s, matchStartLine[2]}, {"version"s, matchStartLine[3]}};
                }
                else if (const auto& proto = isHttpProtocol(matchStartLine[1]); !proto.empty()) {
                    httpm["s"s] = {{"type"s, "response"},
                                   {"reason"s, matchStartLine[3]},
                                   {"status"s, std::stoi(matchStartLine[2].str())},
                                   {"version"s, matchStartLine[1]}};
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

    public:
        [[nodiscard]] static auto parse(std::string& srcBuffer) -> siddiqsoft::rest_response
        {
            siddiqsoft::rest_response resp {};
            auto                      startIterator = srcBuffer.begin();

            if (parseStartLine(resp, startIterator, srcBuffer.end())) {
                // Continue to parse the headers..
            }

            return resp;
        };
    };
} // namespace siddiqsoft

#endif