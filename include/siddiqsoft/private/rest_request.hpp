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
#include <utility>
#include <variant>
#ifndef REST_REQUEST_HPP
#define REST_REQUEST_HPP


#include <iostream>
#include <chrono>
#include <string>
#include <functional>
#include <memory>
#include <format>
#include <iterator>
#include <map>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/date-utils.hpp"

#include "restcl_definitions.hpp"

namespace siddiqsoft
{
    /// @brief A REST request utility class. Models the request a JSON document with `request`, `headers` and `content` elements.
    /// Essentially we're a convenience wrapper on the rest_request.
    class rest_request
    {
        /**
         * @brief Store the Content-Type, Content-Length and the serialized content
         *
         */
        struct ContentType
        {
            std::string type {};
            std::string str {};
            size_t      length {0};

            ContentType(const nlohmann::json& j)
            {
                str    = j.dump();
                length = str.length();
                type   = CONTENT_APPLICATION_JSON;
            }

            void operator=(const nlohmann::json& j)
            {
                str    = j.dump();
                length = str.length();
                type   = CONTENT_APPLICATION_JSON;
            }

            operator std::string()
            {
                if (!str.empty()) {
                    length = str.length();
                }
                return str;
            }

            operator bool() { return !str.empty(); }
        };

#if defined(DEBUG) || defined(_DEBUG)
    public:
#else
    protected:
#endif
        HttpProtocolVersionType        protocol {};
        HttpMethodType                 method {};
        Uri<char, AuthorityHttp<char>> uri;
        nlohmann::json                 headers {};
        ContentType                    content {};

    public:
        rest_request() { headers["Date"] = DateUtils::RFC7231(); }

        auto& setProtocol(const HttpProtocolVersionType& p)
        {
            protocol = p;
            return *this;
        }

        auto getProtocol() { return protocol; }

        auto& setMethod(HttpMethodType& m)
        {
            method = m;
            return *this;
        }

        auto getMethod() { return method; }

        auto& setUri(const Uri<char, AuthorityHttp<char>>& u)
        {
            uri = u;
            return *this;
        }

        auto getUri() { return uri; }

        auto& setHeaders(const nlohmann::json& h)
        {
            headers = h;
            return *this;
        }

    private:
        /// @brief Encode the headers to the given argument
        /// @param rs String where the headers is "written-to".
        void encodeHeaders_to(std::string& rs) const
        {
            for (auto& [k, v] : headers.items()) {
                if (v.is_string()) {
                    std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.get<std::string>());
                }
                else if (v.is_number_unsigned()) {
                    std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.get<uint64_t>());
                }
                else if (v.is_number_integer()) {
                    std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.get<int>());
                }
                else {
                    std::format_to(std::back_inserter(rs), "{}: {}\r\n", k, v.dump());
                }
            }

            std::format_to(std::back_inserter(rs), "\r\n");
        }

    public:
        /// @brief Encode the request to a byte stream ready to transfer to the remote server.
        /// @return String
        std::string encode() const
        {
            std::string rs;


            // Request Line
            std::format_to(std::back_inserter(rs), "{} {} {}\r\n", method, uri, protocol);

            if (content) {
                headers[HF_CONTENT_LENGTH] = content.length;
                headers[HF_CONTENT_TYPE]   = content.type;
            }

            // Headers..
            encodeHeaders_to(rs);

            // Finally the content..
            if (content) {
                std::format_to(std::back_inserter(rs), "{}", content.str);
            }

            return rs;
        }

        auto getHost() const { return this->at("/headers/Host"_json_pointer).template get<std::string>(); }


        /// @brief Set the content (non-JSON)
        /// @param ctype Content-Type
        /// @param c The content
        /// @return Self
        auto& setContent(const std::string& ctype, const std::string& c)
        {
            if (ctype.empty() && !c.empty()) throw std::invalid_argument("Content-Type cannot be empty");
            if (!ctype.empty() && c.empty())
                throw std::invalid_argument(std::format("Content-Type is {} but no content provided!", ctype).c_str());

            if (!ctype.empty() && !c.empty()) {
                headers["Content-Type"]   = ctype;
                headers["Content-Length"] = c.length();
                con
            }

            return *this;
        }


        /// @brief Set the content to json
        /// @param c JSON content
        /// @return Self
        auto& setContent(const nlohmann::json& c) { return setContent(CONTENT_APPLICATION_JSON, c.dump()); }


        friend std::ostream& operator<<(std::ostream&, const rest_request&);
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
            rr.setMethod(HttpMethodType::GET);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_HEAD(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::HEAD);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_POST(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::POST);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_PUT(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::PUT);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_DELETE(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::DELETE);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_CONNECT(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::CONNECT);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_OPTIONS(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::OPTIONS);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_TRACE(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::TRACE);
            rr.setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request operator""_PATCH(const char* url, size_t sz)
        {
            rest_request rr;
            rr.setMethod(HttpMethodType::PATCH);
            rr.setUri(std::string {url, sz});
            return rr;
        }
    } // namespace restcl_literals


    static rest_request
    make_rest_request(HttpMethodType verb = HttpMethodType::GET, const std::string url = {}, const nlohmann::json& h = {})
    {
        rest_request req {};

        req.setMethod(verb);
        req.setUri(Uri<char, AuthorityHttp<char>>(url));
        req.setHeaders(h);

        return req;
    }

    static void to_json(nlohmann::json& dest, const rest_request& src)
    {
        std::cerr << "to_json for rest_request\n";
        dest = src;
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
