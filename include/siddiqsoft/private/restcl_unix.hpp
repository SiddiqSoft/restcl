/**
 * @file restcl_unix.hpp
 * @author Abdulkareem Siddiq (github@siddiqsoft.com)
 * @brief LibCURL based implementation of the basic_restclient
 * @version
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 */

#pragma once
#include <cstring>
#include <type_traits>
#if defined(__linux__) || defined(__APPLE__)

#ifndef RESTCL_UNIX_HPP
#define RESTCL_UNIX_HPP

#include <atomic>
#include <cerrno>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <string>
#include <memory>
#include <mutex>
#include <expected>
#include <stdio.h>
#include <exception>
#include <functional>
#include <optional>
#include <utility>
#include <variant>

#include "nlohmann/json.hpp"

#include "http_frame.hpp"
#include "rest_response.hpp"
#include "basic_restclient.hpp"
#include "rest_request.hpp"

#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/string2map.hpp"
#include "siddiqsoft/conversion-utils.hpp"
#include "siddiqsoft/RunOnEnd.hpp"

#include "siddiqsoft/simple_pool.hpp"
#include "siddiqsoft/resource_pool.hpp"

#include "curl/curl.h"
#include "curl/easy.h"


namespace siddiqsoft
{
    struct rest_result_error
    {
        std::variant<CURLcode, CURLMcode, CURLHcode, CURLSHcode, CURLUcode, uint32_t> error {};


        rest_result_error(const std::variant<CURLcode, CURLMcode, CURLHcode, CURLSHcode, CURLUcode, uint32_t>& ve)
            : error(ve)
        {
        }

        operator std::string() const { return to_string(); }

        std::string to_string() const
        {
            return std::visit(
                    [](auto&& ec) -> std::string {
                        using T = std::decay_t<decltype(ec)>;
                        if constexpr (std::is_same_v<T, CURLcode>) {
                            return curl_easy_strerror(ec);
                        }
                        else if constexpr (std::is_same_v<T, CURLMcode>) {
                            return curl_multi_strerror(ec);
                        }
                        else if constexpr (std::is_same_v<T, CURLHcode>) {
                            switch (ec) {
                                case CURLHE_OK: return "All fine. Proceed as usual";
                                case CURLHE_BADINDEX: return "There is no header with the requested index.";
                                case CURLHE_MISSING: return "No such header exists.";
                                case CURLHE_NOHEADERS: return "No headers at all have been recorded.";
                                case CURLHE_NOREQUEST: return " There was no such request number.";
                                case CURLHE_OUT_OF_MEMORY: return " Out of resources";
                                case CURLHE_BAD_ARGUMENT: return " One or more of the given arguments are bad.";
                                case CURLHE_NOT_BUILT_IN: return "HTTP support or the header API has been disabled in the build.";
                                default: return "Unknown CURLHcode";
                            }
                        }
                        else if constexpr (std::is_same_v<T, CURLSHcode>) {
                            return curl_share_strerror(ec);
                        }
                        else if constexpr (std::is_same_v<T, CURLUcode>) {
                            return curl_url_strerror(ec);
                        }
                        else if constexpr (std::is_same_v<T, uint32_t>) {
                            return strerror(ec);
                        }
                        // Unknown
                        return "rest_result_error:Unknown or Unsupported error code";
                    },
                    error);
        }
    };

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

        LibCurlSingleton() { };

    public:
        static auto GetInstance() -> std::shared_ptr<LibCurlSingleton>
        {
            static std::shared_ptr<LibCurlSingleton> _singleton;
            static std::once_flag                    _libCurlOnceFlag;

            if (_singleton == nullptr) {
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
                    std::print(std::cerr, "{} - Initialize instance failed!\n", __func__);
                }
            }

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
#if defined(DEBUG)
            std::print(std::cerr, "{} - Invoking curl_easy_init...{}\n", __func__, (void*)curlHandle);
#endif

