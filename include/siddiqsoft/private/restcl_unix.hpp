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

#include "nlohmann/json.hpp"

#include "restcl_definitions.hpp"
#include "rest_response.hpp"
#include "basic_restclient.hpp"
#include "rest_request.hpp"
#include "http_frame.hpp"

#include "siddiqsoft/SplitUri.hpp"
#include "siddiqsoft/string2map.hpp"
#include "siddiqsoft/conversion-utils.hpp"
#include "siddiqsoft/RunOnEnd.hpp"

#include "siddiqsoft/simple_pool.hpp"
#include "siddiqsoft/resource_pool.hpp"

#include "curl/curl.h"
#include "curl/easy.h"
#include "libcurl_helpers.hpp"


namespace siddiqsoft
{
    /**
     * @brief Global singleton for this library users LibCURL library entry point
     *        You must install libcurl-devel on UNIX systems
     */
    static LibCurlSingleton g_LibCURLSingleton;

    /// @brief Unix implementation of the basic_restclient
    class HttpRESTClient : public basic_restclient
    {
    public:
        std::string UserAgent {"siddiqsoft.restcl/1.6.0"};

    private:
        static const uint32_t     READBUFFERSIZE {8192};
        static inline const char* RESTCL_ACCEPT_TYPES[4] {"application/json", "text/json", "*/*", NULL};

        bool           isInitialized {false};
        std::once_flag hrcInitFlag {};

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


