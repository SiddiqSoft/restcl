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
#ifndef HTTP_FRAME_HPP
#define HTTP_FRAME_HPP

#include <cstddef>
#include <optional>
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

namespace siddiqsoft
{
    /**
     * @brief Store the Content-Type, Content-Length and the serialized content
     *
     */
    class ContentType
    {
    public:
        std::string type {};
        std::string body {};
        size_t      length {0};
        size_t      offset {0};
        size_t      remainingSize {0};

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(ContentType, type, body, length, offset, remainingSize);

        ContentType()  = default;
        ~ContentType() = default;

        ContentType(const std::shared_ptr<ContentType> src)
        {
            if (src) {
                offset        = src->offset;
                remainingSize = src->remainingSize;
                type          = src->type;
                body          = src->body;
                length        = src->length;
            }
        }

        void operator=(const std::shared_ptr<ContentType> src)
        {
            if (src) {
                offset        = src->offset;
                remainingSize = src->remainingSize;
                type          = src->type;
                body          = src->body;
                length        = src->length;
            }
        }

        void operator=(const nlohmann::json& j)
        {
            body          = j.dump();
            remainingSize = length = body.length();
            type                   = CONTENT_APPLICATION_JSON;
        }

        void operator=(const std::string& s)
        {
            body          = s;
            remainingSize = length = body.length();
            type                   = CONTENT_APPLICATION_TEXT;
        }

        void parseFromSerializedJson(const std::string& s)
        {
            try {
                auto obj      = nlohmann::json::parse(s);
                body          = obj.dump();
                remainingSize = length = body.length();
                type                   = CONTENT_APPLICATION_JSON;
                offset                 = 0;
            }
            catch (...) {
            }
        }

        operator std::string() const { return body; }

        operator bool() const { return !body.empty(); }
    };


    /// @brief A REST request utility class. Models the request a JSON document with `request`, `headers` and `content` elements.
    /// Essentially we're a convenience wrapper on the rest_request.
    class http_frame
    {
#if defined(DEBUG) || defined(_DEBUG)
    public:
#else
    protected:
#endif
        HttpProtocolVersionType        protocol {HttpProtocolVersionType::Http11};
        HttpMethodType                 method {};
        Uri<char, AuthorityHttp<char>> uri {};
        nlohmann::json                 headers {{"Date", DateUtils::RFC7231()}};
        std::shared_ptr<ContentType>   content {new ContentType()};

    protected:
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

            return HttpMethodType::METHOD_UNKNOWN;
        }

    public:
        virtual ~http_frame() = default;
        http_frame()          = default;


        auto& setProtocol(const HttpProtocolVersionType& p)
        {
            protocol = p;
            return *this;
        }

        auto& setProtocol(const std::string& fragment)
        {
            for (const auto& [i, p] : HttpProtocolVersions) {
                if (fragment.starts_with(p)) {
                    protocol = i;
                    return *this;
                }
            }
            // No match.. return exception
            throw std::invalid_argument(std::format("Unknown {}", fragment).c_str());
        }

        auto getProtocol() { return protocol; }

        auto& setMethod(const HttpMethodType& m)
        {
            method = m;
            return *this;
        }

        auto& setMethod(const std::string& fragment)
        {
            for (const auto& [i, v] : HttpVerbs) {
                if (v == fragment) {
                    method = i;
                    return *this;
                }
            }
            // No match.. return exception
            throw std::invalid_argument(std::format("Unknown {}", fragment).c_str());
        }

        [[nodiscard]] HttpMethodType getMethod() const { return method; }

        auto& setUri(const Uri<char, AuthorityHttp<char>>& u)
        {
            uri              = u;
            headers[HF_HOST] = std::format("{}:{}", uri.authority.host, uri.authority.port);

            return *this;
        }

        auto& getUri() const { return uri; }

        auto& setHeaders(const nlohmann::json& h)
        {
            headers.update(h);
            return *this;
        }

        auto& setHeader(const std::string& key, const std::string& value)
        {
            if (!value.empty())
                headers[key] = value;
            else
                headers.erase(key);

            return *this;
        }

        auto& getHeader(const std::string& key) noexcept(false) { return headers.at(key); }

        nlohmann::json& getHeaders() { return headers; }

    protected:
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
        [[nodiscard]] virtual std::string encode() const = 0;

        [[nodiscard]] std::string encodeHeaders() const
        {
            std::string hs {};
            encodeHeaders_to(hs);
            return hs;
        }


        auto getHost() const { return headers["Host"].template get<std::string>(); }


        /// @brief Set the content (non-JSON)
        /// @param ctype Content-Type
        /// @param c The content
        /// @return Self
        auto& setContent(const std::string& ctype, const std::string& c)
        {
            if (ctype.empty() && !c.empty()) throw std::invalid_argument("Content-Type cannot be empty");
            if (!ctype.empty() && c.empty())
                throw std::invalid_argument(std::format("Content-Type is {} but no content provided!", ctype).c_str());

            if (!ctype.empty() && !c.empty() && content) {
                content->body          = c;
                content->offset        = 0;
                content->type          = ctype;
                content->remainingSize = content->length = c.length();
                headers[HF_CONTENT_TYPE]                 = content->type;
                headers[HF_CONTENT_LENGTH]               = content->length;
            }

            return *this;
        }


        auto& setContent(const std::string& src)
        {
            if (content && !src.empty()) {
                content->body          = src;
                content->offset        = 0;
                content->remainingSize = content->length = src.length();

                content->type = headers.value(HF_CONTENT_TYPE, headers.value("content-type", CONTENT_APPLICATION_TEXT));

                if (!headers.contains(HF_CONTENT_LENGTH)) headers[HF_CONTENT_LENGTH] = content->length;
            }
            return *this;
        }


        auto& setContent(std::shared_ptr<ContentType> src)
        {
            try {
                content.swap(src);
            }
            catch (std::exception& e) {
                std::print(std::cerr, "{} - Exception: {}\n", __func__, e.what());
            }
            return *this;
        }


        auto& getContent() { return content; }


        [[nodiscard]] auto encodeContent() const
        {
            content->length = content->body.length();
            return content->body;
        }


        /// @brief Set the content to json
        /// @param c JSON content
        /// @return Self
        auto& setContent(const nlohmann::json& c)
        {
            if (!c.empty() && c.is_object()) {
                // This allows us to handle such things as: application/json+custom
                if (!headers.contains(HF_CONTENT_TYPE)) headers[HF_CONTENT_TYPE] = CONTENT_APPLICATION_JSON;

                return setContent(headers.value(HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON), c.dump());
            }
            return *this;
        }

        friend void to_json(nlohmann::json&, const http_frame&);
    };


    inline void to_json(nlohmann::json& dest, const http_frame& src)
    {
        dest = nlohmann::json {{"protocol", src.protocol},
                               {"method", src.method},
                               {"uri", src.uri},
                               {"headers", src.headers},
                               {"content", src.content}};
    }

} // namespace siddiqsoft
#endif // !HTTP_FRAME_HPP
