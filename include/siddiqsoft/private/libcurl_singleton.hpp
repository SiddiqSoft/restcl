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
#include <utility>
#include <exception>

#include "curl/curl.h"
#include "curl/easy.h"

#include "siddiqsoft/ScopeTrace.hpp"
#include "siddiqsoft/arrp.hpp"

#include "http_frame.hpp"


namespace siddiqsoft
{
    static ScopeTrace g_lct; // libcurl_trace

    /**
     * @brief Groups together the pooled CURL* and the ContentType object
     *        with the ability to on destruction return the CURL shared_ptr
     *        back to the resource_pool container.
     *
     */
    class CurlContextBundle final
    {
    public:
        CURL*                        m_handle {};
        std::shared_ptr<ContentType> _contents {std::make_shared<ContentType>()}; // Always a new instance

    public:
        CurlContextBundle() = default;
        explicit CurlContextBundle(CURL* handle)
            : m_handle {handle}
        {
        }

        CurlContextBundle(const CurlContextBundle&)            = delete;
        CurlContextBundle& operator=(const CurlContextBundle&) = delete;

        CurlContextBundle(CurlContextBundle&& item) noexcept
            : m_handle {std::exchange(item.m_handle, nullptr)}
            , _contents {std::move(item._contents)}
        {
        }

        CurlContextBundle& operator=(CurlContextBundle&& item) noexcept
        {
            if (this != &item) {
                cleanup();
                m_handle  = std::exchange(item.m_handle, nullptr);
                _contents = std::move(item._contents);
            }
            return *this;
        }

        ~CurlContextBundle() { cleanup(); }

                                     operator CURL*() { return m_handle; };
        CURL*                        curlHandle() const { return m_handle; }
        std::shared_ptr<ContentType> contents() { return _contents; }

        void abandon()
        {
            cleanup();
            _contents.reset();
        }

        void cleanup() noexcept
        {
            if (m_handle != nullptr) {
                curl_easy_cleanup(m_handle);
                m_handle = nullptr;
            }
        }
    };

    using CurlContextBundlePtr = arrp::resource_guard<CurlContextBundle>;


    /**
     * @brief LibCurSingleton provides for a facility to automatically initialize
     *        and cleanup the libcurl global/per-application handles.
     *          https://curl.se/libcurl/c/post-callback.html
     */
    class LibCurlSingleton final
    {
    protected:
        arrp::resource_pool<CurlContextBundle> curlHandlePool {[](CurlContextBundle& rsrc) {
            // This method is invoked for each resource that is invalidated
            // or about to be cleaned up.
            if (auto* handle = rsrc.curlHandle(); handle != nullptr) {
                g_lct.trace(" - Pool cleanup handler - cleanup curl handle:{}", static_cast<void*>(handle));
                rsrc.cleanup();
            }
        }};

        LibCurlSingleton() = default;

    public:
        static auto GetInstance() -> std::shared_ptr<LibCurlSingleton>
        {
            static std::shared_ptr<LibCurlSingleton> _singleton;
            static std::once_flag                    _libCurlOnceFlag;
            static const int                         DebugTraceData = 1;

            std::call_once(_libCurlOnceFlag, []() {
                if (_singleton = std::shared_ptr<LibCurlSingleton>(new LibCurlSingleton()); _singleton) {
                    // Perform once-per-application LibCURL initialization logic
                    if (auto rc = curl_global_init(CURL_GLOBAL_ALL); rc == CURLE_OK) {
                        _singleton->isInitialized = true;

                        // Set the factory callback for whenever we need a new curl handle and the
                        // pool has already loaned out everything..
                        _singleton->curlHandlePool.set_factory_callback([] {
                            auto curlHandle = curl_easy_init();
                            if (!curlHandle) {
                                throw std::runtime_error("curl_easy_init() failed");
                            }

                            if (auto rc = curl_easy_setopt(curlHandle, CURLOPT_DEBUGFUNCTION, LibCurlSingleton::debugCallback);
                                rc == CURLE_OK)
                            {
                                rc = curl_easy_setopt(curlHandle, CURLOPT_DEBUGDATA, &DebugTraceData);
                                if (rc == CURLE_OK) {
                                    return CurlContextBundle {curlHandle};
                                }
                                else if (rc != CURLE_OK) {
                                    curl_easy_cleanup(curlHandle);
#if defined(DEBUG)
                                    std::println(std::cerr,
                                                 "{} - Setting the debug Callback data..FAILED: {}",
                                                 __func__,
                                                 curl_easy_strerror(rc));
#endif
                                    throw std::runtime_error(curl_easy_strerror(rc));
                                }
                            }
                            else {
                                curl_easy_cleanup(curlHandle);
#if defined(DEBUG)
                                std::println(
                                        std::cerr, "{} - Setting the debug Callback..FAILED: {}", __func__, curl_easy_strerror(rc));
#endif
                                throw std::runtime_error(curl_easy_strerror(rc));
                            }

                            throw std::runtime_error("Failed to create new CURL handle for pool.");
                        });
                    }
                    else {
#if defined(DEBUG)
                        std::println(std::cerr, "{} - Initialize failed! {}", __func__, curl_easy_strerror(rc));
#endif
                        throw std::runtime_error(curl_easy_strerror(rc));
                    }
                }
                else {
#if defined(DEBUG)
                    std::println(std::cerr, "{} - Initialize instance failed!\n", __func__);
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
            curlHandlePool.clear();
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
                if (isInitialized.load()) {
                    return curlHandlePool.try_borrow_create();
                }

                std::println(std::cerr, "{} - NOT INITIALIZED!! Capacity:{}", __func__, curlHandlePool.size());
            }
            catch (std::runtime_error& re) {
                std::println(std::cerr, "{} - Failed existing BUNDLE from pool. {}", __func__, re.what());
            }
            catch (...) {
                std::println(std::cerr, "{} - Failed existing BUNDLE from pool. unknown error\n", __func__);
            }

            return curlHandlePool.try_borrow_create();
        };


        static int debugCallback(CURL*, curl_infotype type, char* data, size_t sz, void*)
        {
#if defined(DEBUG_TRACE)
            std::println(std::cerr, "{} - {}", std::to_underlying(type), std::string(data, sz));
#endif
            return 0;
        }

#if defined(DEBUG) || defined(_DEBUG)
    public:
        std::atomic_bool isInitialized {false};
#else
    private:
        std::atomic_bool isInitialized {false};
#endif
    };

} // namespace siddiqsoft

#else
#pragma message("Unix/Linux/macOS required")
#endif

#endif
