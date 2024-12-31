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
#include "http_frame.hpp"

namespace siddiqsoft
{
    /// @brief REST Response object
    class rest_response : public http_frame
    {
        int         statusCode {0};
        std::string reasonCode {};

    public:
        /// @brief Check if the response was successful
        /// @return True iff there is no IOError and the StatusCode (99,400)
        bool success() const { return (statusCode > 99) && (statusCode < 400); }


        /// @brief Encode the request to a byte stream ready to transfer to the remote server.
        /// @return String
        std::string encode() const override
        {
            std::string rs;

            if (!content.type.empty() && content.str.empty())
                throw std::invalid_argument("Missing content body when content type is present!");

            // Request Line
            std::format_to(std::back_inserter(rs), "{} {} {}\r\n", protocol, statusCode, reasonCode);

            // Headers..
            encodeHeaders_to(rs);

            // Finally the content..
            if (!content.str.empty() && !content.type.empty()) {
                std::format_to(std::back_inserter(rs), "{}", content.str);
            }

            return rs;
        }


        /// @brief Returns the IO error if present otherwise returns the HTTP status code and reason
        /// @return response_code with the code, message which correspond with valid HTTP respose status and reason phrase or
        /// ioError and ioError message.
        /// The response contains:
        /// resp["response"]["status"] or WinHTTP error code
        /// resp["response"]["reason"] or WinHTTP error code message string
        [[nodiscard]] auto status() const -> std::pair<const unsigned, const std::string&> { return {statusCode, reasonCode}; }

    public:
        friend std::ostream& operator<<(std::ostream&, const rest_response&);


        /// @brief Construct a response object based on the given transport error
        /// @param err Specifies the transport error.
        rest_response& setStatus(const int code, const std::string& message)
        {
            statusCode = code;
            reasonCode = message;
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
