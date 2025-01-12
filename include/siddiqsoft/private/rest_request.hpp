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

#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>
#include <iostream>
#include <chrono>
#include <string>
#include <functional>
#include <memory>
#include <format>
#include <iterator>
#include <expected>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/date-utils.hpp"

#include "restcl_definitions.hpp"
#include "http_frame.hpp"


namespace siddiqsoft
{
    /// @brief A REST request utility class. Models the request a JSON document with `request`, `headers` and `content` elements.
    /// Essentially we're a convenience wrapper on the rest_request.
    class rest_request : public http_frame
    {
    public:
        rest_request() = default;

        rest_request(const HttpMethodType& v, const Uri<char, AuthorityHttp<char>> u)
        {
            setMethod(v);
            setUri(u);
        }

        rest_request(const HttpMethodType&                v,
                     const Uri<char, AuthorityHttp<char>> u,
                     const nlohmann::json&                h)
        {
            setHeaders(h);
            setMethod(v);
            setUri(u);
        }

        rest_request(const HttpMethodType&                v,
                     const Uri<char, AuthorityHttp<char>> u,
                     const nlohmann::json&                h,
                     const nlohmann::json&                c)
        {
            setHeaders(h);
            setMethod(v);
            setUri(u);
            setContent(c);
        }

        /// @brief Encode the request to a byte stream ready to transfer to the remote server.
        /// @return String
        std::string encode() const override
        {
            std::string rs;

            if (!content->type.empty() && content->body.empty())
                throw std::invalid_argument("Missing content body when content type is present!");

            // Request Line
            std::format_to(std::back_inserter(rs), "{} {} {}\r\n", method, uri.urlPart, protocol);

            // Headers..
            encodeHeaders_to(rs);

            // Finally the content..
            if (!content->body.empty() && !content->type.empty()) {
                std::format_to(std::back_inserter(rs), "{}", content->body);
            }

            return rs;
        }

        friend std::ostream& operator<<(std::ostream&, const rest_request&);
        friend void          to_json(nlohmann::json& dest, const rest_request& src);
    };


    inline std::ostream& operator<<(std::ostream& os, const rest_request& src)
    {
        os << src.encode();
        return os;
    }

    namespace restcl_literals
    {
        [[nodiscard]] static rest_request operator""_GET(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::METHOD_GET);
            rr.setUri(std::string {url, sz});
            return std::move(rr);
        }

        [[nodiscard]] static rest_request operator""_HEAD(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::METHOD_HEAD);
            rr.setUri(std::string {url, sz});
            return std::move(rr);
        }

        [[nodiscard]] static rest_request operator""_POST(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::METHOD_POST);
            rr.setUri(std::string {url, sz});
            return std::move(rr);
        }

        [[nodiscard]] static rest_request operator""_PUT(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::METHOD_PUT);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_DELETE(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::METHOD_DELETE);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_CONNECT(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::METHOD_CONNECT);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_OPTIONS(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::METHOD_OPTIONS);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_TRACE(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::METHOD_TRACE);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_PATCH(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::METHOD_PATCH);
            rr.setUri(std::string {url, sz});
            return rr;
        }
    } // namespace restcl_literals


    inline void to_json(nlohmann::json& dest, const rest_request& src)
    {
        dest = nlohmann::json {{"request", {{"method", src.method}, {"uri", src.uri}, {"protocol", src.protocol}}},
                               {"headers", src.headers},
                               {"content", src.content}};
    }

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
