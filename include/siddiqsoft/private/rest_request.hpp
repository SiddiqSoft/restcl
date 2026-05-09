/**
 * @file rest_request.hpp
 * @author Siddiq Software
 * @brief REST request model with HTTP method, URI, headers, and content support.
 * @version 1.0
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 * Provides the rest_request class for building HTTP requests with support for:
 * - All standard HTTP methods (GET, POST, PUT, DELETE, PATCH, etc.)
 * - User-defined literals for convenient request creation
 * - JSON content serialization
 * - Custom headers and content types
 */

#pragma once

#ifndef REST_REQUEST_HPP
#define REST_REQUEST_HPP

#include <stdexcept>
#include <iostream>
#include <string>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/date-utils.hpp"

#include "http_frame.hpp"


namespace siddiqsoft
{
    /// @brief A REST request utility class for building HTTP requests.
    /// 
    /// @details Models an HTTP request with method, URI, headers, and content elements.
    ///          Extends http_frame to provide request-specific functionality including
    ///          encoding to HTTP wire format and user-defined literal support.
    ///          Supports all standard HTTP methods (GET, POST, PUT, DELETE, PATCH, etc.)
    ///          and automatic JSON serialization.
    /// 
    /// @tparam CharT Character type (default: char)
    /// 
    /// @example
    /// @code
    /// // Using user-defined literals
    /// using namespace siddiqsoft::restcl_literals;
    /// auto req = "https://api.example.com/users"_POST;
    /// req.setHeader("Authorization", "Bearer token")
    ///    .setContent({{"name", "John"}, {"email", "john@example.com"}});
    /// 
    /// // Or using constructors
    /// rest_request<> req(HttpMethodType::METHOD_GET, 
    ///                    Uri::parse("https://api.example.com/users"));
    /// @endcode
    template <typename CharT = char>
    class rest_request : public http_frame<CharT>
    {
    public:
        /// @brief Default constructor
        /// @details Creates an empty request with HTTP/1.1 protocol and Date header
        rest_request() = default;

        /// @brief Constructor with method and URI
        /// @param v HTTP method (GET, POST, PUT, DELETE, etc.)
        /// @param u Parsed URI object
        /// @details Initializes request with specified method and URI, automatically sets Host header
        rest_request(const HttpMethodType& v, const Uri<CharT, AuthorityHttp<CharT>> u)
        {
            this->setMethod(v);
            this->setUri(u);
        }

        /// @brief Constructor with method, URI, and headers
        /// @param v HTTP method
        /// @param u Parsed URI object
        /// @param h JSON object containing header key-value pairs
        /// @details Initializes request with method, URI, and custom headers
        rest_request(const HttpMethodType& v, const Uri<CharT, AuthorityHttp<CharT>> u, const nlohmann::json& h)
        {
            this->setHeaders(h);
            this->setMethod(v);
            this->setUri(u);
        }

        /// @brief Constructor with method, URI, headers, and content
        /// @param v HTTP method
        /// @param u Parsed URI object
        /// @param h JSON object containing header key-value pairs
        /// @param c JSON object to be serialized as request body
        /// @details Initializes complete request with all components.
        ///          Automatically sets Content-Type to "application/json" and Content-Length header.
        rest_request(const HttpMethodType&                  v,
                     const Uri<CharT, AuthorityHttp<CharT>> u,
                     const nlohmann::json&                  h,
                     const nlohmann::json&                  c)
        {
            this->setHeaders(h);
            this->setMethod(v);
            this->setUri(u);
            this->setContent(c);
        }


