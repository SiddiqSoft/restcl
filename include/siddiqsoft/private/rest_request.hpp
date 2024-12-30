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

namespace siddiqsoft
{
    /// @brief A REST request utility class. Models the request a JSON document with `request`, `headers` and `content` elements.
    /// Essentially we're a convenience wrapper on the rest_request.
    class rest_request : public nlohmann::json
    {
#if defined(DEBUG) || defined(_DEBUG)
    public:
        Uri<char, AuthorityHttp<char>> uri;
#else
    protected:
        Uri<char, AuthorityHttp<char>> uri;
#endif

    public:
        rest_request()
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
        {
        }


        rest_request(const rest_request&& src)
            : nlohmann::json(std::move(src))
        {
        }


        std::string getContent() const
        {
            // Build the content to ensure we have the content-type
            if (this->at("content").is_object()) {
                return this->at("content").dump();
            }
            else if (this->at("content").is_string()) {
                return this->at("content").get<std::string>();
            }

            return {};
        }

    private:
        /// @brief Encode the headers to the given argument
        /// @param rs String where the headers is "written-to".
        void encodeHeaders_to(std::string& rs) const
        {
            for (auto& [k, v] : this->at("headers").items()) {
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
            std::string body;
            auto&       hs = this->at("headers");
            auto&       rl = this->at("request");

            // Request Line
            std::format_to(std::back_inserter(rs),
                           "{} {} {}\r\n",
                           rl["method"].get<std::string>(),
                           rl["uri"].get<std::string>(),
                           rl["protocol"].get<std::string>());

            // Build the content to ensure we have the content-type
            if (auto& cs = this->at("content"); cs) {
                if (cs.is_object()) {
                    body = cs.dump();
                }
                else if (cs.is_string()) {
                    body = cs.get<std::string>();
                }
            }

            // Headers..
            encodeHeaders_to(rs);

            // Finally the content..
            if (!body.empty()) {
                std::format_to(std::back_inserter(rs), "{}", body);
            }

            return rs;
        }


        auto& setMethod(HttpMethodType verb)
        {
            (*this)["/request/method"_json_pointer] = verb;
            return *this;
        }


        auto& setProtocol(const HttpProtocolVersionType& proto = HttpProtocolVersionType::Http12)
        {
            using namespace nlohmann::literals;

            (*this)["/request/protocol"_json_pointer] = proto;
            return *this;
        }

        auto& setUri(const Uri<char, AuthorityHttp<char>>& endpoint)
        {
            using namespace nlohmann::literals;

            uri = endpoint;
            // Build the request line data
            (*this)["/request/uri"_json_pointer] = uri.urlPart;

            // At the time of the Uri, we must ensure that the protocol and the headers are also set.
            this->setProtocol();

            // Enforce some default headers
            if (auto& hs = this->at("headers"); hs != nullptr) {
                if (!hs.contains("Date")) hs["Date"] = DateUtils::RFC7231();
                if (!hs.contains("Accept")) hs["Accept"] = "application/json";
                if (!hs.contains("Host")) hs["Host"] = std::format("{}:{}", uri.authority.host, uri.authority.port);
                if (!hs.contains(HF_CONTENT_LENGTH)) hs[HF_CONTENT_LENGTH] = 0;
            }

            return *this;
        }

        auto getHost() const { return this->at("/headers/Host"_json_pointer).template get<std::string>(); }


        /// @brief Constructor with endpoint and optional headers and json content
        /// @param endpoint Fully qualified http schema uri
        /// @param h Optional json containing the headers
        auto& setHeaders(const nlohmann::json& h) noexcept(false)
        {
            using namespace nlohmann::literals;

            // Set the headers if present (we add some defaults below if not provided or missing param)
            if (!h.empty() && h.is_object() && !h.is_null()) {
                this->at("headers") = h;
            }

            // Enforce some default headers
            if (auto& hs = this->at("headers"); hs != nullptr) {
                if (!hs.contains("Date")) hs["Date"] = DateUtils::RFC7231();
                if (!hs.contains("Accept")) hs["Accept"] = "application/json";
                if (!hs.contains("Host")) hs["Host"] = std::format("{}:{}", uri.authority.host, uri.authority.port);
                if (!hs.contains(HF_CONTENT_LENGTH)) hs[HF_CONTENT_LENGTH] = 0;
            }

            return *this;
        }

        /// @brief Set the content (non-JSON)
        /// @param ctype Content-Type
        /// @param c The content
        /// @return Self
        auto& setContent(const std::string& ctype, const std::string& c)
        {
            if (!c.empty()) {
                this->at("headers")[HF_CONTENT_TYPE]   = ctype;
                this->at("headers")[HF_CONTENT_LENGTH] = c.length();
                this->at("content")                    = c;
            }
            return *this;
        }

        /// @brief Set the content to json
        /// @param c JSON content
        /// @return Self
        auto& setContent(const nlohmann::json& c)
        {
            if (!c.is_null()) {
                this->at("content")                    = c;
                this->at("headers")[HF_CONTENT_LENGTH] = c.dump().length();
                // Make sure we do not override existing value
                if (!this->at("headers").contains(HF_CONTENT_TYPE)) this->at("headers")[HF_CONTENT_TYPE] = "application/json";
            }

            return *this;
        }

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
