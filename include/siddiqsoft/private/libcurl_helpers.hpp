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
        std::once_flag initFlag {};

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
