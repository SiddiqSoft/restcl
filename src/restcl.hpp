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
#ifndef RESTCL_HPP
#define RESTCL_HPP


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


#if __cpp_lib_format

namespace siddiqsoft
{
    /// @brief REST Methods: GET, PATCH, POST, PUT, DELETE, HEAD, OPTIONS
    enum class RESTMethodType
    {
        Get,
        Patch,
        Post,
        Put,
        Delete,
        Head,
        Options
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(RESTMethodType,
                                 {{RESTMethodType::Get, "GET"},
                                  {RESTMethodType::Patch, "PATCH"},
                                  {RESTMethodType::Put, "PUT"},
                                  {RESTMethodType::Post, "POST"},
                                  {RESTMethodType::Delete, "DELETE"},
                                  {RESTMethodType::Head, "HEAD"},
                                  {RESTMethodType::Options, "OPTIONS"}});

    /// @brief HTTP Protocol version: Http2, Http11 and Http10
    enum class HTTPProtocolVersion
    {
        Http2,
        Http11,
        Http10
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(HTTPProtocolVersion,
                                 {{HTTPProtocolVersion::Http2, "HTTP/2"},
                                  {HTTPProtocolVersion::Http11, "HTTP/1.1"},
                                  {HTTPProtocolVersion::Http10, "HTTP/1.0"}});


    /// @brief Base for all RESTRequests
    class basic_request
    {
    protected:
        /// @brief Not directly constructible; use the derived classes to build the request
        basic_request() { }

        /// @brief Constructs a request with endpoint string into internal Uri object
        /// @param s Valid endpoint string
        explicit basic_request(const std::string& s) noexcept(false)
            : uri(s) {};

        /// @brief Not directly constructible; use the derived classes to build the request
        /// @param s The source Uri
        explicit basic_request(const Uri<char>& s) noexcept(false)
            : uri(s) {};

    public:
        /// @brief Access the "headers", "request", "content" in the json object
        /// @param key Allows access into the json object via string or json_pointer
        /// @return Non-mutable reference to the specified element.
        const auto& operator[](const auto& key) const
        {
            return rrd.at(key);
        }

        /// @brief Access the "headers", "request", "content" in the json object
        /// @param key Allows access into the json object via string or json_pointer
        /// @return Mutable reference to the specified element.
        auto& operator[](const auto& key)
        {
            return rrd.at(key);
        }


        /// @brief Set the content (non-JSON)
        /// @param ctype Content-Type
        /// @param c The content
        /// @return Self
        basic_request& setContent(const std::string& ctype, const std::string& c)
        {
            if (!c.empty()) {
                rrd["headers"]["Content-Type"]   = ctype;
                rrd["headers"]["Content-Length"] = c.length();
                rrd["content"]                   = c;
            }
            return *this;
        }

        /// @brief Set the content to json
        /// @param c JSON content
        /// @return Self
        basic_request& setContent(const nlohmann::json& c)
        {
            if (!c.is_null()) {
                rrd["content"]                   = c;
                rrd["headers"]["Content-Length"] = c.dump().length();
                // Make sure we do not override existing value
                if (!rrd["headers"].contains("Content-Type")) rrd["headers"]["Content-Type"] = "application/json";
            }
            return *this;
        }


        std::string getContent() const
        {
            // Build the content to ensure we have the content-type
            if (rrd["content"].is_object()) {
                return rrd["content"].dump();
            }
            else if (rrd["content"].is_string()) {
                return rrd["content"].get<std::string>();
            }

            return {};
        }


        /// @brief Encode the headers to the given argument
        /// @param rs String where the headers is "written-to".
        void encodeHeaders_to(std::string& rs) const
        {
            for (auto& [k, v] : rrd["headers"].items()) {
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
            auto&       hs = rrd.at("headers");
            auto&       rl = rrd.at("request");

            // Request Line
            std::format_to(std::back_inserter(rs),
                           "{} {} {}\r\n",
                           rl["method"].get<std::string>(),
                           rl["uri"].get<std::string>(),
                           rl["version"].get<std::string>());

            // Build the content to ensure we have the content-type
            if (rrd["content"].is_object()) {
                body = rrd["content"].dump();
            }
            else if (rrd["content"].is_string()) {
                body = rrd["content"].get<std::string>();
            }

            // Headers..
            encodeHeaders_to(rs);

            // Finally the content..
            if (!body.empty()) {
                std::format_to(std::back_inserter(rs), "{}", body);
            }

            return std::move(rs);
        }


    public:
        friend std::ostream& operator<<(std::ostream&, const basic_request&);
        friend void          to_json(nlohmann::json&, const basic_request&);

    public:
        Uri<char, AuthorityHttp<char>> uri;

    protected:
        nlohmann::json rrd {{"request", {{"method", nullptr}, {"uri", nullptr}, {"version", nullptr}}},
                            {"headers", nullptr},
                            {"content", nullptr}};
    };

