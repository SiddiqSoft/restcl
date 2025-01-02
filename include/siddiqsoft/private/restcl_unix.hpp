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
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <openssl/bio.h>
#include <system_error>
#if defined(__linux__) || defined(__APPLE__) || defined(FORCE_USE_OPENSSL)

#ifndef RESTCL_UNIX_HPP
#define RESTCL_UNIX_HPP

#include <optional>
#include <stdexcept>
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
#include "rest_response.hpp"
#include "basic_restclient.hpp"
#include "rest_request.hpp"
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
        std::string UserAgent {"siddiqsoft.restcl/1.6.0"};

    private:
        static const uint32_t     READBUFFERSIZE {8192};
        static const uint32_t     MAX_ZERO_READ_FROM_SSL_THRESHOLD = 5;
        static const uint32_t     MAX_SAME_READ_FROM_SSL_THRESHOLD = 10;
        static inline const char* RESTCL_ACCEPT_TYPES[4] {"application/json", "text/json", "*/*", NULL};

        bool                     isInitialized {false};
        std::once_flag           hrcInitFlag {};
        std::shared_ptr<SSL_CTX> sslCtx {};

    protected:
        std::atomic_uint64_t ioAttempt {0};
        std::atomic_uint64_t ioAttemptFailed {0};
        std::atomic_uint64_t ioConnect {0};
        std::atomic_uint64_t ioConnectFailed {0};
        std::atomic_uint64_t ioSend {0};
        std::atomic_uint64_t ioSendFailed {0};
        std::atomic_uint64_t ioReadAttempt {0};
        std::atomic_uint64_t ioRead {0};
        std::atomic_uint64_t ioReadFailed {0};
        std::atomic_uint64_t callbackAttempt {0};
        std::atomic_uint64_t callbackFailed {0};
        std::atomic_uint64_t callbackCompleted {0};

    private:
        basic_callbacktype               _callback {};
        std::array<char, READBUFFERSIZE> _buff {};

        inline void dispatchCallback(basic_callbacktype& cb, rest_request& req, std::tuple<int, rest_response&> resp)
        {
            callbackAttempt++;
            if (cb) {
                cb(req, resp);
                callbackCompleted++;
            }
            else if (_callback) {
                _callback(req, resp);
                callbackCompleted++;
            }
        }

        /// @brief Adds asynchrony to the library via the simple_pool utility
        siddiqsoft::simple_pool<RestPoolArgsType> pool {[&](RestPoolArgsType&& arg) -> void {
            // This function is invoked any time we have an item
            // The arg is moved here and belongs to use. Once this
            // method completes the lifetime of the object ends;
            // typically this is *after* we invoke the callback.
            try {
                if (!arg.request.headers.contains("Connection")) {
                    arg.request.headers["Connection"] = "Keep-Alive";
                }

                auto resp = send(arg.request);
                dispatchCallback(arg.callback, arg.req, resp);
            }
            catch (const std::system_error& se) {
                // Failed; dispatch anyways and let the client figure out the issue.
                dispatchCallback(arg.callback, arg.req, {se.code(), {}});
            }
            catch (const std::exception& ex) {
                callbackFailed++;
                std::cerr << std::format("simple_pool - processing {} pool handler \\033[48;5;1m got exception: {}\\033[39;49m "
                                         "******************************************\n",
                                         callbackAttempt.load(),
                                         ex.what());
            }
        }};

        /*
                void recv()
                {
                    bool     readOk              = false;
                    bool     doneReading         = false;
                    uint32_t nReadCounter        = 0;
                    uint32_t nZeroReadCounter    = 0;
                    uint32_t sslret              = 0;
                    uint32_t bytesRead           = 0;
                    uint32_t readOffset          = 0;
                    uint32_t remainingBufferSize = 0;

                    do {
                        nReadCounter++;
                        remainingBufferSize = sizeof(char) * (READBUFFERSIZE - readOffset);
                        if (sslCtx.get() == nullptr) break;
                        sslret = SSL_read(sslCtx.get(), _buff.data() + readOffset, remainingBufferSize);
                        if (sslret <= 0) {
                            sslret = SSL_get_error(sslCtx.get(), sslret);
                            switch (sslret) {
                                case SSL_ERROR_NONE: {
                                    doneReading = true;
                                } break;
                                case SSL_ERROR_WANT_READ: {
                                    if (nReadCounter > MAX_SAME_READ_FROM_SSL_THRESHOLD) doneReading = true;
                                } break;
                                case SSL_ERROR_WANT_WRITE: {
                                    doneReading = true;
                                } break;
                                case SSL_ERROR_ZERO_RETURN: {
                                    doneReading = true;
                                } break;
                                case SSL_ERROR_SYSCALL: {
                                    doneReading = true;
                                } break;
                                default: {
                                    readOk      = false;
                                    doneReading = true;
                                }
                            }
                        }
                        else if ((nReadCounter == 1) && (sslret > 0) && (sslret < READBUFFERSIZE)) {
                            _buff.at(sslret) = '\0';
                            readOffset += sslret;
                            doneReading = true;
                            readOk      = true;
                            break;
                        }
                        else if (sslret > 0) {
                            readOffset += sslret;
                            readOk = true;
                        }
                        else if (sslret == 0) {
                            ++nZeroReadCounter;
                            if (nZeroReadCounter > MAX_ZERO_READ_FROM_SSL_THRESHOLD) {
                                doneReading = true;
                                readOk      = false;
                            }
                        }
                    } while (!doneReading && (readOffset < READBUFFERSIZE));
                }
        */
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
            UserAgent = ua;

            if (func) _callback = std::move(func);

            // Grab a context (configure and initialize)
            std::call_once(hrcInitFlag, [&]() {
                // The SSL CTX is released when this client object goes out of scope.
                if (sslCtx = g_ossl.configure().start().getCTX(); sslCtx) {
                    // Configure the SSL library level options here..
                    SSL_CTX_set_options(sslCtx.get(), SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
                    isInitialized = true;
                }
            });

            std::cerr << __func__ << " - completed ok.\n";

            return *this;
        }

        /// @brief Implements an asynchronous invocation of the send() method
        /// @param req Request object
        /// @param callback The method will be async and there will not be a response object returned
        void send(rest_request&& req, std::optional<basic_callbacktype> callback = std::nullopt) override
        {
            if (!isInitialized) throw std::runtime_error("Initialization failed/incomplete!");

            if (!_callback && !callback.has_value())
                throw std::invalid_argument("Async operation requires you to handle the response; register callback via "
                                            "configure() or provide callback at point of invocation.");

            pool.queue(RestPoolArgsType {std::move(req), callback.has_value() ? callback.value() : _callback});
        }


        /// @brief Implements a synchronous send of the request.
        /// @param req Request object
        /// @return Response object only if the callback is not provided to emulate synchronous invocation
        [[nodiscard]] rest_response send(const rest_request& req) override noexcept(false)
        {
            auto rc {0};

            if (!isInitialized) throw std::runtime_error("Initialization failed/incomplete!");

            auto destinationHost = req.getHost();

            if (std::unique_ptr<BIO, decltype(&BIO_free_all)> io(BIO_new_ssl_connect(sslCtx.get()), &BIO_free_all); io) {
                ioAttempt++;
                if (rc = BIO_set_conn_hostname(io.get(), destinationHost.c_str()); rc == 1) {
                    if (rc = BIO_do_connect(io.get()); rc == 1) {
                        ioConnect++;
                        if (rc = BIO_do_handshake(io.get()); rc == 1) {
                            // Send the request..
                            rc = BIO_puts(io.get(), req.encode().c_str());
                            ioSend += (rc > -1);
                            ioSendFailed += (rc <= 0);

                            uint32_t          len {0};
                            uint32_t          moreData {0};
                            std::stringstream responseBuffer {};
                            do {
                                std::array<char, READBUFFERSIZE> iobuff {};
                                ioReadAttempt++;

                                // Read..
                                len = BIO_read(io.get(), iobuff.data(), iobuff.size());
                                if (len > 0) {
                                    // Pad with nul if we're less then capacity
                                    if (len < _buff.size()) iobuff.at(len) = '\0';
                                    // responseBuffer.write(iobuff.data(), len);
                                    responseBuffer << std::string {iobuff.data(), len};
                                }
                                // Check if we have any more data to read..
                                moreData = BIO_pending(io.get());
                                if (moreData == 0) break;
                            } while ((len > 0) || BIO_should_retry(io.get()));

                            // wasted performance! we must update the parse to use
                            // stringstream
                            ioRead++;
                            std::string buffer = responseBuffer.str();
                            std::clog << buffer << std::endl;
                            return rest_response::parse(buffer);
                        }
                        else {
                            ioConnectFailed++;
                            throw std::system_error(
                                    ECONNREFUSED,
                                    std::format("{}:BIO_do_handshake failed to {}; rc:{}", __func__, destinationHost, rc));
                        }
                    }
                    else {
                        throw std::system_error(
                                ECONNREFUSED, std::format("{}:BIO_do_connect failed to {}; rc:{}", __func__, destinationHost, rc));
                    }
                }
                else {
                    throw std::system_error(
                            EHOSTUNREACH,
                            std::format("{}:BIO_set_conn_hostname failed to {}; rc:{}", __func__, destinationHost, rc));
                }
            }
            else {
                ioAttemptFailed++;
                throw std::system_error(ENETUNREACH,
                                        std::format("{}:BIO_new_ssl_connect Failed SSL/BIO to {}", __func__, destinationHost));
                std::cerr << __func__ << " - Failed BIO_new_ssl_connect; rc=" << rc << std::endl;
            }

            return rest_response {};
        }

        /// @brief Serializer to ostream for RESResponseType
        friend std::ostream& operator<<(std::ostream& os, const HttpRESTClient& src);
        friend void          to_json(nlohmann::json& dest, const HttpRESTClient& src);
    };

    inline void to_json(nlohmann::json& dest, const HttpRESTClient& src)
    {
        dest["UserAgent"] = src.UserAgent;
        dest["counters"]  = {{"ioAttempt", src.ioAttempt.load()},
                             {"ioAttemptFailed", src.ioAttemptFailed.load()},
                             {"callbackAttempt", src.callbackAttempt.load()},
                             {"callbackCompleted", src.callbackCompleted.load()},
                             {"callbackFailed", src.callbackFailed.load()},
                             {"ioConnect", src.ioConnect.load()},
                             {"ioConnectFailed", src.ioConnectFailed.load()},
                             {"ioReadAttempt", src.ioReadAttempt.load()},
                             {"ioRead", src.ioRead.load()},
                             {"ioReadFailed", src.ioReadFailed.load()},
                             {"ioSendFailed", src.ioSendFailed.load()},
                             {"ioSend", src.ioSend.load()}};
    }

    inline std::ostream& operator<<(std::ostream& os, const HttpRESTClient& src)
    {
        nlohmann::json doc {src};

        os << doc.dump(3);
        return os;
    }
    using restcl = HttpRESTClient;
} // namespace siddiqsoft

template <>
struct std::formatter<siddiqsoft::HttpRESTClient> : std::formatter<std::string>
{
    template <class FC>
    auto format(const siddiqsoft::HttpRESTClient& sv, FC& ctx) const
    {
        nlohmann::json doc {sv};
        return std::formatter<std::string>::format(doc.dump(3), ctx);
    }
};


#else
#pragma message("Windows required")
#endif


#endif // !RESTCLWINHTTP_HPP
