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
#ifndef RESTCL_DEFINITIONS_HPP
#define RESTCL_DEFINITIONS_HPP

#include <string>
#include <regex>
#include <array>
#include <map>

#include "nlohmann/json.hpp"

namespace siddiqsoft
{
    static const std::regex  HTTP_RESPONSE_REGEX {"(HTTP.*)\\s(\\d+)\\s([^\\r\\n]*)\\r\\n"};
    static const std::string HTTP_NEWLINE {"\r\n"};
    static const std::string ELEM_NEWLINE_LF {"\r"};
    static const std::string ELEM_SEPERATOR {": "};
    static const std::string HTTP_EMPTY_STRING {};
    static const std::string HTTP_END_OF_HEADERS {"\r\n\r\n"};
    static const std::string HTTP_PROTOCOLPREFIX {"HTTP/"};

    enum class HttpProtocolVersionType
    {
        Http1,
        Http11,
        Http12,
        // Http2,
        // Http3,
        UNKNOWN
    };
    NLOHMANN_JSON_SERIALIZE_ENUM(HttpProtocolVersionType,
                                 {{HttpProtocolVersionType::Http1, "HTTP/1.0"},
                                  {HttpProtocolVersionType::Http11, "HTTP/1.1"},
                                  {HttpProtocolVersionType::UNKNOWN, "UNKNOWN"}});
    static const std::map<HttpProtocolVersionType, std::string> HttpProtocolVersions {
            {HttpProtocolVersionType::Http1, "HTTP/1.0"},
            {HttpProtocolVersionType::Http11, "HTTP/1.1"},
    };

    enum class HttpMethodType
    {
        UNKNOWN,
        GET,
        HEAD,
        POST,
        PUT,
        DELETE,
        CONNECT,
        OPTIONS,
        TRACE,
        PATCH
    };
    NLOHMANN_JSON_SERIALIZE_ENUM(HttpMethodType,
                                 {{HttpMethodType::UNKNOWN, "UNKNOWN"},
                                  {HttpMethodType::GET, "GET"},
                                  {HttpMethodType::HEAD, "HEAD"},
                                  {HttpMethodType::POST, "POST"},
                                  {HttpMethodType::PUT, "PUT"},
                                  {HttpMethodType::DELETE, "DELETE"},
                                  {HttpMethodType::CONNECT, "CONNECT"},
                                  {HttpMethodType::TRACE, "TRACE"},
                                  {HttpMethodType::PATCH, "PATCH"},
                                  {HttpMethodType::OPTIONS, "OPTIONS"}});

    /// @brief HTTP Protocol version: Http2, Http11 and Http3
    static const std::map<HttpMethodType, std::string> HttpVerbs {{HttpMethodType::GET, "GET"},
                                                                  {HttpMethodType::HEAD, "HEAD"},
                                                                  {HttpMethodType::POST, "POST"},
                                                                  {HttpMethodType::PUT, "PUT"},
                                                                  {HttpMethodType::DELETE, "DELETE"},
                                                                  {HttpMethodType::CONNECT, "CONNECT"},
                                                                  {HttpMethodType::OPTIONS, "OPTIONS"},
                                                                  {HttpMethodType::TRACE, "TRACE"},
                                                                  {HttpMethodType::PATCH, "PATCH"}};

    static const std::string HF_CONTENT_LENGTH {"Content-Length"};
    static const std::string HF_CONTENT_TYPE {"Content-Type"};
    static const std::string HF_DATE {"Date"};
    static const std::string HF_ACCEPT {"Accept"};
    static const std::string HF_HOST {"Host"};

    static const std::string CONTENT_APPLICATION_JSON {"application/json"};
    static const std::string CONTENT_JSON {"json"};
    static const std::string CONTENT_APPLICATION_TEXT {"application/text"};
    static const std::string CONTENT_TEXT_PLAIN {"text/plain"};
} // namespace siddiqsoft

template <>
struct std::formatter<siddiqsoft::HttpMethodType> : std::formatter<std::string>
{
    auto format(const siddiqsoft::HttpMethodType& m, std::format_context& ctx) const
    {
        switch (m) {
            case siddiqsoft::HttpMethodType::GET: return std::formatter<std::string>::format("GET", ctx);
            case siddiqsoft::HttpMethodType::HEAD: return std::formatter<std::string>::format("HEAD", ctx);
            case siddiqsoft::HttpMethodType::POST: return std::formatter<std::string>::format("POST", ctx);
            case siddiqsoft::HttpMethodType::PUT: return std::formatter<std::string>::format("PUT", ctx);
            case siddiqsoft::HttpMethodType::DELETE: return std::formatter<std::string>::format("DELETE", ctx);
            case siddiqsoft::HttpMethodType::CONNECT: return std::formatter<std::string>::format("CONNECT", ctx);
            case siddiqsoft::HttpMethodType::OPTIONS: return std::formatter<std::string>::format("OPTIONS", ctx);
            case siddiqsoft::HttpMethodType::TRACE: return std::formatter<std::string>::format("TRACE", ctx);
            case siddiqsoft::HttpMethodType::PATCH: return std::formatter<std::string>::format("PATCH", ctx);
            default: return std::formatter<std::string>::format("UNKNOWN", ctx);
        }
    }
};

template <>
struct std::formatter<siddiqsoft::HttpProtocolVersionType> : std::formatter<std::string>
{
    auto format(const siddiqsoft::HttpProtocolVersionType& p, std::format_context& ctx) const
    {
        switch (p) {
            case siddiqsoft::HttpProtocolVersionType::Http1: return std::formatter<std::string>::format("HTTP/1.0", ctx);
            case siddiqsoft::HttpProtocolVersionType::Http11: return std::formatter<std::string>::format("HTTP/1.1", ctx);
            default: return std::formatter<std::string>::format("UNKNOWN", ctx);
        }
    }
};

#endif // !RESTCL_DEFINITIONS_HPP
