/**
 * @file http_frame.hpp
 * @author Siddiq Software
 * @brief Base HTTP frame class with protocol, method, URI, headers, and content management.
 * @version 1.0
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 * Provides the http_frame base class for HTTP requests and responses with support for:
 * - HTTP protocol versions (HTTP/1.0, HTTP/1.1, HTTP/2, HTTP/3)
 * - All standard HTTP methods (GET, POST, PUT, DELETE, PATCH, etc.)
 * - URI parsing and manipulation
 * - Header management with JSON serialization
 * - Content type and body handling
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
    static const std::string ELEM_NEWLINE_LF {"\n"};
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
     * @class ContentType
     * @brief Manages HTTP content including type, body, and transmission state
     * 
     * @details
     * The ContentType class encapsulates HTTP message body content along with metadata
     * about the content. It tracks:
     * - Content-Type header value (e.g., "application/json")
     * - Body content as a string
     * - Total length of the content
     * - Current offset for streaming/chunked transmission
     * - Remaining bytes to transmit
     * 
     * This class supports multiple assignment operators for convenient content setting:
     * - From JSON objects (nlohmann::json)
     * - From plain strings
     * - From other ContentType objects
     * 
     * @note The class is designed to work with HTTP streaming and chunked transfer encoding
     * 
     * @example
     * @code
     * ContentType content;
     * content = nlohmann::json{{"key", "value"}};  // Sets as JSON
     * std::cout << content.type << std::endl;      // "application/json"
     * std::cout << content.body << std::endl;      // "{\"key\":\"value\"}"
     * std::cout << content.length << std::endl;    // 16
     * @endcode
     */
    class ContentType
    {
    public:
        /// @brief Content-Type header value (e.g., "application/json", "text/plain")
        std::string type {};
        
        /// @brief The actual content body as a string
        std::string body {};
        
        /// @brief Total length of the content in bytes
        size_t      length {0};
        
        /// @brief Current offset for streaming/chunked transmission
        size_t      offset {0};
        
        /// @brief Remaining bytes to transmit (length - offset)
        size_t      remainingSize {0};

        /// @brief Default constructor - creates empty content
        ContentType()  = default;
        
        /// @brief Destructor
        ~ContentType() = default;

        /// @brief Copy constructor from shared_ptr
        /// @param src Source ContentType shared pointer
        /// @details Copies all fields from source if not null
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

        /// @brief Assignment operator from shared_ptr
        /// @param src Source ContentType shared pointer
        /// @return Reference to self for chaining
        /// @details Copies all fields from source if not null
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

        /// @brief Assignment operator from JSON object
        /// @param j JSON object to assign
        /// @details Sets content-type to "application/json" and serializes JSON to body
        void operator=(const nlohmann::json& j)
        {
            body          = j.dump();
            remainingSize = length = body.length();
            type                   = CONTENT_APPLICATION_JSON;
        }

        /// @brief Assignment operator from string
        /// @param s String content to assign
        /// @details Sets content-type to "application/text" and stores string as body
        void operator=(const std::string& s)
        {
            body          = s;
            remainingSize = length = body.length();
            type                   = CONTENT_APPLICATION_TEXT;
        }

        /// @brief Parse and set content from serialized JSON string
        /// @param s Serialized JSON string
        /// @details Parses JSON string, serializes it, and sets content-type to "application/json"
        /// @note Silently fails if JSON parsing fails
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

        /// @brief Convert to string
        /// @return The body content as string
        operator std::string() const { return body; }

        /// @brief Check if content is non-empty
        /// @return true if body is not empty, false otherwise
        operator bool() const { return !body.empty(); }

        /// @brief Friend function for JSON serialization
        friend void to_json(nlohmann::json&, const ContentType&);
    };


    /**
     * @class http_frame
     * @brief Base class for HTTP requests and responses with protocol, method, URI, headers, and content management
     * 
     * @tparam CharT Character type (default: char)
     * 
     * @details
     * The http_frame class is the foundation for both HTTP requests and responses. It provides:
     * - HTTP protocol version management (HTTP/1.0, HTTP/1.1, HTTP/2, HTTP/3)
     * - HTTP method handling (GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS, TRACE, CONNECT)
     * - URI parsing and manipulation
     * - Header management with JSON serialization
     * - Content type and body handling
     * - Method chaining for fluent API
     * 
     * This is an abstract base class that must be extended by rest_request and rest_response.
     * 
     * @note The class automatically initializes with HTTP/1.1 protocol and a Date header
     * 
     * @example
     * @code
     * // Typically used through derived classes
     * rest_request<> req = "https://api.example.com/users"_GET;
     * req.setHeader("Authorization", "Bearer token")
     *    .setContent({{"name", "John"}});
     * @endcode
     */
    template <typename CharT = char>
    class http_frame
    {
#if defined(DEBUG) || defined(_DEBUG)
    public:
#else
    protected:
#endif
        /// @brief HTTP protocol version (default: HTTP/1.1)
        HttpProtocolVersionType          protocol {HttpProtocolVersionType::Http11};
        
        /// @brief HTTP method (GET, POST, etc.)
        HttpMethodType                   method {};
        
        /// @brief Parsed URI with scheme, authority, path, and query
        Uri<CharT, AuthorityHttp<CharT>> uri {};
        
        /// @brief HTTP headers as JSON object (includes Date header by default)
        nlohmann::json                   headers {{"Date", DateUtils::RFC7231()}};
        
        /// @brief HTTP message body content with type and metadata
        std::shared_ptr<ContentType>     content {new ContentType()};

    protected:
        /// @brief Helper to identify HTTP protocol version from string fragment
        /// @param fragment String starting with HTTP protocol version
        /// @return HttpProtocolVersionType enum value or UNKNOWN
        static auto isHttpProtocol(const std::string& fragment)
        {
            for (const auto& [i, p] : HttpProtocolVersions) {
                if (fragment.starts_with(p)) return i;
            }

            return HttpProtocolVersionType::UNKNOWN;
        }

        /// @brief Helper to identify HTTP method from string fragment
        /// @param fragment String containing HTTP method name
        /// @return HttpMethodType enum value or METHOD_UNKNOWN
        static auto isHttpVerb(const std::string& fragment)
        {
            for (const auto& [i, v] : HttpVerbs) {
                if (v == fragment) return i;
            }

            return HttpMethodType::METHOD_UNKNOWN;
        }

    public:
        /// @brief Virtual destructor for proper cleanup of derived classes
        virtual ~http_frame() = default;
        
        /// @brief Default constructor
        http_frame()          = default;


        /// @brief Set HTTP protocol version from enum
        /// @param p HttpProtocolVersionType enum value
        /// @return Reference to self for method chaining
        auto& setProtocol(const HttpProtocolVersionType& p)
        {
            protocol = p;
            return *this;
        }

        /// @brief Set HTTP protocol version from string
        /// @param fragment String like "HTTP/1.1", "HTTP/2", etc.
        /// @return Reference to self for method chaining
        /// @throws std::invalid_argument if protocol version is not recognized
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

        /// @brief Get the current HTTP protocol version
        /// @return HttpProtocolVersionType enum value
        auto getProtocol() { return protocol; }

        /// @brief Set HTTP method from enum
        /// @param m HttpMethodType enum value
        /// @return Reference to self for method chaining
        auto& setMethod(const HttpMethodType& m)
        {
            method = m;
            return *this;
        }

        /// @brief Set HTTP method from string
        /// @param fragment String like "GET", "POST", "PUT", etc.
        /// @return Reference to self for method chaining
        /// @throws std::invalid_argument if method is not recognized
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

        /// @brief Get the current HTTP method
        /// @return HttpMethodType enum value
        [[nodiscard]] HttpMethodType getMethod() const { return method; }

        /// @brief Set the request URI and automatically update Host header
        /// @param u Parsed URI object
        /// @return Reference to self for method chaining
        /// @details Automatically sets the Host header to "host:port" format
        auto& setUri(const Uri<char, AuthorityHttp<char>>& u)
        {
            uri              = u;
            headers[HF_HOST] = std::format("{}:{}", uri.authority.host, uri.authority.port);

            return *this;
        }

        /// @brief Get the current URI
        /// @return Const reference to the URI object
        auto& getUri() const { return uri; }

        /// @brief Set multiple headers from JSON object
        /// @param h JSON object with header key-value pairs
        /// @return Reference to self for method chaining
        /// @details Updates existing headers with new values
        auto& setHeaders(const nlohmann::json& h)
        {
            headers.update(h);
            return *this;
        }

        /// @brief Set a single header
        /// @param key Header name
        /// @param value Header value (empty string removes the header)
        /// @return Reference to self for method chaining
        /// @details If value is empty, the header is removed
        auto& setHeader(const std::string& key, const std::string& value)
        {
            if (!value.empty())
                headers[key] = value;
            else
                headers.erase(key);

            return *this;
        }

        /// @brief Get a single header value
        /// @param key Header name
        /// @return Reference to the header value
        /// @throws std::out_of_range if header does not exist
        auto& getHeader(const std::string& key) const noexcept(false) { return headers.at(key); }

        /// @brief Get all headers as JSON object
        /// @return Reference to the headers JSON object
        nlohmann::json& getHeaders() { return headers; }

    protected:
        /// @brief Encode headers to HTTP format and append to string
        /// @param rs String to append headers to
        /// @details Formats headers as "Key: Value\r\n" and ends with "\r\n"
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
        /// @brief Encode the entire HTTP frame to a byte stream
        /// @return String containing the complete HTTP message
        /// @details Pure virtual method - must be implemented by derived classes
        [[nodiscard]] virtual std::string encode() const = 0;

        /// @brief Encode only the headers to HTTP format
        /// @return String containing formatted headers
        [[nodiscard]] std::string encodeHeaders() const
        {
            std::string hs {};
            encodeHeaders_to(hs);
            return hs;
        }


        /// @brief Get the Host header value
        /// @return Host header value as string
        auto getHost() const { return headers["Host"].template get<std::string>(); }


        /// @brief Set content with explicit content type
        /// @param ctype Content-Type header value (e.g., "application/json")
        /// @param c Content body as string
        /// @return Reference to self for method chaining
        /// @throws std::invalid_argument if content type is empty but content is not, or vice versa
        /// @details Automatically sets Content-Type and Content-Length headers
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


        /// @brief Set content from string (auto-detects content type from headers)
        /// @param src Content body as string
        /// @return Reference to self for method chaining
        /// @details Uses Content-Type header if set, otherwise defaults to "application/text"
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


        /// @brief Set content from ContentType shared pointer
        /// @param src Shared pointer to ContentType object
        /// @return Reference to self for method chaining
        /// @details Swaps the internal content pointer with the provided one
        auto& setContent(std::shared_ptr<ContentType> src)
        {
            try {
                content.swap(src);
            }
            catch (std::exception& e) {
#if defined(DEBUG)
                std::print(std::cerr, "{} - Exception: {}\n", __func__, e.what());
#endif
            }
            return *this;
        }


        /// @brief Get the content object
        /// @return Const reference to the ContentType shared pointer
        auto& getContent() const { return content; }

        /// @brief Get the content body as string
        /// @return Const reference to the body string
        auto& getContentBody() const { return content->body; }

        /// @brief Get the content body as JSON object
        /// @return Parsed JSON object if content-type is JSON, nullptr otherwise
        /// @details Silently returns nullptr if JSON parsing fails
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

        /// @brief Encode the content body
        /// @return The content body as string
        /// @details Updates the content length before returning
        [[nodiscard]] auto encodeContent() const
        {
            content->length = content->body.length();
            return content->body;
        }


        /// @brief Set content from JSON object
        /// @param c JSON object to set as content
        /// @return Reference to self for method chaining
        /// @details Automatically sets Content-Type to "application/json" (or preserves custom type)
        auto& setContent(const nlohmann::json& c)
        {
            if (!c.empty() && c.is_object()) {
                // This allows us to handle such things as: application/json+custom
                if (!headers.contains(HF_CONTENT_TYPE)) headers[HF_CONTENT_TYPE] = CONTENT_APPLICATION_JSON;

                return setContent(headers.value(HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON), c.dump());
            }
            return *this;
        }

        /// @brief Friend function for JSON serialization
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
