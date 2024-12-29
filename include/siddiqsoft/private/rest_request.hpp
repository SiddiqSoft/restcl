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
    protected:
        /// @brief Not directly constructible; use the derived classes to build the request
        rest_request()
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
        {
        }

        /// @brief Constructs a request with endpoint string into internal Uri object
        /// @param s Valid endpoint string
        explicit rest_request(const std::string& s) noexcept(false)
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
            , uri(s) {};

        /// @brief Not directly constructible; use the derived classes to build the request
        /// @param s The source Uri
        explicit rest_request(const Uri<char>& s) noexcept(false)
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
            , uri(s) {};

    public:
        /// @brief Set the content (non-JSON)
        /// @param ctype Content-Type
        /// @param c The content
        /// @return Self
        rest_request& setContent(const std::string& ctype, const std::string& c)
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
        rest_request& setContent(const nlohmann::json& c)
        {
            if (!c.is_null()) {
                this->at("content")                    = c;
                this->at("headers")[HF_CONTENT_LENGTH] = c.dump().length();
                // Make sure we do not override existing value
                if (!this->at("headers").contains(HF_CONTENT_TYPE)) this->at("headers")[HF_CONTENT_TYPE] = "application/json";
            }
            return *this;
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
                           rl["version"].get<std::string>());

            // Build the content to ensure we have the content-type
            if (this->contains("content")) {
                if (this->at("content").is_object()) {
                    body = this->at("content").dump();
                }
                else if (this->at("content").is_string()) {
                    body = this->at("content").get<std::string>();
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


    public:
        friend std::ostream& operator<<(std::ostream&, const rest_request&);

    public:
        Uri<char, AuthorityHttp<char>> uri;

    public:
        /// @brief Constructor with only endpoint as string argument
        /// @param endpoint Valid endpoint string
        rest_request(const std::string& verb,
                     const std::string& endpoint,
                     const std::string& proto = HTTP_PROTOCOL_VERSIONS[1]) noexcept(false)
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
            , uri(endpoint)
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
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
            , uri(endpoint)
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
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
            , uri(endpoint)
        {
            // Set the headers if present (we add some defaults below if not provided or missing param)
            if (h.is_object() && !h.is_null()) {
                this->at("headers") = h;
            }

            setContent(c);
        }

        /// @brief Constructor with arbitrary content. If the header Content-Type is missing then we use text/plain
        /// @param endpoint Fully qualified http schema uri
        /// @param h Required json containing the headers. At least Content-Type should be set.
        /// @param c Required string containing the content. Make sure to set "Content-Type"
        explicit rest_request(const Uri<char>& endpoint, const nlohmann::json& h, const std::string& c) noexcept(false)
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
            , uri(endpoint)
        {
            // Set the headers if present (we add some defaults below if not provided or missing param)
            if (h.is_object() && !h.is_null()) {
                this->at("headers") = h;
            }

            if (!c.empty()) {
                if (h.is_null() || (h.is_object() && !h.contains("Content-Type")))
                    this->at("headers")["Content-Type"] = "text/plain";
                this->at("headers")[HF_CONTENT_LENGTH] = c.length();
                this->at("content")                    = c;
            }
        }

        /// @brief Constructor with arbitrary content. If the header Content-Type is missing then we use text/plain
        /// @param endpoint Fully qualified http schema uri
        /// @param h Required json containing the headers. At least Content-Type should be set.
        /// @param c Required string containing the content. Make sure to set "Content-Type"
        explicit rest_request(const Uri<char>& endpoint, const nlohmann::json& h, const char* srcContent) noexcept(false)
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
            , uri(endpoint)
        {
            // Set the headers if present (we add some defaults below if not provided or missing param)
            if (h.is_object() && !h.is_null()) {
                this->at("headers") = h;
            }

            if (srcContent != nullptr) {
                if (h.is_null() || (h.is_object() && !h.contains("Content-Type")))
                    this->at("headers")["Content-Type"] = "text/plain";
                this->at("headers")[HF_CONTENT_LENGTH] = strlen(srcContent);
                this->at("content")                    = srcContent;
            }
            else {
                this->at("headers")[HF_CONTENT_LENGTH] = 0;
            }
        }
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
