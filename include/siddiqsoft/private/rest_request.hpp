/*
    restcl : Focussed REST Client for Modern C++

    BSD 3-Clause License

    Copyright (c) 2021, Siddiq Software LLC
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
    SERVICES; LOSS OF USE, *this, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once
#ifndef REST_REQUEST_HPP
#define REST_REQUEST_HPP


#include <iostream>
#include <chrono>
#include <string>
#include <functional>
#include <memory>
#include <format>
#include <iterator>


#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/date-utils.hpp"

#include "restcl_definitions.hpp"
#include "basic_request.hpp"

namespace siddiqsoft
{
    /// @brief A REST request utility class. Models the request a JSON document with `request`, `headers` and `content` elements.
    class rest_request : public basic_request
    {
    public:
        /// @brief Constructor with only endpoint as string argument
        /// @param endpoint Valid endpoint string
        rest_request(const std::string& verb,
                     const std::string& endpoint,
                     const std::string& proto = HTTP_PROTOCOL_VERSIONS[1]) noexcept(false)
            : basic_request(endpoint)
        {
            // Build the request line data
            this->at("request") = {{"uri", uri.urlPart}, {"method", verb}, {"protocol", proto}};

            // Enforce some default headers
            auto& hs = this->at("headers");
            if (!hs.contains("Date")) hs["Date"] = DateUtils::RFC7231();
            if (!hs.contains("Accept")) hs["Accept"] = "application/json";
            if (!hs.contains("Host")) hs["Host"] = std::format("{}:{}", uri.authority.host, uri.authority.port);
            if (!hs.contains("Content-Length")) hs["Content-Length"] = 0;
        }


        /// @brief Constructor with endpoint and optional headers and json content
        /// @param endpoint Fully qualified http schema uri
        /// @param h Optional json containing the headers
        rest_request(const std::string& verb, const Uri<char>& endpoint, const nlohmann::json& h = nullptr) noexcept(false)
            : basic_request(endpoint)
        {
            // Set the headers if present (we add some defaults below if not provided or missing param)
            if (h.is_object() && !h.is_null()) {
                this->at("headers") = h;
            }

            // Build the request line data
            this->at("request") = {
                    {"uri", uri.urlPart}, {"method", verb}, {"version", HTTP_PROTOCOL_VERSIONS[HTTP_PROTOCOL_VERSION_ID::Http12]}};

            // Enforce some default headers
            auto& hs = this->at("headers");
            if (!hs.contains("Date")) hs["Date"] = DateUtils::RFC7231();
            if (!hs.contains("Accept")) hs["Accept"] = "application/json";
            if (!hs.contains("Host")) hs["Host"] = std::format("{}:{}", uri.authority.host, uri.authority.port);
            if (!hs.contains(HF_CONTENT_LENGTH)) hs[HF_CONTENT_LENGTH] = 0;
        }

        /// @brief Constructor with json content
        /// @param endpoint Fully qualified http schema uri
        /// @param h Required json containing the headers
        /// @param c Required json containing the content
        rest_request(const Uri<char>& endpoint, const nlohmann::json& h, const nlohmann::json& c) noexcept(false)
            : rest_request(endpoint, h)
        {
            setContent(c);
        }

        /// @brief Constructor with arbitrary content. If the header Content-Type is missing then we use text/plain
        /// @param endpoint Fully qualified http schema uri
        /// @param h Required json containing the headers. At least Content-Type should be set.
        /// @param c Required string containing the content. Make sure to set "Content-Type"
        explicit rest_request(const Uri<char>& endpoint, const nlohmann::json& h, const std::string& c) noexcept(false)
            : rest_request(endpoint, h)
        {
            if (h.is_null() || (h.is_object() && !h.contains("Content-Type"))) this->at("headers")["Content-Type"] = "text/plain";
            this->at("headers")[HF_CONTENT_LENGTH] = c.length();
            this->at("content")                    = c;
        }

        /// @brief Constructor with arbitrary content. If the header Content-Type is missing then we use text/plain
        /// @param endpoint Fully qualified http schema uri
        /// @param h Required json containing the headers. At least Content-Type should be set.
        /// @param c Required string containing the content. Make sure to set "Content-Type"
        explicit rest_request(const Uri<char>& endpoint, const nlohmann::json& h, const char* c) noexcept(false)
            : rest_request(endpoint, h)
        {
            if (c != nullptr) {
                if (h.is_null() || (h.is_object() && !h.contains("Content-Type")))
                    this->at("headers")["Content-Type"] = "text/plain";
                this->at("headers")[HF_CONTENT_LENGTH] = strlen(c);
                this->at("content")                    = c;
            }
            else {
                this->at("headers")[HF_CONTENT_LENGTH] = 0;
            }
        }
    };


    namespace restcl_literals
    {
        [[nodiscard]] static rest_request operator""_GET(const char* url, size_t sz)
        {
            return rest_request("GET", std::string {url, sz});
        }
        [[nodiscard]] static rest_request operator""_HEAD(const char* url, size_t sz)
        {
            return rest_request("HEAD", std::string {url, sz});
        }
        [[nodiscard]] static rest_request operator""_POST(const char* url, size_t sz)
        {
            return rest_request("POST", std::string {url, sz});
        }
        [[nodiscard]] static rest_request operator""_PUT(const char* url, size_t sz)
        {
            return rest_request("PUT", std::string {url, sz});
        }
        [[nodiscard]] static rest_request operator""_DELETE(const char* url, size_t sz)
        {
            return rest_request("DELETE", std::string {url, sz});
        }
        [[nodiscard]] static rest_request operator""_CONNECT(const char* url, size_t sz)
        {
            return rest_request("CONNECT", std::string {url, sz});
        }
        [[nodiscard]] static rest_request operator""_OPTIONS(const char* url, size_t sz)
        {
            return rest_request("OPTIONS", std::string {url, sz});
        }
        [[nodiscard]] static rest_request operator""_TRACE(const char* url, size_t sz)
        {
            return rest_request("TRACE", std::string {url, sz});
        }
        [[nodiscard]] static rest_request operator""_PATCH(const char* url, size_t sz)
        {
            return rest_request("PATCH", std::string {url, sz});
        }
    } // namespace restcl_literals
} // namespace siddiqsoft

template <>
struct std::formatter<siddiqsoft::rest_request> : std::formatter<std::string>
{
    template <class FC>
    auto format(const siddiqsoft::rest_request& sv, FC& ctx) const
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};

#endif // !REST_REQUEST_HPP
