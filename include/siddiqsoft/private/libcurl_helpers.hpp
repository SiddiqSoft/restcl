/**
 * @file libcurl_helpers.hpp
 * @author Abdulkareem Siddiq (github@siddiqsoft.com)
 * @brief LibCURL Helpers
 * @version 0.1
 * @date 2024-12-23
 *
 * @copyright Copyright (c) 2024 SiddiqSoftware
 *
 */

#pragma once

#include <thread>
#if defined(__linux__) || defined(__APPLE__)

#ifndef LIBCURL_HELPERS_HPP
#define LIBCURL_HELPERS_HPP

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <expected>
#include <format>

#include "siddiqsoft/resource_pool.hpp"

#include "curl/curl.h"

namespace siddiqsoft
{
    class borrowed_curl_ptr
    {
    public:
        std::thread::id _owningTid {};

    private:
        std::shared_ptr<CURL>                 _hndl;
        resource_pool<std::shared_ptr<CURL>>& _pool;

    public:
        borrowed_curl_ptr() = delete;
        borrowed_curl_ptr(resource_pool<std::shared_ptr<CURL>>& pool, std::shared_ptr<CURL> item)
            : _pool {pool}
            , _hndl {std::move(item)}
        {
            _owningTid = std::this_thread::get_id();
        }

        operator CURL*() { return _hndl.get(); }

        /**
         * @brief Abandon the borrowed_curl_ptr so we do not return it to the pool!
         *        The shared_ptr is released and will be freed when ref count zero
         *        clearing the underlying CURL resource.
         *
         */
        void abandon()
        {
            std::print(std::cerr,
                       "borrowed_curl_ptr::abandon - {} Abandoning "
                       "*********************************************************************\n",
                       _owningTid);
            _hndl.reset();
        }

        ~borrowed_curl_ptr()
        {
            if (_hndl) {
                _pool.checkin(std::move(_hndl));
            }
        }
    };

    /**
     * @brief LibCurSingleton provides for a facility to automatically initialize
     *        and cleanup the libcurl global/per-application handles.
     *          https://curl.se/libcurl/c/post-callback.html
     */
    class LibCurlSingleton
    {
    private:
        std::once_flag                       initFlag {};
        resource_pool<std::shared_ptr<CURL>> curlHandlePool {};


    public:
        LibCurlSingleton()                   = default;
        LibCurlSingleton(LibCurlSingleton&&) = delete;
        auto operator=(LibCurlSingleton&&)   = delete;

        ~LibCurlSingleton() { curl_global_cleanup(); }

        auto configure() -> LibCurlSingleton& { return *this; }

        auto start() -> LibCurlSingleton&
        {
            std::call_once(initFlag, [&]() {
                // Perform once-per-application LibCURL initialization logic
                curl_global_init(CURL_GLOBAL_ALL);
                isInitialized = true;
            });
            return *this;
        }

        /**
         * @brief Get a new easy CURL context.
         * Auto-clears the CURL* when this object goes out of scope.
         * @return std::shared_ptr<CURL>
         */
        [[nodiscard("Auto-clears the CURL when this object goes out of scope.")]] auto getEasyHandle() -> borrowed_curl_ptr
        {
            try {
                // It is critical for us to check if we are non-empty otherwise
                // there will be a race-condition during the checkout
                if (isInitialized && (curlHandlePool.getCapacity() > 0)) {
                    // return an existing handle..
                    return borrowed_curl_ptr(curlHandlePool, curlHandlePool.checkout());
                }
            }
            catch (std::runtime_error& re) {
                // ignore the exception.. and..
                std::print(std::cerr, "{} - Failed getting existing handle from pool. {}\n", __func__, re.what());
            }
            catch (...) {
                std::print(std::cerr, "{} - Failed getting existing handle from pool. unknown error\n", __func__);
            }

            // ..return a new handle..
            return borrowed_curl_ptr {curlHandlePool, {curl_easy_init(), curl_easy_cleanup}};
        };

#if defined(DEBUG) || defined(_DEBUG)
    public:
        bool isInitialized {false};
#else
    private:
        bool isInitialized {false};
#endif
    };
} // namespace siddiqsoft

#endif // LIBCURL_HELPERS_HPP
#endif // defined(__linux__) || defined(__APPLE__)
