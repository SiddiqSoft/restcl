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
#include <regex>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/date-utils.hpp"


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
        Http2,
        Http3,
        UNKNOWN
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(HttpProtocolVersionType,
                                 {{HttpProtocolVersionType::Http1, "HTTP/1.0"},
                                  {HttpProtocolVersionType::Http11, "HTTP/1.1"},
                                  {HttpProtocolVersionType::Http12, "HTTP/1.2"},
                                  {HttpProtocolVersionType::Http2, "HTTP/2"},
                                  {HttpProtocolVersionType::Http3, "HTTP/3"},
                                  {HttpProtocolVersionType::UNKNOWN, "UNKNOWN"}});

    static const std::map<HttpProtocolVersionType, std::string> HttpProtocolVersions {
            {HttpProtocolVersionType::Http1, "HTTP/1.0"},
            {HttpProtocolVersionType::Http11, "HTTP/1.1"},
            {HttpProtocolVersionType::Http12, "HTTP/1.2"},
            {HttpProtocolVersionType::Http2, "HTTP/2"},
            {HttpProtocolVersionType::Http3, "HTTP/3"},
    };

    enum class HttpMethodType
    {
        METHOD_GET,
        METHOD_HEAD,
        METHOD_POST,
        METHOD_PUT,
        METHOD_DELETE,
        METHOD_CONNECT,
        METHOD_OPTIONS,
        METHOD_TRACE,
        METHOD_PATCH,
        METHOD_UNKNOWN
    };
    NLOHMANN_JSON_SERIALIZE_ENUM(HttpMethodType,
                                 {{HttpMethodType::METHOD_UNKNOWN, "UNKNOWN"},
                                  {HttpMethodType::METHOD_GET, "GET"},
                                  {HttpMethodType::METHOD_HEAD, "HEAD"},
                                  {HttpMethodType::METHOD_POST, "POST"},
                                  {HttpMethodType::METHOD_PUT, "PUT"},
                                  {HttpMethodType::METHOD_DELETE, "DELETE"},
                                  {HttpMethodType::METHOD_CONNECT, "CONNECT"},
                                  {HttpMethodType::METHOD_TRACE, "TRACE"},
                                  {HttpMethodType::METHOD_PATCH, "PATCH"},
                                  {HttpMethodType::METHOD_OPTIONS, "OPTIONS"}});

    /// @brief HTTP Protocol version: Http2, Http11 and Http3
    static const std::map<HttpMethodType, std::string> HttpVerbs {{HttpMethodType::METHOD_GET, "GET"},
                                                                  {HttpMethodType::METHOD_HEAD, "HEAD"},
                                                                  {HttpMethodType::METHOD_POST, "POST"},
                                                                  {HttpMethodType::METHOD_PUT, "PUT"},
                                                                  {HttpMethodType::METHOD_DELETE, "DELETE"},
                                                                  {HttpMethodType::METHOD_CONNECT, "CONNECT"},
                                                                  {HttpMethodType::METHOD_OPTIONS, "OPTIONS"},
                                                                  {HttpMethodType::METHOD_TRACE, "TRACE"},
                                                                  {HttpMethodType::METHOD_PATCH, "PATCH"}};

    static const std::string HF_CONTENT_LENGTH {"Content-Length"};
    static const std::string HF_CONTENT_TYPE {"Content-Type"};
    static const std::string HF_DATE {"Date"};
    static const std::string HF_ACCEPT {"Accept"};
    static const std::string HF_EXPECT {"Expect"};
    static const std::string HF_HOST {"Host"};

    static const std::string CONTENT_APPLICATION_JSON {"application/json"};
    static const std::string CONTENT_JSON {"json"};
    static const std::string CONTENT_APPLICATION_TEXT {"application/text"};
    static const std::string CONTENT_TEXT_PLAIN {"text/plain"};

    static inline std::string to_string(const HttpMethodType& m)
    {
        switch (m) {
            case siddiqsoft::HttpMethodType::METHOD_GET: return "GET";
            case siddiqsoft::HttpMethodType::METHOD_HEAD: return "HEAD";
            case siddiqsoft::HttpMethodType::METHOD_POST: return "POST";
            case siddiqsoft::HttpMethodType::METHOD_PUT: return "PUT";
            case siddiqsoft::HttpMethodType::METHOD_DELETE: return "DELETE";
            case siddiqsoft::HttpMethodType::METHOD_CONNECT: return "CONNECT";
            case siddiqsoft::HttpMethodType::METHOD_OPTIONS: return "OPTIONS";
            case siddiqsoft::HttpMethodType::METHOD_TRACE: return "TRACE";
            case siddiqsoft::HttpMethodType::METHOD_PATCH: return "PATCH";
            default: return "UNKNOWN";
        }
    }

    static inline std::string to_string(const HttpProtocolVersionType& m)
    {
        switch (m) {
            case siddiqsoft::HttpProtocolVersionType::Http1: return "HTTP/1.0";
            case siddiqsoft::HttpProtocolVersionType::Http11: return "HTTP/1.1";
            case siddiqsoft::HttpProtocolVersionType::Http12: return "HTTP/1.2";
            default: return "UNKNOWN";
        }
    }


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

        friend void to_json(nlohmann::json&, const ContentType&);
    };


    /// @brief A REST request utility class. Models the request a JSON document with `request`, `headers` and `content` elements.
    /// Essentially we're a convenience wrapper on the rest_request.
    template <typename CharT = char>
    class http_frame
    {
#if defined(DEBUG) || defined(_DEBUG)
    public:
#else
    protected:
#endif
        HttpProtocolVersionType          protocol {HttpProtocolVersionType::Http11};
        HttpMethodType                   method {};
        Uri<CharT, AuthorityHttp<CharT>> uri {};
        nlohmann::json                   headers {{"Date", DateUtils::RFC7231()}};
        std::shared_ptr<ContentType>     content {new ContentType()};

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


        auto& getContent() const { return content; }

        auto& getContentBody() const { return content->body; }

        [[nodiscard]] auto getContentBodyJSON() const -> nlohmann::json
        {
            try {
                if (content->type.find("json") != std::string::npos) {
                    // Content-Type is json..
                    if (!content->body.empty()) {
                        return nlohmann::json::parse(content->body);
                    }
                }
            }
            catch (...) {
            }

            return nullptr;
        }

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

        friend void to_json(nlohmann::json&, const http_frame<>&);
    };


    inline void to_json(nlohmann::json& dest, const http_frame<>& src)
    {
        dest = nlohmann::json {{"protocol", src.protocol},
                               {"method", src.method},
                               {"uri", src.uri},
                               {"headers", src.headers},
                               {"content", src.content}};
    }

    inline void to_json(nlohmann::json& dest, const ContentType& src)
    {
        dest = nlohmann::json {{"type", src.type},
                               {"body", src.body},
                               {"length", src.length},
                               {"offset", src.offset},
                               {"remainingSize", src.remainingSize}};
    }

} // namespace siddiqsoft

