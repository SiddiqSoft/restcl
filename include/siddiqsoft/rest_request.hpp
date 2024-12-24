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
    SERVICES; LOSS OF USE, rrd, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
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
    template <RESTMethodType RM, HTTPProtocolVersion HttpVer = HTTPProtocolVersion::Http2>
    class rest_request : public basic_request
    {
    public:
        /// @brief Constructor with only endpoint as string argument
        /// @param endpoint Valid endpoint string
        rest_request(const std::string& endpoint) noexcept(false)
            : basic_request(endpoint)
        {
            // Build the request line data
            rrd["request"] = {{"uri", uri.urlPart}, {"method", RM}, {"version", HttpVer}};

            // Enforce some default headers
            auto& hs = rrd.at("headers");
            if (!hs.contains("Date")) hs["Date"] = DateUtils::RFC7231();
            if (!hs.contains("Accept")) hs["Accept"] = "application/json";
            if (!hs.contains("Host")) hs["Host"] = std::format("{}:{}", uri.authority.host, uri.authority.port);
            if (!hs.contains("Content-Length")) hs["Content-Length"] = 0;
        }


        /// @brief Constructor with endpoint and optional headers and json content
        /// @param endpoint Fully qualified http schema uri
        /// @param h Optional json containing the headers
        rest_request(const Uri<char>& endpoint, const nlohmann::json& h = nullptr) noexcept(false)
            : basic_request(endpoint)
        {
            // Set the headers if present (we add some defaults below if not provided or missing param)
            if (h.is_object() && !h.is_null()) {
                rrd["headers"] = h;
            }

            // Build the request line data
            rrd["request"] = {{"uri", uri.urlPart}, {"method", RM}, {"version", HttpVer}};

            // Enforce some default headers
            auto& hs = rrd.at("headers");
            if (!hs.contains("Date")) hs["Date"] = DateUtils::RFC7231();
            if (!hs.contains("Accept")) hs["Accept"] = "application/json";
            if (!hs.contains("Host")) hs["Host"] = std::format("{}:{}", uri.authority.host, uri.authority.port);
            if (!hs.contains("Content-Length")) hs["Content-Length"] = 0;
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
            if (h.is_null() || (h.is_object() && !h.contains("Content-Type"))) rrd["headers"]["Content-Type"] = "text/plain";
            rrd["headers"]["Content-Length"] = c.length();
            rrd["content"]                   = c;
        }

        /// @brief Constructor with arbitrary content. If the header Content-Type is missing then we use text/plain
        /// @param endpoint Fully qualified http schema uri
        /// @param h Required json containing the headers. At least Content-Type should be set.
        /// @param c Required string containing the content. Make sure to set "Content-Type"
        explicit rest_request(const Uri<char>& endpoint, const nlohmann::json& h, const char* c) noexcept(false)
            : rest_request(endpoint, h)
        {
            if (c != nullptr) {
                if (h.is_null() || (h.is_object() && !h.contains("Content-Type"))) rrd["headers"]["Content-Type"] = "text/plain";
                rrd["headers"]["Content-Length"] = strlen(c);
                rrd["content"]                   = c;
            }
            else {
                rrd["headers"]["Content-Length"] = 0;
            }
        }
    };


    using ReqPost    = rest_request<RESTMethodType::Post>;
    using ReqPut     = rest_request<RESTMethodType::Put>;
    using ReqPatch   = rest_request<RESTMethodType::Patch>;
    using ReqGet     = rest_request<RESTMethodType::Get>;
    using ReqDelete  = rest_request<RESTMethodType::Delete>;
    using ReqOptions = rest_request<RESTMethodType::Options>;
    using ReqHead    = rest_request<RESTMethodType::Head>;

#pragma region Literal Operators for rest_request
    namespace restcl_literals
    {
        [[nodiscard]] static rest_request<RESTMethodType::Get> operator"" _GET(const char* s, size_t sz)
        {
            return rest_request<RESTMethodType::Get>(SplitUri<>(std::string {s, sz}));
        }

        [[nodiscard]] static rest_request<RESTMethodType::Delete> operator"" _DELETE(const char* s, size_t sz)
        {
            return rest_request<RESTMethodType::Delete>(SplitUri<>(std::string {s, sz}));
        }


        static rest_request<RESTMethodType::Head> operator"" _HEAD(const char* s, size_t sz)
        {
            return rest_request<RESTMethodType::Head>(SplitUri<>(std::string {s, sz}));
        }


        static rest_request<RESTMethodType::Options> operator"" _OPTIONS(const char* s, size_t sz)
        {
            return rest_request<RESTMethodType::Options>(SplitUri<>(std::string {s, sz}));
        }


        static rest_request<RESTMethodType::Patch> operator"" _PATCH(const char* s, size_t sz)
        {
            return rest_request<RESTMethodType::Patch>(SplitUri<>(std::string {s, sz}));
        }


        static rest_request<RESTMethodType::Post> operator"" _POST(const char* s, size_t sz)
        {
            return rest_request<RESTMethodType::Post>(SplitUri(std::string {s, sz}));
        }


        static rest_request<RESTMethodType::Put> operator"" _PUT(const char* s, size_t sz)
        {
            return rest_request<RESTMethodType::Put>(SplitUri(std::string {s, sz}));
        }
    } // namespace restcl_literals
#pragma endregion

} // namespace siddiqsoft


template <siddiqsoft::RESTMethodType RM>
struct std::formatter<siddiqsoft::rest_request<RM>> : std::formatter<std::string>
{
    template <class FC>
    auto format(const siddiqsoft::rest_request<RM>& sv, FC& ctx) const
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};

#endif // !REST_REQUEST_HPP
