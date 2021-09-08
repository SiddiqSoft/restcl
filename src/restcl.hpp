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
#include "siddiqsoft/azure-cpp-utils.hpp"

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
    class basic_restrequest
    {
    protected:
        /// @brief Not directly constructible; use the derived classes to build the request
        basic_restrequest() { }

        /// @brief Constructs a request with endpoint string into internal Uri object
        /// @param s Valid endpoint string
        explicit basic_restrequest(const std::string& s) noexcept(false)
            : uri(s) {};

        /// @brief Not directly constructible; use the derived classes to build the request
        /// @param s The source Uri
        explicit basic_restrequest(const Uri<char>& s) noexcept(false)
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
        basic_restrequest& setContent(const std::string& ctype, const std::string& c)
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
        basic_restrequest& setContent(const nlohmann::json& c)
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
        friend std::ostream& operator<<(std::ostream&, const basic_restrequest&);
        friend void          to_json(nlohmann::json&, const basic_restrequest&);

    public:
        Uri<char, AuthorityHttp<char>> uri;

    protected:
        nlohmann::json rrd {{"request", {{"method", nullptr}, {"uri", nullptr}, {"version", nullptr}}},
                            {"headers", nullptr},
                            {"content", nullptr}};
    };

    /// @brief Explicit implementation is required due to the restriction on direct instantiation of the basic_restrequest class.
    /// @param dest The destination json
    /// @param src The basic_restrequest class (or derived)
    static void to_json(nlohmann::json& dest, const basic_restrequest& src)
    {
        dest["uri"] = src.uri;
        dest["rrd"] = src.rrd;
    }


    /// @brief A REST request utility class. Models the request a JSON document with `request`, `headers` and `content` elements.
    template <RESTMethodType RM, HTTPProtocolVersion HttpVer = HTTPProtocolVersion::Http2>
    class RESTRequest : public basic_restrequest
    {
    public:
        /// @brief Constructor with only endpoint as string argument
        /// @param endpoint Valid endpoint string
        RESTRequest(const std::string& endpoint) noexcept(false)
            : basic_restrequest(endpoint)
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
        RESTRequest(const Uri<char>& endpoint, const nlohmann::json& h = nullptr) noexcept(false)
            : basic_restrequest(endpoint)
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
        RESTRequest(const Uri<char>& endpoint, const nlohmann::json& h, const nlohmann::json& c) noexcept(false)
            : RESTRequest(endpoint, h)
        {
            setContent(c);
        }

        /// @brief Constructor with arbitrary content. If the header Content-Type is missing then we use text/plain
        /// @param endpoint Fully qualified http schema uri
        /// @param h Required json containing the headers. At least Content-Type should be set.
        /// @param c Required string containing the content. Make sure to set "Content-Type"
        explicit RESTRequest(const Uri<char>& endpoint, const nlohmann::json& h, const std::string& c) noexcept(false)
            : RESTRequest(endpoint, h)
        {
            if (h.is_null() || (h.is_object() && !h.contains("Content-Type"))) rrd["headers"]["Content-Type"] = "text/plain";
            rrd["headers"]["Content-Length"] = c.length();
            rrd["content"]                   = c;
        }

        /// @brief Constructor with arbitrary content. If the header Content-Type is missing then we use text/plain
        /// @param endpoint Fully qualified http schema uri
        /// @param h Required json containing the headers. At least Content-Type should be set.
        /// @param c Required string containing the content. Make sure to set "Content-Type"
        explicit RESTRequest(const Uri<char>& endpoint, const nlohmann::json& h, const char* c) noexcept(false)
            : RESTRequest(endpoint, h)
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


    using ReqPost    = RESTRequest<RESTMethodType::Post>;
    using ReqPut     = RESTRequest<RESTMethodType::Put>;
    using ReqPatch   = RESTRequest<RESTMethodType::Patch>;
    using ReqGet     = RESTRequest<RESTMethodType::Get>;
    using ReqDelete  = RESTRequest<RESTMethodType::Delete>;
    using ReqOptions = RESTRequest<RESTMethodType::Options>;
    using ReqHead    = RESTRequest<RESTMethodType::Head>;


    static std::ostream& operator<<(std::ostream& os, const basic_restrequest& src)
    {
        std::format_to(std::ostream_iterator<char>(os), "{}", src);
        return os;
    }


    /// @brief REST Response object
    class basic_restresponse
    {
    protected:
        basic_restresponse() { }

    public:
        basic_restresponse(const basic_restresponse& src) noexcept
        {
            try {
                ioError     = src.ioError;
                ioErrorCode = src.ioErrorCode;
                rrd         = src.rrd;
            }
            catch (const std::exception&) {
            }
        }


        /// @brief Move constructor
        basic_restresponse(basic_restresponse&& src) noexcept
        {
            try {
                std::swap(ioError, src.ioError);
                std::swap(ioErrorCode, src.ioErrorCode);
                std::swap(rrd, src.rrd);
            }
            catch (const std::exception&) {
            }
        }


        /// @brief Move assignment operator
        /// @param src The source object
        /// @return Self
        basic_restresponse& operator=(basic_restresponse&& src) noexcept
        {
            try {
                std::swap(ioError, src.ioError);
                std::swap(ioErrorCode, src.ioErrorCode);
                std::swap(rrd, src.rrd);
            }
            catch (const std::exception&) {
            }

            return *this;
        }


        basic_restresponse& operator=(const basic_restresponse& src) noexcept
        {
            try {
                ioError     = src.ioError;
                ioErrorCode = src.ioErrorCode;
                rrd         = src.rrd;
            }
            catch (const std::exception&) {
            }

            return *this;
        }


        /// @brief Set the content of the response. An attempt is made to parse to json object
        /// @param c Content from the receive
        /// @return Self
        basic_restresponse& setContent(const std::string& c)
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
            return (ioErrorCode == 0) && (rrd["response"].value("status", 0) > 99 && rrd["response"].value("status", 0) < 400);
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
        /// @return tuple with the ioError/Error or StatusCode/Reason
        std::tuple<uint32_t, std::string> status() const
        {
            return {ioErrorCode > 0 ? ioErrorCode : rrd["response"].value("status", 0),
                    ioErrorCode > 0 ? ioError : rrd["response"].value("reason", "")};
        }

    public:
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(basic_restresponse, ioErrorCode, ioError, rrd);
        friend std::ostream& operator<<(std::ostream&, const basic_restresponse&);

    protected:
        uint32_t       ioErrorCode {0};
        std::string    ioError {};
        nlohmann::json rrd {{"response", {{"version", HTTPProtocolVersion::Http2}, {"status", 0}, {"reason", ""}}},
                            {"headers", nullptr},
                            {"content", nullptr}};
    };


    /// @brief REST Response object
    class RESTResponse : public basic_restresponse
    {
    public:
        RESTResponse() { }

        /// @brief Construct a response object based on the given transport error
        /// @param err Specifies the transport error.
        RESTResponse(const std::pair<uint32_t, std::string>& err)
        {
            ioErrorCode = std::get<0>(err);
            ioError     = std::get<1>(err);
        }
    };


    /// @brief Base class for the rest client
    class basic_restclient
    {
    public:
        std::string  UserAgent {"siddiqsoft.restcl/0.7.4"};
        std::wstring UserAgentW {L"siddiqsoft.restcl/0.7.4"};

        /// @brief The function or lambda must accept const basic_restrequest& and const basic_restresponse&
        using basic_callbacktype = std::function<void(const basic_restrequest&, const basic_restresponse&)>;

    public:
        /// @brief Implements a synchronous send and invokes the callback
        /// @param callback a function that accepts const basic_restrequst& and const basic_restresponse&
        virtual void send(basic_restrequest&&, basic_callbacktype&& callback) = 0;
    };


    /// @brief Serializer to ostream for RESResponseType
    static std::ostream& operator<<(std::ostream& os, const basic_restresponse& src)
    {
        std::format_to(std::ostream_iterator<char>(os), "{}", src);
        return os;
    }


#pragma region Literal Operators for RESTRequest
    namespace literals
    {
        static RESTRequest<RESTMethodType::Get> operator"" _GET(const char* s, size_t sz)
        {
            return RESTRequest<RESTMethodType::Get>(SplitUri<>(std::string {s, sz}));
        }


        static RESTRequest<RESTMethodType::Delete> operator"" _DELETE(const char* s, size_t sz)
        {
            return RESTRequest<RESTMethodType::Delete>(SplitUri<>(std::string {s, sz}));
        }


        static RESTRequest<RESTMethodType::Head> operator"" _HEAD(const char* s, size_t sz)
        {
            return RESTRequest<RESTMethodType::Head>(SplitUri<>(std::string {s, sz}));
        }


        static RESTRequest<RESTMethodType::Options> operator"" _OPTIONS(const char* s, size_t sz)
        {
            return RESTRequest<RESTMethodType::Options>(SplitUri<>(std::string {s, sz}));
        }


        static RESTRequest<RESTMethodType::Patch> operator"" _PATCH(const char* s, size_t sz)
        {
            return RESTRequest<RESTMethodType::Patch>(SplitUri<>(std::string {s, sz}));
        }


        static RESTRequest<RESTMethodType::Post> operator"" _POST(const char* s, size_t sz)
        {
            return RESTRequest<RESTMethodType::Post>(SplitUri(std::string {s, sz}));
        }


        static RESTRequest<RESTMethodType::Put> operator"" _PUT(const char* s, size_t sz)
        {
            return RESTRequest<RESTMethodType::Put>(SplitUri(std::string {s, sz}));
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


/// @brief Formatter for RESTRequest
template <>
struct std::formatter<siddiqsoft::basic_restrequest> : std::formatter<std::string>
{
    auto format(const siddiqsoft::basic_restrequest& sv, std::format_context& ctx)
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};


template <siddiqsoft::RESTMethodType RM>
struct std::formatter<siddiqsoft::RESTRequest<RM>> : std::formatter<std::string>
{
    template <class FC>
    auto format(const siddiqsoft::RESTRequest<RM>& sv, FC& ctx)
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};


template <>
struct std::formatter<siddiqsoft::basic_restresponse> : std::formatter<std::string>
{
    auto format(const siddiqsoft::basic_restresponse& sv, std::format_context& ctx)
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};
#pragma endregion

#else
#pragma message(warning : "C++20 required with std::format support")
#endif

#endif // !RESTREQUESTTYPE_HPP
