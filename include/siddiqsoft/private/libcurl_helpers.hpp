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

#include "siddiqsoft/resource_pool.hpp"

#include "curl/curl.h"

namespace siddiqsoft
{
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

        struct auto_return_handle_to_pool
        {
            std::shared_ptr<CURL>                 curl {};
            resource_pool<std::shared_ptr<CURL>>& owningPool;

            auto_return_handle_to_pool() = delete;
            auto_return_handle_to_pool(resource_pool<std::shared_ptr<CURL>>& pool, std::shared_ptr<CURL> item)
                : curl {item}
                , owningPool {pool}
            {
                pool.checkin(curl);
            }
        };

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
        [[nodiscard("Auto-clears the CURL when this object goes out of scope.")]] auto getEasyHandle() -> std::shared_ptr<CURL>
        {
            try {
                // return an existing handle..
                return curlHandlePool.checkout();
            }
            catch (std::runtime_error& re) {
                // ignore the exception.. and..
            }

            // ..return a new handle..
            return {curl_easy_init(), curl_easy_cleanup};
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
