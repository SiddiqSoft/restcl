/*
    restcl : Focussed REST Client for Modern C++

    BSD 3-Clause License

    Copyright (c) 2021, Siddiq Software
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
#ifndef BASIC_REQUEST_HPP
#define BASIC_REQUEST_HPP


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
    /// @brief Base for all RESTRequests
    class basic_request
    {
    protected:
        /// @brief Not directly constructible; use the derived classes to build the request
        basic_request() { }

        /// @brief Constructs a request with endpoint string into internal Uri object
        /// @param s Valid endpoint string
        explicit basic_request(const std::string& s) noexcept(false)
            : uri(s) { };

        /// @brief Not directly constructible; use the derived classes to build the request
        /// @param s The source Uri
        explicit basic_request(const Uri<char>& s) noexcept(false)
            : uri(s) { };

    public:
        /// @brief Access the "headers", "request", "content" in the json object
        /// @param key Allows access into the json object via string or json_pointer
        /// @return Non-mutable reference to the specified element.
        const auto& operator[](const auto& key) const { return rrd.at(key); }

        /// @brief Access the "headers", "request", "content" in the json object
        /// @param key Allows access into the json object via string or json_pointer
        /// @return Mutable reference to the specified element.
        auto& operator[](const auto& key) { return rrd.at(key); }


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
    inline void to_json(nlohmann::json& dest, const basic_request& src)
    {
        dest["uri"] = src.uri;
        dest["rrd"] = src.rrd;
    }


    inline std::ostream& operator<<(std::ostream& os, const basic_request& src)
    {
        os << src.encode();
        return os;
    }
} // namespace siddiqsoft

/*
 * std::format custom implementation must be global scope
 */


/// @brief Formatter for rest_request
template <>
struct std::formatter<siddiqsoft::basic_request> : std::formatter<std::string>
{
    auto format(const siddiqsoft::basic_request& sv, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};

#endif // !BASIC_REQUEST_HPP
