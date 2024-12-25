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
#ifndef BASIC_RESPONSE_HPP
#define BASIC_RESPONSE_HPP


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
    /// @brief REST Response object
    class basic_response
    {
    protected:
        basic_response() = default;

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
        const auto& operator[](const auto& key) const { return rrd.at(key); }


        /// @brief Mutable access to the underlying json object
        /// @param key Allows access into the json object via string or json_pointer
        /// @return Mutable reference to the specified element.
        auto& operator[](const auto& key) { return rrd.at(key); }


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
        response_code status() const { return {rrd["response"].value<unsigned>("status", 0), rrd["response"].value("reason", "")}; }

    public:
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(basic_response, rrd);
        friend std::ostream& operator<<(std::ostream&, const basic_response&);

    protected:
        nlohmann::json rrd {{"response", {{"version", HTTPProtocolVersion::Http2}, {"status", 0}, {"reason", ""}}},
                            {"headers", nullptr},
                            {"content", nullptr}};
    };


    /// @brief Serializer to ostream for RESResponseType
    inline std::ostream& operator<<(std::ostream& os, const basic_response& src)
    {
        os << src.encode();
        return os;
    }
} // namespace siddiqsoft


template <>
struct std::formatter<siddiqsoft::basic_response> : std::formatter<std::string>
{
    auto format(const siddiqsoft::basic_response& sv, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};

#endif // !BASIC_RESPONSE_HPP
