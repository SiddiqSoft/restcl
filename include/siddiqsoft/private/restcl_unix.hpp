/**
 * @file restcl_unix.hpp
 * @author Siddiq Software
 * @brief Unix/Linux/macOS REST client implementation using libcurl.
 * @version 1.0
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 * Provides HttpRESTClient class for Unix/Linux/macOS platforms using libcurl.
 * Features include:
 * - Connection pooling with resource management
 * - Synchronous and asynchronous HTTP operations
 * - Support for all HTTP methods and protocols (HTTP/1.0, HTTP/1.1, HTTP/2, HTTP/3)
 * - Automatic header and content handling
 * - Thread-safe operations with atomic counters
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
#include "libcurl_singleton.hpp"
#include "rest_request.hpp"

#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/string2map.hpp"
#include "siddiqsoft/conversion-utils.hpp"
#include "siddiqsoft/RunOnEnd.hpp"

#include "siddiqsoft/RWLEnvelope.hpp"
#include "siddiqsoft/simple_pool.hpp"
#include "siddiqsoft/arrp.hpp"

#include "curl/curl.h"
#include "curl/easy.h"


namespace siddiqsoft
{
    static siddiqsoft::ScopeTrace Log("restcl_unix");

    /// @brief Encapsulates libcurl error codes from various libcurl APIs
    /// @details Provides unified error handling for different libcurl error types:
    ///          - CURLcode: Easy interface errors
    ///          - CURLMcode: Multi interface errors
    ///          - CURLHcode: Header API errors
    ///          - CURLSHcode: Share interface errors
    ///          - CURLUcode: URL API errors
    ///          - uint32_t: POSIX error codes
    struct rest_result_error final
    {
        /// @brief Variant holding one of the supported error code types
        std::variant<CURLcode, CURLMcode, CURLHcode, CURLSHcode, CURLUcode, uint32_t> error {};

        /// @brief Constructor from error variant
        /// @param ve Error variant containing one of the supported error types
        rest_result_error(const std::variant<CURLcode, CURLMcode, CURLHcode, CURLSHcode, CURLUcode, uint32_t>& ve)
            : error(ve)
        {
        }

        /// @brief Convert error to string representation
        /// @return Human-readable error message
        operator std::string() const { return to_string(); }

        /// @brief Get string representation of the error
        /// @return Human-readable error message describing the error
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

    /// @brief Unix implementation of the basic_restclient
    class HttpRESTClient final : public basic_restclient<char>
    {
    private:
        static const uint32_t             READBUFFERSIZE {8192};
        static inline const char*         RESTCL_ACCEPT_TYPES[4] {"application/json", "text/json", "*/*", NULL};
        std::shared_ptr<LibCurlSingleton> singletonInstance {};
        std::atomic_bool                  isInitialized {false};
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
        basic_callbacktype                      _callback {};
        mutable std::mutex                      callbackMutex {};
        siddiqsoft::RWLEnvelope<nlohmann::json> _config {{{"userAgent", "siddiqsoft.restcl/2"},
                                                          {"trace", false},
                                                          {"id", id},
                                                          {"freshConnect", false},
                                                          {"connectTimeout", 0L},
                                                          {"timeout", 0L},
                                                          {"verifyPeer", 1L},
                                                          {"downloadDirectory", nullptr},
                                                          {"headers", nullptr}}};


        inline void dispatchCallback(basic_callbacktype cb, rest_request<char>& req, std::expected<rest_response<char>, int> resp)
        {
            callbackAttempt++;
            if (cb) {
                cb(req, resp);
                callbackCompleted++;
            }
            else {
                basic_callbacktype configuredCallback;
                {
                    std::scoped_lock lock(callbackMutex);
                    configuredCallback = _callback;
                }
                if (configuredCallback) {
                    configuredCallback(req, resp);
                    callbackCompleted++;
                }
            }
        }

        /// @brief Adds asynchrony to the library via the simple_pool utility
        siddiqsoft::simple_pool<RestPoolArgsType<char>> pool {[&](RestPoolArgsType<char>&& arg) -> void {
            thread_local auto sl = Log.nest("simple_pool/lambda");

            // This function is invoked any time we have an item
            // The arg is moved here and belongs to use. Once this
            // method completes the lifetime of the object ends;
            // typically this is *after* we invoke the callback.
            try {
                dispatchCallback(arg.callback, arg.request, send(arg.request));
            }
            catch (std::system_error& se) {
                // Failed; dispatch anyways and let the client figure out the issue.
                sl.exp(se, "processing {} pool handler \\033[48;5;1m", callbackAttempt.load());
                dispatchCallback(arg.callback, arg.request, std::unexpected<int>(se.code().value()));
            }
            catch (std::exception& ex) {
                callbackFailed++;
                sl.exp(ex, "processing {} pool handler \\033[48;5;1m", callbackAttempt.load());
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
            thread_local auto sl = Log.nest(__func__);

            if (ContentType* content {reinterpret_cast<ContentType*>(contentPtr)};
                contents && (contentPtr != nullptr) && (size > 0))
            {
                content->body.append(reinterpret_cast<char*>(contents), size * nmemb);

                sl.trace("Invoked (reading content); size:{}  nmemb:{}  readFromCurl:{}  \n", size, nmemb, size * nmemb);
                return size * nmemb;
            }

            return 0;
        }


        static size_t onSendCallback(char* libCurlBuffer, size_t size, size_t nmemb, void* contentPtr)
        {
            thread_local auto sl = Log.nest(__func__);

            sl.trace("Invoked; libCurlBuffer:{}, size:{}, nmemb:{}, contentPtr:{}..........................>>>..>>.>.",
                     static_cast<void*>(libCurlBuffer),
                     size,
                     nmemb,
                     contentPtr);

            if (ContentType* content {reinterpret_cast<ContentType*>(contentPtr)};
                (libCurlBuffer != nullptr) && (contentPtr != nullptr) && (size > 0))
            {
                auto sizeToSendToLibCurlBuffer = size * nmemb;

                if (content->remainingSize) {
                    // Clamp to the smaller of remaining data or available buffer
                    auto dataSizeToCopyToLibCurl = content->remainingSize;
                    if (dataSizeToCopyToLibCurl > sizeToSendToLibCurlBuffer) {
                        dataSizeToCopyToLibCurl = sizeToSendToLibCurlBuffer;
                    }

                    memcpy(libCurlBuffer, content->body.data() + content->offset, dataSizeToCopyToLibCurl);
                    content->offset += dataSizeToCopyToLibCurl;
                    // If we reached the size of the content buffer then we have no more to send
                    if (content->offset >= content->length)
                        content->remainingSize = 0;
                    else {
                        content->remainingSize -= dataSizeToCopyToLibCurl;
                    }

                    sl.trace("Invoked (sending content); size:{}  nmemb:{}  sizeToSendToLibCurlBuffer:{}  "
                             "remainingSize:{}  offset:{}  dataSizeToCopyToLibCurl:{}",
                             size,
                             nmemb,
                             sizeToSendToLibCurlBuffer,
                             content->remainingSize,
                             content->offset,
                             dataSizeToCopyToLibCurl);

                    return dataSizeToCopyToLibCurl;
                }
            }

            return 0;
        }


        auto& extractHeadersFromLibCurl(CurlContextBundlePtr& ctxCurl, http_frame<>& dest)
        {
            int          headerCount {0};
            curl_header* currentHeader {nullptr};
            curl_header* previousHeader {nullptr};

            do {
                if (currentHeader = curl_easy_nextheader((*ctxCurl).curlHandle(), CURLH_HEADER, -1, previousHeader); currentHeader)
                {
                    dest.setHeader(currentHeader->name, currentHeader->value);
                    previousHeader = currentHeader;
                    headerCount++;
                }
            } while (currentHeader);

            return dest;
        }

        void resetContentState(std::shared_ptr<ContentType> cntnts)
        {
            if (!cntnts) return;

            cntnts->body.clear();
            cntnts->type.clear();
            cntnts->length        = 0;
            cntnts->offset        = 0;
            cntnts->remainingSize = 0;
        }

    public:
        // Disallow copy-construct
        HttpRESTClient(const HttpRESTClient&)            = delete;
        HttpRESTClient& operator=(const HttpRESTClient&) = delete;
        // Disallow move-construct
        HttpRESTClient(const HttpRESTClient&&)            = delete;
        HttpRESTClient& operator=(const HttpRESTClient&&) = delete;

    protected:
        HttpRESTClient(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {}, std::shared_ptr<LibCurlSingleton> lci = {})
            : singletonInstance(lci)
        {
            configure(cfg, std::forward<basic_callbacktype&&>(cb));
        }

    public:
        ~HttpRESTClient() { Log.nest(__func__).trace("Cleanup:\n{}", nlohmann::json(*this).dump()); }

        /**
         * @brief Performs ONETIME configuration of the underlying provider (LibCURL)
         *
         * @param ua The UserAgent string
         * @param func Optional callback the client-level. You can also provider per-call callbacks for each REST send() operation
         * @return basic_restclient& Returns self reference for chaining.
         */
        basic_restclient& configure(const nlohmann::json& cfg = {}, basic_callbacktype&& func = {}) override
        {
            if (!cfg.is_null() && !cfg.empty())
                _config.mutate([](auto& container, const auto& cfg) noexcept { container.update(cfg); }, cfg);

            if (func) {
                std::scoped_lock lock(callbackMutex);
                _callback = std::move(func);
            }
            isInitialized.store(true, std::memory_order_release);
            return *this;
        }

        /// @brief Implements an asynchronous invocation of the send() method
        /// @param req Request object
        /// @param callback The method will be async and there will not be a response object returned
        basic_restclient& sendAsync(rest_request<>&& req, basic_callbacktype&& callback = {}) override
        {
            if (!isInitialized.load(std::memory_order_acquire))
                Log.err_throw<std::runtime_error>("Initialization failed/incomplete!");

            basic_callbacktype callbackToUse;
            {
                std::scoped_lock lock(callbackMutex);
                if (callback) {
                    callbackToUse = std::move(callback);
                }
                else {
                    callbackToUse = _callback;
                }
            }

            if (!callbackToUse)
                Log.err_throw<std::invalid_argument>("Async operation requires you to handle the response; register callback via "
                                                     "configure() or provide callback at point of invocation.");

            pool.queue(RestPoolArgsType {std::move(req), std::move(callbackToUse)});

            return *this;
        }


        void prepareContext(CurlContextBundlePtr& ctxCurl)
        {
            CURLcode          rc     = CURLcode::CURLE_NOT_BUILT_IN;
            auto              config = _config.snapshot(); // peek at the snapshot of the config to avoid locking for long periods
            thread_local auto sl     = Log.nest(__func__);

            sl.trace("Preparing context: {}", config.dump());

            if (ctxCurl && ((CURL*)(*ctxCurl).curlHandle()) != NULL) curl_easy_reset((CURL*)(*ctxCurl).curlHandle());

            if (long v = config.value("connectTimeout", 0); v > 0) {
                if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_CONNECTTIMEOUT_MS, v); rc != CURLE_OK)
                    sl.err("Error: {}", curl_easy_strerror(rc));
            }

            if (long v = config.value("timeout", 0); v > 0) {
                if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_TIMEOUT_MS, v); rc != CURLE_OK)
                    sl.err("Error: {}", curl_easy_strerror(rc));
            }

            // Set iff we're asked to disable the peer verification. Default we leave it as-is (enabled.)
            if (long v = config.value("verifyPeer", 1); v == 0) {
                if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_SSL_VERIFYPEER, v); rc != CURLE_OK)
                    sl.err("Error: {}", curl_easy_strerror(rc));
            }

            if (long v = config.value("verifyHost", 1); v == 0) {
                if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_SSL_VERIFYHOST, v); rc != CURLE_OK)
                    sl.err("Error: {}", curl_easy_strerror(rc));
            }

            if (config.value("freshConnect", false)) {
                if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_FRESH_CONNECT, 1L); rc != CURLE_OK)
                    sl.err("Error: {}", curl_easy_strerror(rc));
            }

            if (config.value("trace", false)) {
                if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_VERBOSE, 1L); rc != CURLE_OK)
                    sl.err("Error: {}", curl_easy_strerror(rc));
            }
        }


        /// @brief Implements a synchronous send of the request.
        /// @param req Request object
        /// @return Response object only if the callback is not provided to emulate synchronous invocation
        [[nodiscard]] std::expected<rest_response<>, int> send(rest_request<>& req) override
        {
            using namespace nlohmann::literals;

            rest_response<> resp {};
            CURLcode        rc {};
            auto            sl = Log.nest(__func__);

            if (!isInitialized) {
                sl.err("Not INITIALIZED for `{}` Uri: {}", req.getMethod(), req.getUri());
                return std::unexpected(EBUSY);
            }

            auto destinationHost = req.getHost();

            sl.trace("Uri: {}\n{}", req.getUri(), nlohmann::json(req).dump());

            if (auto ctxCurl = singletonInstance->getEasyHandle();
                ((CURL*)(*ctxCurl).curlHandle() != nullptr) && !destinationHost.empty())
            {
                // peek at the snapshot of the config
                auto config          = _config.snapshot();
                auto responseContent = (*ctxCurl)._contents;
                resetContentState(responseContent);

                // Configures the context with options such as timeout, connectionTimeout, verbose, freshConnect..
                prepareContext(ctxCurl);
                // Set User-Agent
                // Use the one present in the request..
                // otherwise use the one configured in the config
                // or the one set in the config headers
                if (rc = curl_easy_setopt(
                            (*ctxCurl).curlHandle(),
                            CURLOPT_USERAGENT,
                            req.getHeaders()
                                    .value("User-Agent",
                                           config.value("userAgent", config.value("/headers/User-Agent"_json_pointer, "")))
                                    .c_str());
                    rc == CURLE_OK)
                {
                    ioAttempt++;
                    if (rc = prepareIOHandlers(ctxCurl, req, responseContent); rc == CURLE_OK) {
                        if (rc = prepareStartLine(ctxCurl, req); rc == CURLE_OK) {
                            if (auto curlHeaders = prepareCurlHeaders(ctxCurl, req); curlHeaders) {
                                // Send the request..
                                if (rc = curl_easy_perform((*ctxCurl).curlHandle()); rc == CURLE_OK) {
                                    ioSend++;
                                    // Parse the response..
                                    extractStartLine(ctxCurl, resp);
                                    extractHeadersFromLibCurl(ctxCurl, resp);
                                    extractContents(responseContent, resp);
                                    return resp;
                                }
                                else {
                                    ioSendFailed++;
                                    if (config.value("trace", false)) {
                                        std::println(std::cerr,
                                                     "{} - curl_easy_perform() failed: `{}`\n{}",
                                                     __func__,
                                                     curl_easy_strerror(rc),
                                                     nlohmann::json(req).dump());
                                    }
                                }
                            }
                        }
                    }
                }

                // To reach here is failure!
                // Invalidate so we do not re-use a failed resource!
                ctxCurl.invalidate();
                if (config.value("trace", false)) {
                    sl.err("some failure `{}`; abandon context !!\n{}", curl_easy_strerror(rc), nlohmann::json(req).dump());
                }
                return std::unexpected(rc);
            }
            else {
                ioAttemptFailed++;

                sl.trace("getting context failed!\n{}", nlohmann::json(req).dump());
                return std::unexpected(ENETUNREACH);
            }

            sl.err("Fall-through failure!");
            return std::unexpected(ENOTRECOVERABLE);
        }


        CURLcode prepareIOHandlers(CurlContextBundlePtr& ctxCurl, rest_request<>& req, std::shared_ptr<ContentType> cntnts)
        {
            CURLcode          rc {CURLE_OK};
            thread_local auto sl = Log.nest(__func__);

            if (!cntnts) {
                cntnts = std::make_shared<ContentType>();
            }
            resetContentState(cntnts);

            // Setup the CURL library for callback for the *response* from the remote!
            if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_WRITEFUNCTION, onReceiveCallback); rc != CURLE_OK) {
                sl.err("Failed setting writefunction! rc:{}", curl_easy_strerror(rc));
                return rc;
            }
            else {
                sl.trace("Setting writefunction data..............................");

                if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_WRITEDATA, cntnts.get()); rc != CURLE_OK) {
                    sl.err("Failed setting writefunction data! rc:{}", curl_easy_strerror(rc));
                    return rc;
                }
            }


            if ((req.getMethod() == HttpMethodType::METHOD_PUT) || (req.getMethod() == HttpMethodType::METHOD_PATCH) ||
                (req.getMethod() == HttpMethodType::METHOD_POST))
            {
                auto& reqContent = req.getContent();
                if (reqContent && reqContent->length > 0) {
                    if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POSTFIELDS, req.getContentBody().c_str());
                        rc != CURLE_OK)
                        return rc;
                    if (rc = curl_easy_setopt(
                                (*ctxCurl).curlHandle(), CURLOPT_POSTFIELDSIZE, static_cast<long>(reqContent->length));
                        rc != CURLE_OK)
                        return rc;
                }
                else {
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POSTFIELDS, nullptr);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POSTFIELDSIZE, 0L);
                }
            }
            else {
                curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POSTFIELDS, nullptr);
                curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POSTFIELDSIZE, 0L);
            }

            curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_UPLOAD, 0L);
            curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_READFUNCTION, nullptr);
            curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_READDATA, nullptr);

            return rc;
        }

        /**
         * @brief Set the Http version, the verb/method for this request against libCURL
         *
         * @param ctxCurl The context bundle ptr
         * @param req Reference to the request
         * @return CURLcode Error from libCurl
         */
        CURLcode prepareStartLine(CurlContextBundlePtr& ctxCurl, rest_request<>& req)
        {
            CURLcode          rc {CURLE_OK};
            thread_local auto sl = Log.nest(__func__);

            // Set the protocol..
            switch (req.getProtocol()) {
                case HttpProtocolVersionType::Http1:
                    rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
                    break;
                case HttpProtocolVersionType::Http2:
                    rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
                    break;
                case HttpProtocolVersionType::Http3:
                    rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3);
                    break;
                case HttpProtocolVersionType::Http11:
                default: rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1); break;
            }

            // Set the URL..
            if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_URL, req.getUri().string().c_str()); rc != CURLE_OK) {
                sl.err("url set failed Uri: {}  Failed: {}", req.getUri().string(), curl_easy_strerror(rc));
            }

            // Disable Expect: header by default for all requests unless user provided one
            if (!req.getHeaders().contains("Expect")) {
                req.setHeader("Expect", "");
            }

            // Setup the method..
            switch (req.getMethod()) {
                case HttpMethodType::METHOD_PUT:
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_CUSTOMREQUEST, "PUT");
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POST, 1L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTPGET, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_NOBODY, 0L);
                    break;
                case HttpMethodType::METHOD_PATCH:
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_CUSTOMREQUEST, "PATCH");
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POST, 1L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTPGET, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_NOBODY, 0L);
                    break;
                case HttpMethodType::METHOD_DELETE:
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_CUSTOMREQUEST, "DELETE");
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POST, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTPGET, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_NOBODY, 0L);
                    break;
                case HttpMethodType::METHOD_OPTIONS:
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_CUSTOMREQUEST, "OPTIONS");
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POST, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTPGET, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_NOBODY, 0L);
                    break;
                case HttpMethodType::METHOD_POST:
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POST, 1L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_CUSTOMREQUEST, nullptr);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTPGET, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_NOBODY, 0L);
                    break;
                case HttpMethodType::METHOD_HEAD:
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_NOBODY, 1L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POST, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTPGET, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_CUSTOMREQUEST, nullptr);
                    break;
                case HttpMethodType::METHOD_GET:
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTPGET, 1L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_POST, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_NOBODY, 0L);
                    curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_CUSTOMREQUEST, nullptr);
                    break;
                default: sl.err("UNSUPPORTED verb: {} to {}", req.getMethod(), req.getHost()); break;
            }

            if (_config.observe([](const auto& d) noexcept { return d.value("trace", false); })) {
                sl.info("Completed: {} to {}", req.getMethod(), req.getHost());
            }

            return rc;
        }

        auto prepareCurlHeaders(CurlContextBundlePtr& ctxCurl, rest_request<>& req) -> std::shared_ptr<struct curl_slist>
        {
            CURLcode          rc = CURLE_NOT_BUILT_IN;
            thread_local auto sl = Log.nest(__func__);

            // Always capture the structure to ensure we do not lose track and cleanup as and when needed
            if (auto curlHeaders = curl_slist_append(NULL, "X-restcl-v2:"); curlHeaders != NULL) {
                try {
                    for (auto& [k, v] : req.getHeaders().items()) {
                        if (k == "Content-Length" || k == "content-length") continue;

                        sl.trace("Setting the header....{} = {}", k, v.dump());

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
                    sl.err("Exception while preparing headers for request: {}", req.getUri().string());
                }

                if (curlHeaders != NULL) {
                    // Immediately save so we ensure proper cleanup
                    std::shared_ptr<struct curl_slist> retHeaders {curlHeaders, curl_slist_free_all};
                    if (rc = curl_easy_setopt((*ctxCurl).curlHandle(), CURLOPT_HTTPHEADER, curlHeaders); rc == CURLE_OK) {
                        return retHeaders;
                    }
                }
            }

            return {};
        }


        void extractContents(std::shared_ptr<ContentType> cntnt, rest_response<>& resp)
        {
            thread_local auto sl = Log.nest(__func__);

            try {
                // Fixup the content data..type and length
                if (!cntnt->body.empty()) {
                    cntnt->type =
                            resp.getHeaders().value("content-type", resp.getHeaders().value(HF_CONTENT_TYPE, CONTENT_TEXT_PLAIN));
                }
                else {
                    cntnt->type.clear();
                }
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
                sl.exp(ex, "Got exception.");
            }

            sl.trace("Completed.");
        }


        void extractStartLine(CurlContextBundlePtr& ctxCurl, rest_response<>& dest)
        {
            CURLcode rc {CURLE_OK};
            long     sc {0};
            thread_local auto sl = Log.nest(__func__);

            if (rc = curl_easy_getinfo((*ctxCurl).curlHandle(), CURLINFO_RESPONSE_CODE, &sc); rc == CURLE_OK) {
                long vc {0};
                if (rc = curl_easy_getinfo((*ctxCurl).curlHandle(), CURLINFO_HTTP_VERSION, &vc); rc == CURLE_OK) {
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
                    if (rc = curl_easy_getinfo((*ctxCurl).curlHandle(), CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl); rc == CURLE_OK) {
                        if ((cl > 0) && (cl != -1) && dest.getContent() && (dest.getContent()->length == 0))
                            dest.getContent()->remainingSize = dest.getContent()->length = cl;
                    }
                }
            }

            if (rc != CURLE_OK) {
                sl.trace("rc:{}  sc:{}  content-length:{}", curl_easy_strerror(rc), sc, dest.getContent()->length);
            }
        }

        /// @brief Serializer to ostream for RESResponseType
        friend std::ostream& operator<<(std::ostream& os, const HttpRESTClient& src);
        friend void          to_json(nlohmann::json& dest, const HttpRESTClient& src);

    public:
        /// @brief Initializes an instance of the basic_restclient<> for Windows or *NIX systems.
        [[nodiscard]] static auto CreateInstance(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {})
        {
            // In the case of *NIX, we use Curl and thus obtain the Curl Lib singleton
            // thus setting up this new restclient with the curl singleton.
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

        os << doc.dump();
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
        return std::formatter<std::string>::format(doc.dump(), ctx);
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
            return std::format_to(ctx.out(), "{}", nlohmann::json(*resp).dump());
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