    /// @brief Explicit implementation is required due to the restriction on direct instantiation of the basic_request class.
    /// @param dest The destination json
    /// @param src The basic_request class (or derived)
    static void to_json(nlohmann::json& dest, const basic_request& src)
    {
        dest["uri"] = src.uri;
        dest["rrd"] = src.rrd;
    }


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


    static std::ostream& operator<<(std::ostream& os, const basic_request& src)
    {
        std::format_to(std::ostream_iterator<char>(os), "{}", src);
        return os;
    }


    /// @brief REST Response object
    class basic_response
    {
    protected:
        basic_response() { }

    public:
        /// @brief Represents the IO error and its corresponding message
        struct response_code
        {
            uint32_t    code {0};
            std::string message {};
            NLOHMANN_DEFINE_TYPE_INTRUSIVE(response_code, code, message);
        };

        basic_response(const basic_response& src) noexcept
        {
            try {
                rrd = src.rrd;
            }
            catch (const std::exception&) {
            }
        }


        /// @brief Move constructor
        basic_response(basic_response&& src) noexcept
        {
            try {
                std::swap(rrd, src.rrd);
            }
            catch (const std::exception&) {
            }
        }


        /// @brief Move assignment operator
        /// @param src The source object
        /// @return Self
        basic_response& operator=(basic_response&& src) noexcept
        {
            try {
                std::swap(rrd, src.rrd);
            }
            catch (const std::exception&) {
            }

            return *this;
        }


        basic_response& operator=(const basic_response& src) noexcept
        {
            try {
                rrd = src.rrd;
            }
            catch (const std::exception&) {
            }

            return *this;
        }


        /// @brief Set the content of the response. An attempt is made to parse to json object
        /// @param c Content from the receive
        /// @return Self
        basic_response& setContent(const std::string& c)
        {
            if (!c.empty()) {
                if (rrd["headers"].value("Content-Type", "").find("json") != std::string::npos) {
                    try {
                        rrd["content"] = nlohmann::json::parse(c);
                        if (!rrd["headers"].contains("Content-Length")) rrd["headers"]["Content-Length"] = c.length();
                        return *this;
                    }
                    catch (...) {
                    }
                }

                // We did not decode a json; assign as-is
                rrd["content"] = c;
                if (!rrd["headers"].contains("Content-Length")) rrd["headers"]["Content-Length"] = c.length();
            }

            return *this;
        }


        /// @brief Check if the response was successful
        /// @return True iff there is no IOError and the StatusCode (99,400)
        bool success() const
        {
            auto sc = status().code;
            return (sc > 99) && (sc < 400);
        }


        /// @brief Access the "headers", "request", "content" in the json object
        /// @param key Allows access into the json object via string or json_pointer
        /// @return Non-mutable reference to the specified element.
        const auto& operator[](const auto& key) const
        {
            return rrd.at(key);
        }


        /// @brief Mutable access to the underlying json object
        /// @param key Allows access into the json object via string or json_pointer
        /// @return Mutable reference to the specified element.
        auto& operator[](const auto& key)
        {
            return rrd.at(key);
        }


        /// @brief Encode the request to a byte stream ready to transfer to the remote server.
        /// @return String
        std::string encode() const
        {
            std::string rs;
            std::string body;
            auto&       hs = rrd.at("headers");
            auto&       rl = rrd.at("response");

            // Request Line
            std::format_to(std::back_inserter(rs),
                           "{} {} {}\r\n",
                           rl["version"].get<std::string>(),
                           rl["status"].get<uint32_t>(),
                           rl["reason"].is_null() ? "" : rl["reason"].get<std::string>());

            // Build the content to ensure we have the content-type
            if (!rrd["content"].is_null() && rrd["content"].is_object()) {
                body = rrd["content"].dump();
            }
            else if (!rrd["content"].is_null() && rrd["content"].is_string()) {
                body = rrd["content"].get<std::string>();
            }

            // Headers..
            for (auto& [k, v] : rrd["headers"].items()) {
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

            // Finally the content..
            if (!body.empty()) {
                std::format_to(std::back_inserter(rs), "{}", body);
            }

            return std::move(rs);
        }


        /// @brief Returns the IO error if present otherwise returns the HTTP status code and reason
        /// @return response_code with the code, message which correspond with valid HTTP respose status and reason phrase or
        /// ioError and ioError message.
        /// The response contains:
        /// resp["response"]["status"] or WinHTTP error code
        /// resp["response"]["reason"] or WinHTTP error code message string
        response_code status() const
        {
            return {rrd["response"].value<unsigned>("status", 0), rrd["response"].value("reason", "")};
        }

    public:
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(basic_response, rrd);
        friend std::ostream& operator<<(std::ostream&, const basic_response&);

    protected:
        nlohmann::json rrd {{"response", {{"version", HTTPProtocolVersion::Http2}, {"status", 0}, {"reason", ""}}},
                            {"headers", nullptr},
                            {"content", nullptr}};
    };


