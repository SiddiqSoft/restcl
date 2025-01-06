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
#if defined(__linux__) || defined(__APPLE__)

#ifndef RESTCL_UNIX_HPP
#define RESTCL_UNIX_HPP

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <openssl/bio.h>
#include <sstream>
#include <system_error>
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
#include <expected>
#include <sstream>

#include "nlohmann/json.hpp"

#include "restcl_definitions.hpp"
#include "rest_response.hpp"
#include "basic_restclient.hpp"
#include "rest_request.hpp"
#include "openssl_helpers.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/string2map.hpp"
#include "siddiqsoft/conversion-utils.hpp"

#include "siddiqsoft/simple_pool.hpp"

#include "curl/curl.h"
#include "libcurl_helpers.hpp"

namespace siddiqsoft
{
    /**
     * @brief Global singleton for this library users OpenSSL library entry point
     *
     */
    static LibCurlSingleton g_ossl;

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

        bool                  isInitialized {false};
        std::once_flag        hrcInitFlag {};
        std::shared_ptr<CURL> sslCtx {};

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

        inline void dispatchCallback(basic_callbacktype& cb, rest_request& req, std::expected<rest_response, int> resp)
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
                if (!arg.request.getHeaders().contains("Connection")) arg.request.setHeaders({{"Connection", "Keep-Alive"}});

                dispatchCallback(arg.callback, arg.request, send(arg.request));
            }
            catch (std::system_error& se) {
                // Failed; dispatch anyways and let the client figure out the issue.
                dispatchCallback(arg.callback, arg.request, std::unexpected(reinterpret_cast<int>(se.code().value())));
            }
            catch (std::exception& ex) {
                callbackFailed++;
                // std::cerr << std::format("simple_pool - processing {} pool handler \\033[48;5;1m got exception: {}\\033[39;49m "
                //                          "******************************************\n",
                //                          callbackAttempt.load(),
                //                          ex.what());
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
            UserAgent = ua;

            if (func) _callback = std::move(func);

            // Grab a context (configure and initialize)
            std::call_once(hrcInitFlag, [&]() {
                // The SSL CTX is released when this client object goes out of scope.
                if (sslCtx = g_ossl.configure().start().getEasyHandle(); sslCtx) {
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
        [[nodiscard]] std::expected<rest_response, int> send(rest_request& req) override
        {
            CURLcode rc {};

            if (!isInitialized) return std::unexpected(EBUSY);

            auto destinationHost = req.getHost();

            std::cerr << std::format("{} - Uri: {}\n{}\n", __func__, req.getUri(), nlohmann::json {req}.dump(3));

            if (sslCtx && !destinationHost.empty()) {
                ioAttempt++;
                if (req.getMethod() == HttpMethodType::GET) {
                    curl_easy_setopt(sslCtx.get(), CURLOPT_URL, req.getUri().string().c_str());
                }
                else if (req.getMethod() == HttpMethodType::POST) {
                    curl_easy_setopt(sslCtx.get(), CURLOPT_URL, req.getUri().string().c_str());
                    curl_easy_setopt(sslCtx.get(), CURLOPT_POST, 1L);
                }
                
#if defined(DEBUG) || defined(_DEBUG)
                curl_easy_setopt(sslCtx.get(), CURLOPT_VERBOSE, 1L);
#endif

                // Send the request..
                if (rc = curl_easy_perform(sslCtx.get()); rc == CURLE_OK) {
                    ioSend++;
                    // std::cerr << "()()())()()()()()()()()()()()()()()()()()()()\n";
                    return rest_response {};
                }
                else {
                    ioSendFailed++;
                    std::cerr << std::format("{}:curl_easy_perform() failed:{}\n", __func__, curl_easy_strerror(rc));
                    return std::unexpected(rc);
                }
            }
            else {
                ioAttemptFailed++;
                return std::unexpected(ENETUNREACH);
            }

            return std::unexpected(ENOTRECOVERABLE);
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