        inline void dispatchCallback(basic_callbacktype& cb, rest_request& req, std::expected<rest_response, int> resp)
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
        siddiqsoft::simple_pool<RestPoolArgsType> pool {[&](RestPoolArgsType&& arg) -> void {
            // This function is invoked any time we have an item
            // The arg is moved here and belongs to use. Once this
            // method completes the lifetime of the object ends;
            // typically this is *after* we invoke the callback.
            try {
                if (!arg.request.getHeaders().contains("Connection")) arg.request.setHeaders({{"Connection", "Keep-Alive"}});

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
         * @brief
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

#if defined(DEBUG)
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


        auto& extractHeadersFromLibCurl(borrowed_curl_ptr ctxCurl, http_frame& dest)
        {
            int          headerCount {0};
            curl_header* currentHeader {nullptr};
            curl_header* previousHeader {nullptr};

            do {
                if (currentHeader = curl_easy_nextheader(ctxCurl, CURLH_HEADER, -1, previousHeader); currentHeader) {
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
        HttpRESTClient()                                 = default;

        /// @brief Move constructor. We have the object hSession which must be transferred to our instance.
        /// @param src Source object is "cleared"
        HttpRESTClient(HttpRESTClient&& src) noexcept
            : UserAgent(std::move(src.UserAgent))
            , _callback(std::move(src._callback))
        {
        }

        /**
         * @brief Performs ONETIME configuration of the underlying provider (LibCURL)
         *
         * @param ua The UserAgent string
         * @param func Optional callback the client-level. You can also provider per-call callbacks for each REST send() operation
         * @return basic_restclient& Returns self reference for chaining.
         */
        basic_restclient& configure(const std::string& ua, basic_callbacktype&& func = {}) override
        {
            UserAgent = ua;

            if (func) _callback = std::move(func);

            // Grab a context (configure and initialize)
            std::call_once(hrcInitFlag, [&]() { isInitialized = true; });
            return *this;
        }

        /// @brief Implements an asynchronous invocation of the send() method
        /// @param req Request object
        /// @param callback The method will be async and there will not be a response object returned
        basic_restclient& sendAsync(rest_request&& req, basic_callbacktype&& callback = {}) override
        {
            if (!isInitialized) throw std::runtime_error("Initialization failed/incomplete!");

            if (!_callback && !callback)
                throw std::invalid_argument("Async operation requires you to handle the response; register callback via "
                                            "configure() or provide callback at point of invocation.");

            pool.queue(RestPoolArgsType {std::move(req), callback ? std::move(callback) : _callback});

            return *this;
        }


        // https://curl.se/libcurl/c/postinmemory.html

        /// @brief Implements a synchronous send of the request.
        /// @param req Request object
        /// @return Response object only if the callback is not provided to emulate synchronous invocation
        [[nodiscard]] std::expected<rest_response, int> send(rest_request& req) override
        {
            rest_response resp {};
            CURLcode      rc {};

            if (!isInitialized) return std::unexpected(EBUSY);

            auto destinationHost = req.getHost();

            std::print(std::cerr, "{} - Uri: {}\n{}\n", __func__, req.getUri(), nlohmann::json(req).dump(3));

            if (auto ctxCurl= g_LibCURLSingleton.getEasyHandle(); ctxCurl && !destinationHost.empty()) {
                std::shared_ptr<ContentType> _contents {new ContentType()};

                curl_easy_reset(ctxCurl);

                ioAttempt++;
                switch (req.getProtocol()) {
                    case HttpProtocolVersionType::Http1:
                        curl_easy_setopt(ctxCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
                        break;
                    case HttpProtocolVersionType::Http2:
                        curl_easy_setopt(ctxCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
                        break;
                    case HttpProtocolVersionType::Http3:
                        curl_easy_setopt(ctxCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3);
                        break;
                    default: curl_easy_setopt(ctxCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1); break;
                }

                // Next, we setup the incoming/receive callback and data
                if (rc = curl_easy_setopt(ctxCurl, CURLOPT_WRITEFUNCTION, onReceiveCallback); rc != CURLE_OK)
                    return std::unexpected<int>(rc);
                if (rc = curl_easy_setopt(ctxCurl, CURLOPT_WRITEDATA, _contents.get()); rc != CURLE_OK)
                    return std::unexpected(rc);

                // Set headers..
                struct curl_slist*   curlHeaders = nullptr;
                siddiqsoft::RunOnEnd cleanupCurlHeaders {[&curlHeaders]() {
                    // Cleans up the curlHeaders pointer when we're out of scope at this nest.
                    if (curlHeaders != nullptr) curl_slist_free_all(curlHeaders);
                }};

                curlHeaders = curl_slist_append(curlHeaders, "Expect:");
                for (auto& [k, v] : req.getHeaders().items()) {
                    // std::print( std::cerr, "{} - Setting the header....{} = {}\n", __func__, k, v.dump());

                    if (v.is_string()) {
                        curlHeaders = curl_slist_append(curlHeaders, std::format("{}: {}", k, v.get<std::string>()).c_str());
                    }
                    else if (v.is_number_unsigned()) {
                        curlHeaders = curl_slist_append(curlHeaders, std::format("{}: {}", k, v.get<uint64_t>()).c_str());
                    }
                    else if (v.is_number_integer()) {
                        curlHeaders = curl_slist_append(curlHeaders, std::format("{}: {}", k, v.get<int>()).c_str());
                    }
                    else if (v.is_null()) {
                        curlHeaders = curl_slist_append(curlHeaders, std::format("{};", k).c_str());
                    }
                    else {
                        curlHeaders = curl_slist_append(curlHeaders, std::format("{}: {}", k, v.dump()).c_str());
                    }
                }
                if (rc = curl_easy_setopt(ctxCurl, CURLOPT_HTTPHEADER, curlHeaders); rc != CURLE_OK)
                    return std::unexpected(rc);

                // Set User-Agent
                if (rc = curl_easy_setopt(
                            ctxCurl, CURLOPT_USERAGENT, req.getHeaders().value("User-Agent", UserAgent).c_str());
                    rc != CURLE_OK)
                    return std::unexpected(rc);

                // Set options for specific HTTP Methods..
                if (req.getMethod() == HttpMethodType::METHOD_GET) {
                    curl_easy_setopt(ctxCurl, CURLOPT_URL, req.getUri().string().c_str());
                    curl_easy_setopt(ctxCurl, CURLOPT_POST, 0L);
                }
                else if (req.getMethod() == HttpMethodType::METHOD_POST) {
                    curl_easy_setopt(ctxCurl, CURLOPT_URL, req.getUri().string().c_str());
                    curl_easy_setopt(ctxCurl, CURLOPT_POST, 1L);
                    // if (req.getContent() && req.getContent()->type.starts_with(CONTENT_APPLICATION_JSON)) {
                    //     if (rc = curl_easy_setopt(ctxCurl.get(), CURLOPT_COPYPOSTFIELDS, req.encodeContent().c_str());
                    //         rc != CURLE_OK)
                    //         return std::unexpected(rc);
                    //     if (rc = curl_easy_setopt(ctxCurl.get(), CURLOPT_POSTFIELDSIZE, req.getContent()->length); rc !=
                    //     CURLE_OK)
                    //         return std::unexpected(rc);
                    //     std::cerr << std::format(
                    //             "{} - Length: {} - Content:{}\n", __func__, req.getContent()->length, req.encodeContent());
                    // }
                    // else
                    {
                        // Set the output/send callback which will process the req's content
                        if (rc = curl_easy_setopt(ctxCurl, CURLOPT_READFUNCTION, onSendCallback); rc != CURLE_OK)
                            return std::unexpected(rc);
                        // If we have content to send then set the raw pointer into our callback
                        if (req.getContent()) rc = curl_easy_setopt(ctxCurl, CURLOPT_READDATA, req.getContent().get());
                    }
                }

#if defined(DEBUG) || defined(_DEBUG)
                curl_easy_setopt(ctxCurl, CURLOPT_VERBOSE, 1L);
#endif

                // Send the request..
                if (rc = curl_easy_perform(ctxCurl); rc == CURLE_OK) {
                    ioSend++;
                    // Parse the response..
                    extractStartLine( ctxCurl, resp);
                    extractHeadersFromLibCurl(ctxCurl, resp);
                    extractContents(_contents, resp);
                    return resp;
                }

                // To reach here is failure!
                ioSendFailed++;
                std::print(std::cerr, "{}:curl_easy_perform() failed: {}\n", __func__, curl_easy_strerror(rc));
                return std::unexpected(rc);
            }
            else {
                ioAttemptFailed++;
                return std::unexpected(ENETUNREACH);
            }

            std::print(std::cerr, "{} - Fall-through failure!\n", __func__);
            return std::unexpected(ENOTRECOVERABLE);
        }

        void extractContents(std::shared_ptr<ContentType> cntnt, rest_response& resp)
        {
            // Fixup the content data..type and length
            cntnt->type = resp.getHeaders().value("content-type", resp.getHeaders().value(HF_CONTENT_TYPE, CONTENT_TEXT_PLAIN));
            // headers in libcurl are always string values so we'd need to convert them to integer
            cntnt->length = std::stoi(resp.getHeaders().value(HF_CONTENT_LENGTH, resp.getHeaders().value("content-length", "0")));
            // Make sure we have the content length properly
            if ((cntnt->length == 0) && !cntnt->body.empty()) cntnt->length = cntnt->body.length();
            // Fixup the content..
            // cntnt->parseFromSerializedJson(cntnt->body);
            resp.setContent(cntnt);
        }


        void extractStartLine(borrowed_curl_ptr ctxCurl, rest_response& dest)
        {
            CURLcode rc {CURLE_OK};
            long     sc {0};

            if (rc = curl_easy_getinfo(ctxCurl, CURLINFO_RESPONSE_CODE, &sc); rc == CURLE_OK) {
                long vc {0};
                if (rc = curl_easy_getinfo(ctxCurl, CURLINFO_HTTP_VERSION, &vc); rc == CURLE_OK) {
                    switch (vc) {
                        case CURL_HTTP_VERSION_1_0: dest.setProtocol(HttpProtocolVersionType::Http1); break;
                        case CURL_HTTP_VERSION_1_1: dest.setProtocol(HttpProtocolVersionType::Http11); break;
                        case CURL_HTTP_VERSION_2_0: dest.setProtocol(HttpProtocolVersionType::Http2); break;
                        case CURL_HTTP_VERSION_3: dest.setProtocol(HttpProtocolVersionType::Http3); break;
                        default: dest.setProtocol(HttpProtocolVersionType::UNKNOWN);
                    }
                    dest.setStatus(sc, "");
                }
            }

            if (rc == CURLE_OK) {
                std::print(std::cerr, "{} - rc:{}  sc:{}", __func__, curl_easy_strerror(rc), sc);
            }
        }

        /// @brief Serializer to ostream for RESResponseType
        friend std::ostream& operator<<(std::ostream& os, const HttpRESTClient& src);
        friend void          to_json(nlohmann::json& dest, const HttpRESTClient& src);
    };

    inline void to_json(nlohmann::json& dest, const HttpRESTClient& src)
    {
        dest = nlohmann::json {{"UserAgent", src.UserAgent},
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
    using restcl = HttpRESTClient;
} // namespace siddiqsoft

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


#else
#pragma message("Windows required")
#endif


#endif // !RESTCLWINHTTP_HPP