    /// @brief REST Response object
    class rest_response : public basic_response
    {
    public:
        rest_response() { }


        rest_response(const int code, const std::string& message)
        {
            setStatus(code, message);
        }


        /// @brief Construct a response object based on the given transport error
        /// @param err Specifies the transport error.
        rest_response& setStatus(const int code, const std::string& message)
        {
            rrd["response"]["status"] = code;
            rrd["response"]["reason"] = message;
            return *this;
        }
    };


    /// @brief The function or lambda must accept const basic_request& and const basic_response&
    using basic_callbacktype = std::function<void(const basic_request&, const basic_response&)>;


    /// @brief Base class for the rest client
    class basic_restclient
    {
    public:
        std::string  UserAgent {"siddiqsoft.restcl/0.10.8"};
        std::wstring UserAgentW {L"siddiqsoft.restcl/0.10.8"};

    public:
        /// @brief Synchronous implementation of the IO
        /// @param req Request
        /// @return The response
        virtual basic_response send(basic_request&) = 0;

        /// @brief Support for async callback
        /// @param req Request
        /// @param callback function that accepts const basic_restrequst& and const basic_response&
        virtual void send(basic_request&&, basic_callbacktype&) = 0;

        /// @brief Support for async callback
        /// @param req Request
        /// @param callback function that accepts const basic_restrequst& and const basic_response&
        virtual void send(basic_request&&, basic_callbacktype&&) = 0;
    };


    /// @brief Serializer to ostream for RESResponseType
    static std::ostream& operator<<(std::ostream& os, const basic_response& src)
    {
        std::format_to(std::ostream_iterator<char>(os), std::string {"{}"}, src);
        return os;
    }


#pragma region Literal Operators for rest_request
    namespace literals
    {
        static rest_request<RESTMethodType::Get> operator"" _GET(const char* s, size_t sz)
        {
            return rest_request<RESTMethodType::Get>(SplitUri<>(std::string {s, sz}));
        }


        static rest_request<RESTMethodType::Delete> operator"" _DELETE(const char* s, size_t sz)
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
    } // namespace literals
#pragma endregion

} // namespace siddiqsoft

#pragma region Custom formatters
/*
 * std::format custom implementation must be global scope
 */

template <>
struct std::formatter<siddiqsoft::HTTPProtocolVersion> : std::formatter<std::string>
{
    auto format(const siddiqsoft::HTTPProtocolVersion& sv, std::format_context& ctx)
    {
        return std::formatter<std::string>::format(nlohmann::json(sv).get<std::string>(), ctx);
    }
};


template <>
struct std::formatter<siddiqsoft::RESTMethodType> : std::formatter<std::string>
{
    auto format(const siddiqsoft::RESTMethodType& sv, std::format_context& ctx)
    {
        return std::formatter<std::string>::format(nlohmann::json(sv).get<std::string>(), ctx);
    }
};


/// @brief Formatter for rest_request
template <>
struct std::formatter<siddiqsoft::basic_request> : std::formatter<std::string>
{
    auto format(const siddiqsoft::basic_request& sv, std::format_context& ctx)
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};


template <siddiqsoft::RESTMethodType RM>
struct std::formatter<siddiqsoft::rest_request<RM>> : std::formatter<std::string>
{
    template <class FC>
    auto format(const siddiqsoft::rest_request<RM>& sv, FC& ctx)
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};


template <>
struct std::formatter<siddiqsoft::basic_response> : std::formatter<std::string>
{
    auto format(const siddiqsoft::basic_response& sv, std::format_context& ctx)
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};
#pragma endregion

#else
#pragma message(warning : "C++20 required with std::format support")
#endif

#endif // !RESTREQUESTTYPE_HPP
