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
#ifndef REST_RESPONSE_HPP
#define REST_RESPONSE_HPP


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
#include "rest_response.hpp"

namespace siddiqsoft
{
    /// @brief REST Response object
    class rest_response : public nlohmann::json
    {
    protected:
        rest_response()
            : nlohmann::json({{"response", {{"protocol", HTTP_PROTOCOL_VERSIONS[0]}, {"status", 0}, {"reason", ""}}},
                              {"headers", nullptr},
                              {"content", nullptr}})
        {
        }

    public:
        /// @brief Set the content of the response. An attempt is made to parse to json object
        /// @param c Content from the receive
        /// @return Self
        rest_response& setContent(const std::string& c)
        {
            if (!c.empty()) {
                if (this->at("headers").value("Content-Type", "").find("json") != std::string::npos) {
                    try {
                        this->at("content") = nlohmann::json::parse(c);
                        if (!this->at("headers").contains("Content-Length")) this->at("headers")["Content-Length"] = c.length();
                        return *this;
                    }
                    catch (...) {
                    }
                }

                // We did not decode a json; assign as-is
                this->at("content") = c;
                if (!this->at("headers").contains("Content-Length")) this->at("headers")["Content-Length"] = c.length();
            }

            return *this;
        }


        /// @brief Check if the response was successful
        /// @return True iff there is no IOError and the StatusCode (99,400)
        bool success() const
        {
            using namespace nlohmann::json_literals;

            //             : nlohmann::json({{"response", {{"protocol", HTTP_PROTOCOL_VERSIONS[0]}, {"status", 0}, {"reason", ""}}},
            auto sc = this->at("response/status"_json_pointer).template get<int>();
            return (sc > 99) && (sc < 400);
        }


        /// @brief Encode the request to a byte stream ready to transfer to the remote server.
        /// @return String
        std::string encode() const
        {
            std::string rs;
            std::string body;
            auto&       hs = this->at("headers");
            auto&       rl = this->at("response");
            auto&       cs = this->at("content");

            // Request Line
            std::format_to(std::back_inserter(rs),
                           "{} {} {}\r\n",
                           rl["version"].get<std::string>(),
                           rl["status"].get<uint32_t>(),
                           rl["reason"].is_null() ? "" : rl["reason"].get<std::string>());

            // Build the content to ensure we have the content-type
            if (!cs.is_null() && cs.is_object()) {
                body = cs.dump();
            }
            else if (!cs.is_null() && cs.is_string()) {
                body = cs.get<std::string>();
            }

            // Headers..
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
        auto status() const -> std::pair<const unsigned, const std::string&>
        {
            auto&       hs = this->at("headers");
            auto&       rl = this->at("response");

            return {rl.value<unsigned>("status", 0), rl.value("reason", "")};
        }

    public:
        friend std::ostream& operator<<(std::ostream&, const rest_response&);
    public:
        rest_response(const int code = 0, const std::string& message = {}) { setStatus(code, message); }


        /// @brief Construct a response object based on the given transport error
        /// @param err Specifies the transport error.
        rest_response& setStatus(const int code, const std::string& message)
        {
            using namespace nlohmann::literals;

            if (code != 0) this->at("response/status"_json_pointer) = code;
            if (!message.empty()) this->at("response/reason"_json_pointer) = message;
            return *this;
        }
    };

    /// @brief Serializer to ostream for RESResponseType
    inline std::ostream& operator<<(std::ostream& os, const rest_response& src)
    {
        os << src.encode();
        return os;
    }
} // namespace siddiqsoft

template <>
struct std::formatter<siddiqsoft::rest_response> : std::formatter<std::string>
{
    auto format(const siddiqsoft::rest_response& sv, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(sv.encode(), ctx);
    }
};

#endif // !REST_RESPONSE_HPP
