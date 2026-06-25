/**
 * @file libcurl_singleton.hpp
 * @author Siddiq Software
 * @brief LibCurl singleton and context bundle management for Unix/Linux/macOS.
 * @version 1.0
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 * Provides LibCurlSingleton for global libcurl initialization and CurlContextBundle
 * for managing pooled CURL handles with automatic resource cleanup.
 */

#pragma once

#if defined(__linux__) || defined(__APPLE__)

#ifndef LIBCURL_SINGLETON_HPP
#define LIBCURL_SINGLETON_HPP

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <stdio.h>
#include <exception>

#include "curl/curl.h"
#include "curl/easy.h"

#include "siddiqsoft/resource_pool.hpp"
#include "http_frame.hpp"


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
        uint32_t                              _id = __COUNTER__;

    public:
        CurlContextBundle() = delete;
        CurlContextBundle(resource_pool<std::shared_ptr<CURL>>& pool, std::shared_ptr<CURL> item)
            : _pool {pool}
            , _hndl {std::move(item)}
        {
#if defined(DEBUG0)
            _owningTid = std::this_thread::get_id();
            std::print(std::cerr, "{} - 0/0 New BUNDLE id:{}..\n", __func__, _id);
#endif
        }

        CURL*                        curlHandle() { return _hndl.get(); };
        std::shared_ptr<ContentType> contents() { return _contents; }

        /**
         * @brief Abandon the CurlContextBundle so we do not return it to the pool!
         *        The shared_ptr is released and will be freed when ref count zero
         *        clearing the underlying CURL resource.
         *
         */
        void abandon()
        {
#if defined(DEBUG0)
            std::print(std::cerr,
                       "CurlContextBundle::abandon - id:{}  {} Abandoning BUNDLE "
                       "*********************************************************************\n",
                       _id,
                       _owningTid);
#endif
            _hndl.reset();
        }

        ~CurlContextBundle()
        {
#if defined(DEBUG0)
            std::print(std::cerr, "{} - 1/2 Clearing contents..\n", __func__);
#endif

            if (_contents) _contents.reset();
            if (_hndl) {
                _pool.checkin(std::move(_hndl));
#if defined(DEBUG0)
                std::print(std::cerr,
                           "{} - 2/2 Returning BUNDLE  id:{}:{}  Capacity:{}..\n",
                           __func__,
                           _id,
                           (void*)_hndl.get(),
                           _pool.size());
#endif
                _hndl.reset();
            }
        }
    };

    using CurlContextBundlePtr = std::shared_ptr<CurlContextBundle>;


    /**
     * @brief LibCurSingleton provides for a facility to automatically initialize
     *        and cleanup the libcurl global/per-application handles.
     *          https://curl.se/libcurl/c/post-callback.html
     */
    class LibCurlSingleton
    {
    protected:
        resource_pool<std::shared_ptr<CURL>> curlHandlePool {};

        LibCurlSingleton() = default;

    public:
        static auto GetInstance() -> std::shared_ptr<LibCurlSingleton>
        {
            static std::shared_ptr<LibCurlSingleton> _singleton;
            static std::once_flag                    _libCurlOnceFlag;

            std::call_once(_libCurlOnceFlag, []() {
                if (_singleton = std::shared_ptr<LibCurlSingleton>(new LibCurlSingleton()); _singleton) {
#if defined(DEBUG0)
                    std::print(std::cerr, "{} - Onetime initialization!\n", __func__);
#endif
                    // Perform once-per-application LibCURL initialization logic
                    if (auto rc = curl_global_init(CURL_GLOBAL_ALL); rc == CURLE_OK) {
                        _singleton->isInitialized = true;
#if defined(DEBUG0)
                        std::print(std::cerr, "{} - Initialized:{} curl_global_init()\n", __func__, _singleton->isInitialized);
#endif
                    }
                    else {
#if defined(DEBUG0)
                        std::print(std::cerr, "{} - Initialize failed! {}\n", __func__, curl_easy_strerror(rc));
#endif
                        throw std::runtime_error(curl_easy_strerror(rc));
                    }
                }
                else {
#if defined(DEBUG)
                    std::print(std::cerr, "{} - Initialize instance failed!\n", __func__);
#endif
                }
            });

            return _singleton;
        }

        LibCurlSingleton(LibCurlSingleton&&)                 = delete;
        auto              operator=(LibCurlSingleton&&)      = delete;
        LibCurlSingleton& operator=(const LibCurlSingleton&) = delete;


        ~LibCurlSingleton()
        {
#if defined(DEBUG)
            std::print(std::cerr, "{} - Invoked. Cleanup {} resource_pool objects..\n", __func__, curlHandlePool.size());
#endif
            curlHandlePool.clear();
#if defined(DEBUG)
            std::print(std::cerr,
                       "{} - Invoked. Cleanup global LibCURL..\n*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-**-*\n",
                       __func__);
#endif
            curl_global_cleanup();
        }


        /**
         * @brief Get a new easy CURL context.
         * Auto-clears the CURL* when this object goes out of scope.
         * @return std::shared_ptr<CURL>
         */
        [[nodiscard("Clears the CURL when this object goes out of scope.")]] auto getEasyHandle() -> CurlContextBundlePtr
        {
            try {
                // It is critical for us to check if we are non-empty otherwise
                // there will be a race-condition during the checkout
                if (isInitialized && (curlHandlePool.size() > 0)) {
                    // return an existing handle..
                    auto ctxbndl = std::shared_ptr<CurlContextBundle>(
                            new CurlContextBundle {curlHandlePool, std::move(curlHandlePool.checkout())});
#if defined(DEBUG0)
                    std::print(std::cerr,
                               "{} - Existing BUNDLE id:{}:{}; Capacity:{}\n",
                               __func__,
                               ctxbndl->_id,
                               reinterpret_cast<void*>((CURL*)ctxbndl->curlHandle()),
                               curlHandlePool.size());
#endif
                    return ctxbndl;
                }
                else if (!isInitialized) {
                    std::print(std::cerr, "{} - NOT INITIALIZED!! Capacity:{}\n", __func__, curlHandlePool.size());
                }
            }
            catch (std::runtime_error& re) {
                // ignore the exception.. and..
                std::print(std::cerr, "{} - Failed existing BUNDLE from pool. {}\n", __func__, re.what());
            }
            catch (...) {
                std::print(std::cerr, "{} - Failed existing BUNDLE from pool. unknown error\n", __func__);
            }

            // ..return a new handle..
            auto curlHandle = curl_easy_init();
#if defined(DEBUG0)
            std::print(std::cerr, "{} - Invoking curl_easy_init...{}\n", __func__, (void*)curlHandle);
#endif

            if (auto rc = curl_easy_setopt(curlHandle, CURLOPT_DEBUGFUNCTION, LibCurlSingleton::debugCallback); rc == CURLE_OK) {
#if defined(DEBUG0)
                std::println(std::cerr, "{} - Setting the debug Callback..", __func__);
#endif
                static const int DebugTraceData = 1;
                rc                              = curl_easy_setopt(curlHandle, CURLOPT_DEBUGDATA, &DebugTraceData);
                if (rc != CURLE_OK) {
                    std::println(std::cerr, "{} - Setting the debug Callback data..FAILED: {}", __func__, curl_easy_strerror(rc));
                }
#if defined(DEBUG0)
                curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1L);
#endif
            }
            else {
                std::println(std::cerr, "{} - Setting the debug Callback..FAILED: {}", __func__, curl_easy_strerror(rc));
            }

            auto ctxbndlnew = std::shared_ptr<CurlContextBundle>(new CurlContextBundle {
                    curlHandlePool, std::shared_ptr<CURL> {curlHandle, [](CURL* cc) {
#if defined(DEBUG0)
                                                               std::print(std::cerr,
                                                                          "Invoking curl_easy_cleanup...{}\n",
                                                                          reinterpret_cast<void*>(cc));
#endif
                                                               if (cc != NULL) curl_easy_cleanup(cc);
                                                           }}});
#if defined(DEBUG0)
            std::print(std::cerr,
                       "{} - NEW BUNDLE id:{}:{}; Capacity:{}\n",
                       __func__,
                       ctxbndlnew->_id,
                       reinterpret_cast<void*>((CURL*)ctxbndlnew->curlHandle()),
                       curlHandlePool.size());
#endif
            return ctxbndlnew;
        };


        static int debugCallback(CURL*, curl_infotype type, char* data, size_t sz, void*)
        {
#if defined(DEBUG0)
            std::println(std::cerr, "{} - {}", std::to_underlying(type), std::string(data, sz));
#endif
            return 0;
        }

#if defined(DEBUG) || defined(_DEBUG)
    public:
        bool isInitialized {false};
#else
    private:
        bool isInitialized {false};
#endif
    };

} // namespace siddiqsoft

#else
#pragma message("Unix/Linux/macOS required")
#endif

#endif