            if (auto rc = curl_easy_setopt(curlHandle, CURLOPT_DEBUGFUNCTION, LibCurlSingleton::debugCallback); rc == CURLE_OK) {
#if defined(DEBUG)
                std::println(std::cerr, "{} - Setting the debug Callback..", __func__);
#endif
                static const int DebugTraceData = 1;
                rc = curl_easy_setopt(curlHandle, CURLOPT_DEBUGDATA, &DebugTraceData);
                if (rc != CURLE_OK) {
                    std::println(std::cerr, "{} - Setting the debug Callback data..FAILED: {}", __func__, curl_easy_strerror(rc));
                }
#if defined(DEBUG)
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
            std::println(std::cerr, "{} - {}", std::to_underlying<>(type), std::string(data, sz));
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


    /// @brief Unix implementation of the basic_restclient
    class HttpRESTClient : public basic_restclient<char>
    {
    private:
        static const uint32_t             READBUFFERSIZE {8192};
        static inline const char*         RESTCL_ACCEPT_TYPES[4] {"application/json", "text/json", "*/*", NULL};
        std::shared_ptr<LibCurlSingleton> singletonInstance {};
        bool                              isInitialized {false};
        uint32_t                          id = __COUNTER__;

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
        basic_callbacktype _callback {};
        nlohmann::json     _config {{"userAgent", "siddiqsoft.restcl/2"},
                                    {"trace", false},
                                    {"id", id},
                                    {"freshConnect", false},
                                    {"connectTimeout", 0L},
                                    {"timeout", 0L},
                                    {"verifyPeer", 1L},
                                    {"downloadDirectory", nullptr},
                                    {"headers", nullptr}};


        inline void dispatchCallback(basic_callbacktype& cb, rest_request<char>& req, std::expected<rest_response<char>, int> resp)
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
        siddiqsoft::simple_pool<RestPoolArgsType<char>> pool {[&](RestPoolArgsType<char>&& arg) -> void {
            // This function is invoked any time we have an item
            // The arg is moved here and belongs to use. Once this
            // method completes the lifetime of the object ends;
            // typically this is *after* we invoke the callback.
            try {
                dispatchCallback(arg.callback, arg.request, send(arg.request));
            }
            catch (std::system_error& se) {
                // Failed; dispatch anyways and let the client figure out the issue.
                std::print(std::cerr,
                           "simple_pool - processing {} pool handler \\033[48;5;1m got exception: {}\n",
                           callbackAttempt.load(),
                           se.what());
                dispatchCallback(arg.callback, arg.request, std::unexpected<int>(reinterpret_cast<int>(se.code().value())));
            }
            catch (std::exception& ex) {
                callbackFailed++;
                std::print(std::cerr,
                           "simple_pool - processing {} pool handler \\033[48;5;1m got exception: {}\n",
                           callbackAttempt.load(),
                           ex.what());
            }
        }};

        /**
         * @brief Called by libCurl when we're receiving data from the remote server.
         *
         * @param contents
         * @param size
         * @param nmemb
         * @param userp This is ignored and we use the internal stringstream to hold our data
         *              as we get contents from the remote server.
         * @return size_t
         */
        static size_t onReceiveCallback(void* contents, size_t size, size_t nmemb, void* contentPtr)
        {
            if (ContentType * content {reinterpret_cast<ContentType*>(contentPtr)};
                contents && (contentPtr != nullptr) && (size > 0))
            {
                content->body.append(reinterpret_cast<char*>(contents), size * nmemb);

#if defined(DEBUG0)
                std::print(std::cerr,
                           "{} - Invoked (reading content); size:{}  nmemb:{}  readFromCurl:{}  \n",
                           __func__,
                           size,
                           nmemb,
                           size * nmemb);
#endif
                return size * nmemb;
            }

            return 0;
        }


        static size_t onSendCallback(char* libCurlBuffer, size_t size, size_t nmemb, void* contentPtr)
        {
#if defined(DEBUG)
            std::println(std::cerr,
                         "{} - Invoked; libCurlBuffer:{}, size:{}, nmemb:{}, contentPtr:{}..........................>>>..>>.>.",
                         __func__,
                         libCurlBuffer,
                         size,
                         nmemb,
                         contentPtr);
#endif

            if (ContentType * content {reinterpret_cast<ContentType*>(contentPtr)};
                (libCurlBuffer != nullptr) && (contentPtr != nullptr) && (size > 0))
            {
                auto sizeToSendToLibCurlBuffer = size * nmemb;

                if (content->remainingSize) {
                    auto dataSizeToCopyToLibCurl = content->remainingSize;
                    if (dataSizeToCopyToLibCurl > sizeToSendToLibCurlBuffer) {
                        dataSizeToCopyToLibCurl = sizeToSendToLibCurlBuffer;
                        memcpy(libCurlBuffer, content->body.data() + content->offset, dataSizeToCopyToLibCurl);
                        content->offset += dataSizeToCopyToLibCurl;
                        // If we reached the size of the content buffer then we have no more to send
                        if (content->offset >= content->length)
                            content->remainingSize = 0;
                        else {
                            content->remainingSize -= dataSizeToCopyToLibCurl;
                        }
#if defined(DEBUG)
                        std::print(std::cerr,
                                   "{} - Invoked (sending content); size:{}  nmemb:{}  sizeToSendToLibCurlBuffer:{}  "
                                   "remainingSize:{}  offset:{}  dataSizeToCopyToLibCurl:{}\n",
                                   __func__,
                                   size,
                                   nmemb,
                                   sizeToSendToLibCurlBuffer,
                                   content->remainingSize,
                                   content->offset,
                                   dataSizeToCopyToLibCurl);
#endif

                        return dataSizeToCopyToLibCurl;
                    }
                }
            }

            return 0;
        }


        auto& extractHeadersFromLibCurl(CurlContextBundlePtr ctxCurl, http_frame<>& dest)
        {
            int          headerCount {0};
            curl_header* currentHeader {nullptr};
            curl_header* previousHeader {nullptr};

            do {
                if (currentHeader = curl_easy_nextheader(ctxCurl->curlHandle(), CURLH_HEADER, -1, previousHeader); currentHeader) {
                    dest.setHeader(currentHeader->name, currentHeader->value);
                    previousHeader = currentHeader;
                    headerCount++;
                }
            } while (currentHeader);

            return dest;
        }

    public:
        HttpRESTClient(const HttpRESTClient&)            = delete;
        HttpRESTClient& operator=(const HttpRESTClient&) = delete;
        // HttpRESTClient()                                 = default;

        /// @brief Move constructor. We have the object hSession which must be transferred to our instance.
        /// @param src Source object is "cleared"
        HttpRESTClient(HttpRESTClient&& src) noexcept
            : _callback(std::move(src._callback))
            , _config(src._config)
            , id(src.id)
        {
            isInitialized     = src.isInitialized;
            ioAttempt         = src.ioAttempt.load();
            ioAttemptFailed   = src.ioAttemptFailed.load();
            ioConnect         = src.ioConnect.load();
            ioConnectFailed   = src.ioConnectFailed.load();
            ioSend            = src.ioSend.load();
            ioSendFailed      = src.ioSendFailed.load();
            ioReadAttempt     = src.ioReadAttempt.load();
            ioRead            = src.ioRead.load();
            ioReadFailed      = src.ioReadFailed.load();
            callbackAttempt   = src.callbackAttempt.load();
            callbackFailed    = src.callbackFailed.load();
            callbackCompleted = src.callbackCompleted.load();
        }

    protected:
        HttpRESTClient(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {}, std::shared_ptr<LibCurlSingleton> lci = {})
            : singletonInstance(lci)
        {
            configure(cfg, std::forward<basic_callbacktype&&>(cb));
        }

    public:
        ~HttpRESTClient()
        {
#if defined(DEBUG0)
            std::print(std::cerr, "{} - Cleanup:\n{}", __func__, nlohmann::json(*this).dump(2));
#endif
        }

        /**
         * @brief Performs ONETIME configuration of the underlying provider (LibCURL)
         *
         * @param ua The UserAgent string
         * @param func Optional callback the client-level. You can also provider per-call callbacks for each REST send() operation
         * @return basic_restclient& Returns self reference for chaining.
         */
        basic_restclient& configure(const nlohmann::json& cfg = {}, basic_callbacktype&& func = {}) override
        {
            if (!cfg.is_null() && !cfg.empty()) _config.update(cfg);

            if (func) _callback = std::move(func);
            isInitialized = true;
            return *this;
        }

        /// @brief Implements an asynchronous invocation of the send() method
        /// @param req Request object
        /// @param callback The method will be async and there will not be a response object returned
        basic_restclient& sendAsync(rest_request<>&& req, basic_callbacktype&& callback = {}) override
        {
            if (!isInitialized) throw std::runtime_error("Initialization failed/incomplete!");

            if (!_callback && !callback)
                throw std::invalid_argument("Async operation requires you to handle the response; register callback via "
                                            "configure() or provide callback at point of invocation.");

            pool.queue(RestPoolArgsType {std::move(req), callback ? std::move(callback) : _callback});

            return *this;
        }


        void prepareContext(CurlContextBundlePtr ctxCurl)
        {
            CURLcode rc = CURLcode::CURLE_NOT_BUILT_IN;

            // std::print(std::cerr, "{} - Invoked.. ctxCurl:{}\n", __func__, (void*)ctxCurl->curlHandle());

            if (ctxCurl && ((CURL*)ctxCurl->curlHandle()) != NULL) curl_easy_reset((CURL*)ctxCurl->curlHandle());

            // std::print(std::cerr, "{} - Configuring options...\n", __func__);
            if (long v = _config.value("connectTimeout", 0); v > 0) {
                if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_CONNECTTIMEOUT_MS, v); rc != CURLE_OK)
                    std::print(std::cerr, "{} - Error: {}\n", __func__, curl_easy_strerror(rc));
            }

            if (long v = _config.value("timeout", 0); v > 0) {
                if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_TIMEOUT_MS, v); rc != CURLE_OK)
                    std::print(std::cerr, "{} - Error: {}\n", __func__, curl_easy_strerror(rc));
            }

            // Set iff we're asked to disable the peer verification. Default we leave it as-is (enabled.)
            if (long v = _config.value("verifyPeer", 1); v == 0) {
                if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_SSL_VERIFYPEER, v); rc != CURLE_OK)
                    std::print(std::cerr, "{} - Error: {}\n", __func__, curl_easy_strerror(rc));
            }

            if (_config.value("freshConnect", false)) {
                if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_FRESH_CONNECT, 1L); rc != CURLE_OK)
                    std::print(std::cerr, "{} - Error: {}\n", __func__, curl_easy_strerror(rc));
            }

            if (_config.value("trace", false)) {
                if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_VERBOSE, 1L); rc != CURLE_OK)
                    std::print(std::cerr, "{} - Error: {}\n", __func__, curl_easy_strerror(rc));
            }

            // std::print(std::cerr, "{} - Completed.\n", __func__);
        }


        /// @brief Implements a synchronous send of the request.
        /// @param req Request object
        /// @return Response object only if the callback is not provided to emulate synchronous invocation
        [[nodiscard]] std::expected<rest_response<>, int> send(rest_request<>& req) override
        {
            using namespace nlohmann::literals;

            rest_response<> resp {};
            CURLcode        rc {};

            if (!isInitialized) {
#if defined(DEBUG)
                std::println(std::cerr, "{} - Not INITIALIZED for `{}` Uri: {}\n", __func__, req.getMethod(), req.getUri());
#endif
                return std::unexpected(EBUSY);
            }

            auto destinationHost = req.getHost();

#if defined(DEBUG0)
            if (_config.value("trace", false)) {
                std::println(std::cerr, "{} - Uri: {}\n{}\n", __func__, req.getUri(), nlohmann::json(req).dump(3));
            }
#endif

            if (auto ctxCurl = singletonInstance->getEasyHandle();
                ((CURL*)ctxCurl->curlHandle() != nullptr) && !destinationHost.empty())
            {
                // Configures the context with options such as timeout, connectionTimeout, verbose, freshConnect..
                prepareContext(ctxCurl);
                // Set User-Agent
                // Use the one present in the request..
                // otherwise use the one configured in the config
                // or the one set in the config headers
                if (rc = curl_easy_setopt(
                            ctxCurl->curlHandle(),
                            CURLOPT_USERAGENT,
                            req.getHeaders()
                                    .value("User-Agent",
                                           _config.value("userAgent", _config.value("/headers/User-Agent"_json_pointer, "")))
                                    .c_str());
                    rc == CURLE_OK)
                {
                    ioAttempt++;
                    if (rc = prepareStartLine(ctxCurl, req); rc == CURLE_OK) {
                        if (rc = prepareIOHandlers(ctxCurl, req, ctxCurl->_contents); rc == CURLE_OK) {
                            if (auto curlHeaders = prepareCurlHeaders(ctxCurl, req); curlHeaders) {
                                // Send the request..
                                if (rc = curl_easy_perform(ctxCurl->curlHandle()); rc == CURLE_OK) {
                                    ioSend++;
                                    // Parse the response..
                                    extractStartLine(ctxCurl, resp);
                                    extractHeadersFromLibCurl(ctxCurl, resp);
                                    extractContents(ctxCurl->_contents, resp);
                                    return resp;
                                }
                                else {
                                    ioSendFailed++;
                                    if (_config.value("trace", false)) {
                                        std::print(std::cerr,
                                                   "{} - curl_easy_perform() failed: `{}`\n{}\n",
                                                   __func__,
                                                   curl_easy_strerror(rc),
                                                   nlohmann::json(req).dump(2));
                                    }
                                }
                            }
                        }
                    }
                }

                // To reach here is failure!
                // Abandon so we we dot re-use a failed resource!
                ctxCurl->abandon();
                if (_config.value("trace", false)) {
                    std::println(std::cerr,
                                 "{} - some failure `{}`; abandon context !!\n{}\n",
                                 __func__,
                                 curl_easy_strerror(rc),
                                 nlohmann::json(req).dump(2));
                }
                return std::unexpected(rc);
            }
            else {
                ioAttemptFailed++;
#if defined(DEBUG)
                std::println(std::cerr, "{} - getting context failed!\n{}\n", __func__, nlohmann::json(req).dump(2));
#endif
                return std::unexpected(ENETUNREACH);
            }

            std::println(std::cerr, "{} - Fall-through failure!\n", __func__);
            return std::unexpected(ENOTRECOVERABLE);
        }


        CURLcode prepareIOHandlers(CurlContextBundlePtr ctxCurl, rest_request<>& req, std::shared_ptr<ContentType> cntnts)
        {
            CURLcode rc {CURLE_OK};

            // Setup the CURL library for callback for the *response* from the remote!
            if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_WRITEFUNCTION, onReceiveCallback); rc != CURLE_OK) {
                std::println(std::cerr, "{} - Failed setting writefunction! rc:{}", __func__, curl_easy_strerror(rc));
                return rc;
            }
            else {
#if defined(DEBUG0)
                std::println(std::cerr, "{} - Setting writefunction data..............................", __func__);
#endif
                if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_WRITEDATA, cntnts.get()); rc != CURLE_OK) {
                    std::println(std::cerr, "{} - Failed setting writefunction data! rc:{}", __func__, curl_easy_strerror(rc));
                    return rc;
                }
            }

            // Special cases for each verb..
            if ((req.getMethod() == HttpMethodType::METHOD_PUT) || (req.getMethod() == HttpMethodType::METHOD_PATCH) ||
                (req.getMethod() == HttpMethodType::METHOD_POST))
            {
                // WARNING!
                // The cntnts is the *incoming* or *response* FROM the remote server!
                // The req object contains contents that is to be SENT *to* the remote server!
                if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_POSTFIELDS, req.getContentBody().c_str()); rc != CURLE_OK)
                    return rc;
                else {
                    if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_POSTFIELDSIZE, req.getContent()->length);
                        rc != CURLE_OK)
                        return rc;
                    else {
                        // Why do this again?!
                        // When you call the POSTFIELDS it resets the verb to POST so we have to ensure that it is set
                        // back to the requested verb such as PUT or PATCH!
                        prepareStartLine(ctxCurl, req);

#if defined(DEBUG0)
                        std::println(std::cerr,
                                     "{} - Method:{}  POSTFIELDS: -- {}:{}",
                                     __func__,
                                     req.getMethod(),
                                     req.getContent()->length,
                                     req.getContentBody());
#endif
                    }
                }
            }

            if (auto& reqContent = req.getContent(); (reqContent->length > 0)) {
#if defined(DEBUG0)
                std::println(std::cerr,
                             "{} - Method:{}  Registering data sender for {} bytes.",
                             __func__,
                             req.getMethod(),
                             reqContent->length);
#endif
                //  Set the output/send callback which will process the req's content
                if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_READFUNCTION, onSendCallback); rc != CURLE_OK) {
                    std::println(std::cerr, "{} - Failed setting readfunction! rc:{}", __func__, curl_easy_strerror(rc));
                    return rc;
                }
                else {
#if defined(DEBUG0)
                    std::println(std::cerr, "{} - Setting readfunction data.................", __func__);
#endif
                    if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_READDATA, reqContent.get()); rc != CURLE_OK) {
                        std::println(std::cerr, "{} - Failed setting readfunction data!! rc:{}", __func__, curl_easy_strerror(rc));
                        return rc;
                    }
                }
            }


            return rc;
        }

        /**
         * @brief Set the Http version, the verb/method for this request against libCURL
         *
         * @param ctxCurl The context bundle ptr
         * @param req Reference to the request
         * @return CURLcode Error from libCurl
         */
        CURLcode prepareStartLine(CurlContextBundlePtr ctxCurl, rest_request<>& req)
        {
            CURLcode rc {CURLE_OK};

            // Set the protocol..
            switch (req.getProtocol()) {
                case HttpProtocolVersionType::Http1:
                    rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
                    break;
                case HttpProtocolVersionType::Http2:
                    rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
                    break;
                case HttpProtocolVersionType::Http3:
                    rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3);
                    break;
                case HttpProtocolVersionType::Http11:
                default: rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1); break;
            }

            // Set the URL..
            if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_URL, req.getUri().string().c_str()); rc != CURLE_OK) {
                std::println(std::cerr,
                             "{} - url set failed Uri: {}  Failed: {}",
                             __func__,
                             req.getUri().string(),
                             curl_easy_strerror(rc));
            }

            // Setup the method..
            switch (req.getMethod()) {
                case HttpMethodType::METHOD_PUT:
                    curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_CUSTOMREQUEST, "PUT");
                    req.setHeader("Transfer-Encoding", {});
                    req.setHeader("Expect", {});
                    break;
                case HttpMethodType::METHOD_PATCH: curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_CUSTOMREQUEST, "PATCH"); break;
                case HttpMethodType::METHOD_DELETE: curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_CUSTOMREQUEST, "DELETE"); break;
                case HttpMethodType::METHOD_OPTIONS:
                    curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_CUSTOMREQUEST, "OPTIONS");
                    break;
                case HttpMethodType::METHOD_POST: curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_POST, 1L); break;
                case HttpMethodType::METHOD_HEAD: curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_NOBODY, 1L); break;
                case HttpMethodType::METHOD_GET:
                    curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_HTTPGET, 1L);
                    curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_POST, 0L);
                    break;
                default:
                    std::println(std::cerr, "{} - {} to {} UNSUPPORTED verb!", __func__, req.getMethod(), req.getHost());
                    break;
            }

