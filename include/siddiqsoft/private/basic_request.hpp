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
    SERVICES; LOSS OF USE, *this, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
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
    class basic_request : public nlohmann::json
    {
    protected:
        /// @brief Not directly constructible; use the derived classes to build the request
        basic_request()
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
        {
        }

        /// @brief Constructs a request with endpoint string into internal Uri object
        /// @param s Valid endpoint string
        explicit basic_request(const std::string& s) noexcept(false)
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
            , uri(s) { };

        /// @brief Not directly constructible; use the derived classes to build the request
        /// @param s The source Uri
        explicit basic_request(const Uri<char>& s) noexcept(false)
            : nlohmann::json({{"request", {{"method", nullptr}, {"uri", nullptr}, {"protocol", nullptr}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
            , uri(s) { };

    public:
        /// @brief Set the content (non-JSON)
        /// @param ctype Content-Type
        /// @param c The content
        /// @return Self
        basic_request& setContent(const std::string& ctype, const std::string& c)
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
        basic_request& setContent(const nlohmann::json& c)
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
            if (this->at("content").is_object()) {
                body = this->at("content").dump();
            }
            else if (this->at("content").is_string()) {
                body = this->at("content").get<std::string>();
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

    public:
        Uri<char, AuthorityHttp<char>> uri;
    };


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
