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
#include <format>
#include <thread>

#include "siddiqsoft/resource_pool.hpp"
#include "http_frame.hpp"

#include "curl/curl.h"

namespace siddiqsoft
{
    /**
     * @brief Groups together the pooled CURL* and the ContentType object
     *        with the ability to on destruction return the CURL shared_ptr
     *        back to the resource_pool container.
     *
     */
    class CurlContextBundle
    {
    public:
#if defined(DEBUG)
        std::thread::id _owningTid {};
#endif
        std::shared_ptr<CURL>                 _hndl;                         // The checkout'd curl handle
        resource_pool<std::shared_ptr<CURL>>& _pool;                         // Reference to where we should checkin
        std::shared_ptr<ContentType>          _contents {new ContentType()}; // Always a new instance

    public:
        CurlContextBundle() = delete;
        CurlContextBundle(resource_pool<std::shared_ptr<CURL>>& pool, std::shared_ptr<CURL> item)
            : _pool {pool}
            , _hndl {std::move(item)}
        {
#if defined(DEBUG)
            _owningTid = std::this_thread::get_id();
#endif
        }

        operator CURL*() { return _hndl.get(); }
        operator std::shared_ptr<ContentType>() { return _contents; }

        /**
         * @brief Abandon the CurlContextBundle so we do not return it to the pool!
         *        The shared_ptr is released and will be freed when ref count zero
         *        clearing the underlying CURL resource.
         *
         */
        void abandon()
        {
#if defined(DEBUG)
            std::print(std::cerr,
                       "CurlContextBundle::abandon - {} Abandoning CURL handle"
                       "*********************************************************************\n",
                       _owningTid);
#endif
            _hndl.reset();
        }

        ~CurlContextBundle()
        {
            // std::print(std::cerr, "{} - 1/2 Clearing contents..\n", __func__);
            _contents.reset();
            if (_hndl) {
                // std::print(std::cerr, "{} - 2/2 Returning CURL handle..\n", __func__);
                _pool.checkin(std::move(_hndl));
            }
            // std::print(std::cerr, "{} - 2/2 Done ..\n", __func__);
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
        [[nodiscard("Auto-clears the CURL when this object goes out of scope.")]] auto getEasyHandle() -> CurlContextBundle
        {
            try {
                // It is critical for us to check if we are non-empty otherwise
                // there will be a race-condition during the checkout
                if (isInitialized && (curlHandlePool.getCapacity() > 0)) {
                    // return an existing handle..
                    return CurlContextBundle(curlHandlePool, curlHandlePool.checkout());
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
            return CurlContextBundle {curlHandlePool, {curl_easy_init(), curl_easy_cleanup}};
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
