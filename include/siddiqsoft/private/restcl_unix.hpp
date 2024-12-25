/**
 * @file restcl_unix.hpp
 * @author Abdulkareem Siddiq (github@siddiqsoft.com)
 * @brief OpenSSL based implementation of the basic_restclient
 * @version
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 */

#pragma once
#include <optional>
#include <stdexcept>
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

#include "restcl_definitions.hpp"
#include "basic_request.hpp"
#include "basic_response.hpp"
#include "basic_restclient.hpp"
#include "rest_request.hpp"
#include "rest_response.hpp"
#include "openssl_helpers.hpp"

#include "siddiqsoft/string2map.hpp"
#include "siddiqsoft/conversion-utils.hpp"

#include "siddiqsoft/simple_pool.hpp"

#include "openssl/ssl.h"


namespace siddiqsoft
{
    /**
     * @brief Global singleton for this library users OpenSSL library entry point
     *
     */
    static LibSSLSingleton g_ossl;

    /// @brief Unix implementation of the basic_restclient
    class HttpRESTClient : public basic_restclient
    {
    public:
        std::string  UserAgent {"siddiqsoft.restcl/1.6.0"};

    private:
        static const uint32_t        READBUFFERSIZE {8192};
        static inline const char*    RESTCL_ACCEPT_TYPES[4] {"application/json", "text/json", "*/*", NULL};

        std::once_flag           initFlag {};
        std::shared_ptr<SSL_CTX> sslCtx {};

        basic_callbacktype _callback {};

        /// @brief Adds asynchrony to the library via the simple_pool utility
        siddiqsoft::simple_pool<RestPoolArgsType> pool {[&](RestPoolArgsType&& arg) -> void {
            // This function is invoked any time we have an item
            // The arg is moved here and belongs to use. Once this
            // method completes the lifetime of the object ends;
            // typically this is *after* we invoke the callback.
            try {
                auto resp = send(arg.request);
                if (arg.callback) arg.callback(arg.request, resp);
            }
            catch (const std::exception&) {
            }
        }};

    public:
        HttpRESTClient(const HttpRESTClient&)            = delete;
        HttpRESTClient& operator=(const HttpRESTClient&) = delete;
        HttpRESTClient()                                 = default;

        /// @brief Move constructor. We have the object hSession which must be transferred to our instance.
        /// @param src Source object is "cleared"
        HttpRESTClient(HttpRESTClient&& src) noexcept
            : sslCtx(std::move(src.sslCtx))
            , UserAgent(std::move(src.UserAgent))
            , _callback(std::move(src._callback))
        {
        }

        /**
         * @brief Performs ONETIME configuration of the underlying provider (OpenSSL)
         * 
         * @param ua The UserAgent string
         * @param func Optional callback the client-level. You can also provider per-call callbacks for each REST send() operation
         * @return basic_restclient& Returns self reference for chaining.
         */
        basic_restclient& configure(const std::string& ua, basic_callbacktype&& func = {}) override
        {
            UserAgent  = ua;

            if (func) _callback = std::move(func);

            // Grab a context (configure and initialize)
            std::call_once(initFlag, [&]() {
                // The SSL CTX is released when this client object goes out of scope.
                sslCtx = g_ossl.configure().start().getCTX();
                // Configure the SSL library level options here..
                SSL_CTX_set_options(sslCtx.get(), SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
                });

            return *this;
        }

        /// @brief Implements an asynchronous invocation of the send() method
        /// @param req Request object
        /// @param callback The method will be async and there will not be a response object returned
        void send(basic_request&& req, std::optional<basic_callbacktype> callback = std::nullopt) override
        {
            if (!_callback && !callback.has_value())
                throw std::invalid_argument("Async operation requires you to handle the response; register callback via "
                                            "configure() or provide callback at point of invocation.");

            pool.queue(RestPoolArgsType {std::move(req), callback.has_value() ? callback.value() : _callback});
        }


        /// @brief Implements a synchronous send of the request.
        /// @param req Request object
        /// @return Response object only if the callback is not provided to emulate synchronous invocation
        [[nodiscard]] basic_response send(const basic_request& req) override
        {
            rest_response resp {0, "not-set"};

            return resp;
        }
    };

    using restcl = HttpRESTClient;
} // namespace siddiqsoft
#else
#pragma message("Windows required")
#endif

#endif // !RESTCLWINHTTP_HPP
