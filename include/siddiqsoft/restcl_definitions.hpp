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
#ifndef RESTCL_DEFINITIONS_HPP
#define RESTCL_DEFINITIONS_HPP


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
} // namespace siddiqsoft

/*
 * std::format custom implementation must be global scope
 */

template <>
struct std::formatter<siddiqsoft::HTTPProtocolVersion> : std::formatter<std::string>
{
    auto format(const siddiqsoft::HTTPProtocolVersion& sv, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(nlohmann::json(sv).get<std::string>(), ctx);
    }
};


template <>
struct std::formatter<siddiqsoft::RESTMethodType> : std::formatter<std::string>
{
    auto format(const siddiqsoft::RESTMethodType& sv, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(nlohmann::json(sv).get<std::string>(), ctx);
    }
};


#endif // !RESTCL_DEFINITIONS_HPP