        /// @brief Encode the request to HTTP wire format.
        /// 
        /// @details Converts the request object into a complete HTTP message ready for transmission.
        ///          Format: REQUEST_LINE\r\nHEADERS\r\n\r\nBODY
        ///          Example: GET /users HTTP/1.1\r\nHost: api.example.com\r\n\r\n
        /// 
        /// @return String containing the complete HTTP request in wire format
        /// 
        /// @throws std::invalid_argument if Content-Type is set but body is empty
        /// 
        /// @note The request line format is: METHOD URI PROTOCOL\r\n
        /// @note Headers are formatted as: KEY: VALUE\r\n
        /// @note Body is appended after the blank line separator (\r\n\r\n)
        /// 
        /// @example
        /// @code
        /// rest_request<> req = "https://api.example.com/users"_POST;
        /// req.setContent({{"name", "John"}});
        /// std::string encoded = req.encode();
        /// // Result: "POST /users HTTP/1.1\r\nHost: api.example.com\r\n...\r\n\r\n{\"name\":\"John\"}"
        /// @endcode
        std::string encode() const override
        {
            std::string rs;

            if (!this->content->type.empty() && this->content->body.empty())
                throw std::invalid_argument("Missing content body when content type is present!");

            // Request Line
            std::format_to(std::back_inserter(rs), "{} {} {}\r\n", this->method, this->uri.urlPart, this->protocol);

            // Headers..
            this->encodeHeaders_to(rs);

            // Finally the content..
            if (!this->content->body.empty() && !this->content->type.empty()) {
                std::format_to(std::back_inserter(rs), "{}", this->content->body);
            }

            return rs;
        }

        friend std::ostream& operator<<(std::ostream&, const rest_request<>&);
        friend void          to_json(nlohmann::json& dest, const rest_request<>& src);
    };


    inline std::ostream& operator<<(std::ostream& os, const rest_request<>& src)
    {
        os << src.encode();
        return os;
    }

    namespace restcl_literals
    {
        [[nodiscard]] static rest_request<char> operator""_GET(const char* url, size_t sz)
        {
            rest_request<char> rr;
            rr.setMethod(HttpMethodType::METHOD_GET).setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request<char> operator""_HEAD(const char* url, size_t sz)
        {
            rest_request<char> rr;
            rr.setMethod(HttpMethodType::METHOD_HEAD).setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request<char> operator""_POST(const char* url, size_t sz)
        {
            rest_request<char> rr;
            rr.setMethod(HttpMethodType::METHOD_POST).setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request<char> operator""_PUT(const char* url, size_t sz)
        {
            rest_request<char> rr;
            rr.setMethod(HttpMethodType::METHOD_PUT).setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request<char> operator""_DELETE(const char* url, size_t sz)
        {
            rest_request<char> rr;
            rr.setMethod(HttpMethodType::METHOD_DELETE).setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request<char> operator""_CONNECT(const char* url, size_t sz)
        {
            rest_request<char> rr;
            rr.setMethod(HttpMethodType::METHOD_CONNECT).setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static auto operator""_OPTIONS(const char* url, size_t sz)
        {
            rest_request<char> rr;
            rr.setMethod(HttpMethodType::METHOD_OPTIONS).setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request<char> operator""_TRACE(const char* url, size_t sz)
        {
            rest_request<char> rr;
            rr.setMethod(HttpMethodType::METHOD_TRACE).setUri(std::string {url, sz});
            return rr;
        }

        [[nodiscard]] static rest_request<char> operator""_PATCH(const char* url, size_t sz)
        {
            rest_request<char> rr;
            rr.setMethod(HttpMethodType::METHOD_PATCH).setUri(std::string {url, sz});
            return rr;
        }
    } // namespace restcl_literals


    inline void to_json(nlohmann::json& dest, const rest_request<>& src)
    {
        dest = nlohmann::json {{"request", {{"method", src.method}, {"uri", src.uri}, {"protocol", src.protocol}}},
                               {"headers", src.headers},
                               {"content", src.content}};
    }

} // namespace siddiqsoft


template <>
struct std::formatter<siddiqsoft::rest_request<>> : std::formatter<std::string>
{
    template <class FC>
    auto format(const siddiqsoft::rest_request<>& sv, FC& ctx) const
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};

#endif // !REST_REQUEST_HPP
