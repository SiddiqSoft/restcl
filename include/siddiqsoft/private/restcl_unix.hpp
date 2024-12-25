/*
    restcl : WinHTTP implementation for Windows

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
    SERVICES; LOSS OF USE, d_, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#if defined(__linux__) || defined(__APPLE__)

#ifndef RESTCL_UNIX_HPP
#define RESTCL_UNIX_HPP

#include <iostream>
#include <chrono>
#include <string>
#include <functional>
#include <memory>
#include <format>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <deque>
#include <semaphore>
#include <stop_token>


#include "nlohmann/json.hpp"
#include "../restcl.hpp"

#include "siddiqsoft/string2map.hpp"
#include "siddiqsoft/conversion-utils.hpp"

#include "siddiqsoft/simple_pool.hpp"

#include "openssl/ssl.h"


namespace siddiqsoft
{
    /// @brief Windows implementation of the basic_restclient
    class HttpRESTClient : public basic_restclient
    {
    public:
        std::string  UserAgent {"siddiqsoft.restcl/1.6.0"};
        std::wstring UserAgentW {L"siddiqsoft.restcl/1.6.0"};

    private:
        static const uint32_t        READBUFFERSIZE {8192};
        static inline const char*    RESTCL_ACCEPT_TYPES[4] {"application/json", "text/json", "*/*", NULL};
        static inline const wchar_t* RESTCL_ACCEPT_TYPES_W[4] {L"application/json", L"text/json", L"*/*", NULL};

        std::shared_ptr<SSL_CTX> sslCtx;

        /// @brief Adds asynchrony to the library via the simple_pool utility
        siddiqsoft::simple_pool<RestPoolArgsType> pool {[&](RestPoolArgsType&& arg) -> void {
            // This function is invoked any time we have an item
            // The arg is moved here and belongs to use. Once this
            // method completes the lifetime of the object ends;
            // typically this is *after* we invoke the callback.
            try {
                auto resp = send(arg.request);
                arg.callback(arg.request, resp);
            }
            catch (const std::exception&) {
            }
        }};

    public:
        HttpRESTClient(const HttpRESTClient&)            = delete;
        HttpRESTClient& operator=(const HttpRESTClient&) = delete;
        HttpRESTClient()                                 = delete;

        /// @brief Move constructor. We have the object hSession which must be transferred to our instance.
        /// @param src Source object is "cleared"
        HttpRESTClient(HttpRESTClient&& src) noexcept
            : sslCtx(std::move(src.sslCtx))
            , UserAgent(std::move(src.UserAgent))
            , UserAgentW(std::move(src.UserAgentW))
        {
        }


        /// @brief Creates the Windows REST Client with given UserAgent string
        /// Sets the HTTP/2 option and the decompression options
        /// @param ua User agent string; defaults to `siddiqsoft.restcl_unix/1.6 (generic; x64)`
        HttpRESTClient(std::shared_ptr<SSL_CTX> ssl, const std::string& ua = "siddiqsoft.restcl_unix/1.6 (generic; x64)")
            : sslCtx(ssl)
            , UserAgent(ua)
        {
            UserAgentW = ConversionUtils::convert_to<char, wchar_t>(ua);
        }


        /// @brief Implements an asynchronous invocation of the send() method
        /// @param req Request object
        /// @param callback The method will be async and there will not be a response object returned
        void send(basic_request&& req, basic_callbacktype& callback) override
        {
            pool.queue(RestPoolArgsType {std::move(req), callback});
        }


        /// @brief Implements an asynchronous invocation of the send() method
        /// @param req Request object
        /// @param callback The method will be async and there will not be a response object returned
        void send(basic_request&& req, basic_callbacktype&& callback) override
        {
            pool.queue(RestPoolArgsType {std::move(req), std::move(callback)});
        }


        /// @brief Implements a synchronous send of the request.
        /// @param req Request object
        /// @return Response object only if the callback is not provided to emulate synchronous invocation
        [[nodiscard]] basic_response send(basic_request& req) override
        {
            rest_response resp {0,"not-set"};

            return resp;
        }
    };

    using restcl = HttpRESTClient;
} // namespace siddiqsoft
#else
#pragma message("Windows required")
#endif

#endif // !RESTCLWINHTTP_HPP