#if defined(DEBUG)
            if (_config.value("trace", false)) {
                std::println(std::cerr, "{} - {} to {} Completed.", __func__, req.getMethod(), req.getHost());
            }
#endif

            return rc;
        }

        auto prepareCurlHeaders(CurlContextBundlePtr ctxCurl, rest_request<>& req) -> std::shared_ptr<struct curl_slist>
        {
            CURLcode rc = CURLE_NOT_BUILT_IN;

            // Always capture the structure to ensure we do not lose track and cleanup as and when needed
            if (auto curlHeaders = curl_slist_append(NULL, "X-restcl-v2:"); curlHeaders != NULL) {
                try {
                    for (auto& [k, v] : req.getHeaders().items()) {
#if defined(DEBUG0)
                        if (_config.value("trace", false)) {
                            std::print(std::cerr, "{} - Setting the header....{} = {}\n", __func__, k, v.dump());
                        }
#endif

                        if (v.is_string()) {
                            if (auto val = v.get<std::string>(); !val.empty() && (val.length() > 0)) {
                                // Only add non-empty string contents..otherwise will treat it as a remove header
                                if (auto ch = curl_slist_append(curlHeaders, std::format("{}: {}", k, val).c_str()); ch != NULL)
                                    curlHeaders = ch;
                            }
                            else {
                                // Empty value means remove that field.
                                if (auto ch = curl_slist_append(curlHeaders, std::format("{}:", k).c_str()); ch != NULL)
                                    curlHeaders = ch;
                            }
                        }
                        else if (v.is_number_unsigned()) {
                            if (auto ch = curl_slist_append(curlHeaders, std::format("{}: {}", k, v.get<uint64_t>()).c_str());
                                ch != NULL)
                                curlHeaders = ch;
                        }
                        else if (v.is_number_integer()) {
                            if (auto ch = curl_slist_append(curlHeaders, std::format("{}: {}", k, v.get<int>()).c_str());
                                ch != NULL)
                                curlHeaders = ch;
                        }
                        else if (v.empty() || v.is_null()) {
                            if (auto ch = curl_slist_append(curlHeaders, std::format("{}:", k).c_str()); ch != NULL)
                                curlHeaders = ch;
                        }
                        else if (!v.empty()) {
                            if (auto ch = curl_slist_append(curlHeaders, std::format("{}: {}", k, v.dump()).c_str()); ch != NULL)
                                curlHeaders = ch;
                        }
                    }
                }
                catch (...) {
                }

                if (curlHeaders != NULL) {
                    // Immediately save so we ensure proper cleanup
                    std::shared_ptr<struct curl_slist> retHeaders {curlHeaders, curl_slist_free_all};
                    if (rc = curl_easy_setopt(ctxCurl->curlHandle(), CURLOPT_HTTPHEADER, curlHeaders); rc == CURLE_OK) {
                        return retHeaders;
                    }
                }
            }

            return {};
        }


        void extractContents(std::shared_ptr<ContentType> cntnt, rest_response<>& resp)
        {
            try {
                // Fixup the content data..type and length
                cntnt->type = resp.getHeaders().value("content-type", resp.getHeaders().value(HF_CONTENT_TYPE, CONTENT_TEXT_PLAIN));
                // headers in libcurl are always string values so we'd need to convert them to integer
                cntnt->length =
                        std::stoi(resp.getHeaders().value(HF_CONTENT_LENGTH, resp.getHeaders().value("content-length", "0")));
                // Make sure we have the content length properly
                if ((cntnt->length == 0) && !cntnt->body.empty()) cntnt->remainingSize = cntnt->length = cntnt->body.length();
                // Fixup the content..
                // cntnt->parseFromSerializedJson(cntnt->body);
                resp.setContent(cntnt);
            }
            catch (std::exception& ex) {
                std::print(std::cerr, "{} - Error:{}\n", __func__, ex.what());
            }

#if defined(DEBUG0)
            std::print(std::cerr, "{} - Completed.", __func__);
#endif
        }


        void extractStartLine(CurlContextBundlePtr ctxCurl, rest_response<>& dest)
        {
            CURLcode rc {CURLE_OK};
            long     sc {0};

            if (rc = curl_easy_getinfo(ctxCurl->curlHandle(), CURLINFO_RESPONSE_CODE, &sc); rc == CURLE_OK) {
                long vc {0};
                if (rc = curl_easy_getinfo(ctxCurl->curlHandle(), CURLINFO_HTTP_VERSION, &vc); rc == CURLE_OK) {
                    switch (vc) {
                        case CURL_HTTP_VERSION_1_0: dest.setProtocol(HttpProtocolVersionType::Http1); break;
                        case CURL_HTTP_VERSION_1_1: dest.setProtocol(HttpProtocolVersionType::Http11); break;
                        case CURL_HTTP_VERSION_2_0: dest.setProtocol(HttpProtocolVersionType::Http2); break;
                        case CURL_HTTP_VERSION_3: dest.setProtocol(HttpProtocolVersionType::Http3); break;
                        default: dest.setProtocol(HttpProtocolVersionType::UNKNOWN);
                    }
                    dest.setStatus(sc, "");

                    // Get the read/data size from server
                    curl_off_t cl {};
                    if (rc = curl_easy_getinfo(ctxCurl->curlHandle(), CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl); rc == CURLE_OK) {
                        if ((cl > 0) && (cl != -1) && dest.getContent() && (dest.getContent()->length == 0))
                            dest.getContent()->remainingSize = dest.getContent()->length = cl;
                    }
                }
            }

#if defined(DEBUG)
            if (rc != CURLE_OK) {
                std::print(std::cerr,
                           "{} - rc:{}  sc:{}  content-length:{}",
                           __func__,
                           curl_easy_strerror(rc),
                           sc,
                           dest.getContent()->length);
            }
#endif
        }

        /// @brief Serializer to ostream for RESResponseType
        friend std::ostream& operator<<(std::ostream& os, const HttpRESTClient& src);
        friend void          to_json(nlohmann::json& dest, const HttpRESTClient& src);

    public:
        [[nodiscard]] static auto CreateInstance(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {})
        {
            return std::shared_ptr<HttpRESTClient> {
                    new HttpRESTClient(cfg, std::forward<basic_callbacktype&&>(cb), LibCurlSingleton::GetInstance())};
        }
    };

    inline void to_json(nlohmann::json& dest, const HttpRESTClient& src)
    {
        dest = nlohmann::json {{"config", src._config},
                               {"id", src.id},
                               {"ioAttempt", src.ioAttempt.load()},
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
        nlohmann::json doc(src);

        os << doc.dump(3);
        return os;
    }

    // using restcl= HttpRESTClient;
    using restcl = std::shared_ptr<HttpRESTClient>;

} // namespace siddiqsoft

template <>
struct std::formatter<CURLcode> : std::formatter<std::string>
{
    auto format(const CURLcode& rc, auto& ctx) const { return format_to(ctx.out(), "{}", curl_easy_strerror(rc)); }
};


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

template <>
struct std::formatter<siddiqsoft::rest_result_error> : std::formatter<std::string>
{
    template <class FC>
    auto format(const siddiqsoft::rest_result_error& rrc, FC& ctx) const
    {
        return std::formatter<std::string>::format(rrc.to_string(), ctx);
    }
};

template <>
struct std::formatter<std::expected<siddiqsoft::rest_response<char>, int>> : std::formatter<std::string>
{
    auto format(const std::expected<siddiqsoft::rest_response<char>, int>& resp, auto& ctx) const
    {
        if (resp.has_value()) {
            return std::format_to(ctx.out(), "{}", nlohmann::json(*resp).dump(3));
        }
        else {
            return std::format_to(ctx.out(), "IO error: {}", resp.error());
        }
    }
};

#else
#pragma message("Windows required")
#endif


#endif
