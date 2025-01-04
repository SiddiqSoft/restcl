/**
 * @file openssl_helpers.hpp
 * @author Abdulkareem Siddiq (github@siddiqsoft.com)
 * @brief OpenSSL v3.0 Helpers
 * @version 0.1
 * @date 2024-12-23
 *
 * @copyright Copyright (c) 2024 SiddiqSoftware
 *
 */

#pragma once
#ifndef LIBCURL_HELPERS_HPP
#define LIBCURL_HELPERS_HPP
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

#include "curl/curl.h"

namespace siddiqsoft
{
    /**
     * @brief LibSSLSingleton provides entry point and stores the various configuration for your
     *        applications use of the OpenSSL library.
     *        There is no need to perform explicit initialization and cleanup for the library.
     *        https://docs.openssl.org/3.0/man7/migration_guide/#support-of-legacy-engines
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
                // Perform once-per-application OpenSSL initialization logic
                /* Setup encryption algorithms and load error strings */
                curl_global_init(CURL_GLOBAL_ALL);
                isInitialized = true;
            });

            std::cerr << "LibCurlSingleton - start() completed.\n";

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

#endif