template <>
struct std::formatter<siddiqsoft::HttpMethodType> : std::formatter<std::string>
{
    auto format(const siddiqsoft::HttpMethodType& m, std::format_context& ctx) const
    {
        switch (m) {
            case siddiqsoft::HttpMethodType::METHOD_GET: return std::formatter<std::string>::format("GET", ctx);
            case siddiqsoft::HttpMethodType::METHOD_HEAD: return std::formatter<std::string>::format("HEAD", ctx);
            case siddiqsoft::HttpMethodType::METHOD_POST: return std::formatter<std::string>::format("POST", ctx);
            case siddiqsoft::HttpMethodType::METHOD_PUT: return std::formatter<std::string>::format("PUT", ctx);
            case siddiqsoft::HttpMethodType::METHOD_DELETE: return std::formatter<std::string>::format("DELETE", ctx);
            case siddiqsoft::HttpMethodType::METHOD_CONNECT: return std::formatter<std::string>::format("CONNECT", ctx);
            case siddiqsoft::HttpMethodType::METHOD_OPTIONS: return std::formatter<std::string>::format("OPTIONS", ctx);
            case siddiqsoft::HttpMethodType::METHOD_TRACE: return std::formatter<std::string>::format("TRACE", ctx);
            case siddiqsoft::HttpMethodType::METHOD_PATCH: return std::formatter<std::string>::format("PATCH", ctx);
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

template <>
struct std::formatter<siddiqsoft::ContentType> : std::formatter<std::string>
{
    auto format(const siddiqsoft::ContentType& cntnt, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(),
                              "Content-Type:- type:{}\nlength:{}\noffset:{}\nremainingSize:{}\nbody:{}\n",
                              cntnt.type,
                              cntnt.length,
                              cntnt.offset,
                              cntnt.remainingSize,
                              cntnt.body);
    }
    /*
        auto format(const siddiqsoft::ContentType* cntnt, std::format_context& ctx) const
        {
            return std::formatter<std::string>::format(nlohmann::json(*cntnt).dump(), ctx);
        }

        auto format(const std::shared_ptr<siddiqsoft::ContentType> cntnt, std::format_context& ctx) const
        {
            return std::formatter<std::string>::format(nlohmann::json(*cntnt).dump(), ctx);
        }
    */
};


#endif // !HTTP_FRAME_HPP
