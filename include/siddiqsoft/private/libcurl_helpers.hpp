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

#include <exception>
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
                std::print(std::cerr, "{} - 2/2 Returning CURL handle..\n", __func__);
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
    protected:
        resource_pool<std::shared_ptr<CURL>> curlHandlePool {};

        LibCurlSingleton() { };

    public:
        static auto GetInstance() -> std::shared_ptr<LibCurlSingleton>
        {
            static std::shared_ptr<LibCurlSingleton> _singleton;
            static std::once_flag                    _libCurlOnceFlag;

            if (_singleton == nullptr) {
                _singleton = std::shared_ptr<LibCurlSingleton>();

                std::print(std::cerr, "LibCurlSingleton::GetInstance() - Onetime initialization!\n");
                // Perform once-per-application LibCURL initialization logic
                if (auto rc = curl_global_init(CURL_GLOBAL_ALL); rc == CURLE_OK) {
                    _singleton->isInitialized = true;
                    std::print(std::cerr, "start{} - Initialized:{}\n", __func__, _singleton->isInitialized);
                }
                else {
                    std::print(std::cerr, "start{} - Initialize failed! {}\n", __func__, curl_easy_strerror(rc));
                    throw std::runtime_error(curl_easy_strerror(rc));
                }
            }

            return _singleton;
        }

        LibCurlSingleton(LibCurlSingleton&&)                 = delete;
        auto              operator=(LibCurlSingleton&&)      = delete;
        LibCurlSingleton& operator=(const LibCurlSingleton&) = delete;


        ~LibCurlSingleton()
        {
            std::print(std::cerr, "{} - Invoked. Cleanup global LibCURL..\n", __func__);
            curl_global_cleanup();
        }


        /**
         * @brief Get a new easy CURL context.
         * Auto-clears the CURL* when this object goes out of scope.
         * @return std::shared_ptr<CURL>
         */
        [[nodiscard("Auto-clears the CURL when this object goes out of scope.")]] auto getEasyHandle() -> CurlContextBundle&&
        {
            try {
                // It is critical for us to check if we are non-empty otherwise
                // there will be a race-condition during the checkout
                if (isInitialized && (curlHandlePool.getCapacity() > 0)) {
                    // return an existing handle..
                    auto ctxbndl = new CurlContextBundle {curlHandlePool, curlHandlePool.checkout()};
                    return std::move(*ctxbndl);
                }
                else if (!isInitialized) {
                    std::print(std::cerr, "{} - NOT INITIALIZED!! Capacity:{}\n", __func__, curlHandlePool.getCapacity());
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
            std::print(std::cerr, "{} - NEW curl handle; Capacity:{}\n", __func__, curlHandlePool.getCapacity());
            auto ctxbndlnew = new CurlContextBundle {curlHandlePool, {curl_easy_init(), curl_easy_cleanup}};
            return std::move(*ctxbndlnew);
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
