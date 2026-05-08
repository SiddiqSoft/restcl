/**
 * @file basic_restclient.hpp
 * @author Siddiq Software
 * @brief Abstract base class defining the REST client interface for synchronous and asynchronous HTTP operations.
 * @version 1.0
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 */
#pragma once
#include <optional>
#ifndef BASIC_RESTCLIENT_HPP
#define BASIC_RESTCLIENT_HPP

#include <tuple>
#include <functional>
#include <expected>
#include <atomic>

#include "http_frame.hpp"
#include "rest_request.hpp"
#include "rest_response.hpp"

namespace siddiqsoft
{
    struct rest_result_error;

    /// @brief The function or lambda must accept const rest_request& and const rest_response<>&
    using basic_callbacktype = std::function<void(rest_request<>&, std::expected<rest_response<>, int>)>;


    /// @brief Base class for the rest client
    /// @details Abstract interface defining the contract for REST client implementations.
    ///          Provides both synchronous and asynchronous HTTP operations with
    ///          platform-specific implementations (Unix/Linux via libcurl, Windows via WinHTTP).
    template <typename CharT = char>
    class basic_restclient
    {
    public:
        /**
         * @brief Configuration entry point for initializing platform-specific resources.
         *
         * @details This method is used for one-time initialization of SSL objects, connection pools,
         *          and other platform-specific shared resources. It should be called before any
         *          send() or sendAsync() operations.
         *
         * @param cfg JSON configuration object with optional settings:
         *            - "userAgent": User-Agent header string (default: "siddiqsoft.restcl/2")
         *            - "connectTimeout": Connection timeout in milliseconds (default: 0 = no timeout)
         *            - "timeout": Overall request timeout in milliseconds (default: 0 = no timeout)
         *            - "trace": Enable verbose tracing (default: false)
         *            - "freshConnect": Force new connections instead of reusing (default: false)
         *            - "verifyPeer": Verify SSL peer certificates (default: 1 = enabled)
         * @param cb Optional global callback function for async operations.
         *           If provided, this callback will be used for all sendAsync() calls
         *           that don't provide their own callback.
         * @return Reference to self to allow method chaining
         *
         * @note This is a pure virtual method that must be implemented by derived classes.
         * @note Configuration can be called multiple times to update settings.
         *
         * @example
         * @code
         * auto client = GetRESTClient();
         * client->configure({
         *     {"userAgent", "MyApp/1.0"},
         *     {"timeout", 5000},
         *     {"trace", false}
         * }, [](const auto& req, std::expected<rest_response<>, int> resp) {
         *     // Handle response
         * });
         * @endcode
         */
        virtual basic_restclient& configure(const nlohmann::json& = {}, basic_callbacktype&& = {}) = 0;

        /**
         * @brief Synchronous HTTP request execution.
         *
         * @details Sends an HTTP request to the remote server and waits for the response.
         *          This is a blocking operation that will not return until the response is received
         *          or an error occurs.
         *
         * @param req Reference to the rest_request object containing the HTTP request details
         *            (method, URI, headers, content, etc.)
         * @return std::expected<rest_response<CharT>, int> containing either:
         *         - A rest_response object with status code, headers, and content on success
         *         - An error code (platform-specific) on failure:
         *           * Windows: WinHTTP error codes (12001, 12002, 12029, etc.)
         *           * Unix/Linux: POSIX error codes (ECONNRESET, ENETUNREACH, etc.)
         *
         * @note The response object is only valid if has_value() returns true.
         * @note This method must be implemented by derived classes.
         * @note [[nodiscard]] attribute indicates the return value should not be ignored.
         *
         * @example
         * @code
         * rest_request<> req = "https://api.example.com/users"_GET;
         * auto result = client->send(req);
         * if (result.has_value()) {
         *     auto& resp = result.value();
         *     std::cout << "Status: " << resp.statusCode() << std::endl;
         * } else {
         *     std::cerr << "Error: " << result.error() << std::endl;
         * }
         * @endcode
         */
        [[nodiscard]] virtual std::expected<rest_response<CharT>, int> send(rest_request<>&) = 0;

        /**
         * @brief Asynchronous HTTP request execution.
         *
         * @details Queues an HTTP request for asynchronous processing. The request is executed
         *          in a background thread pool, and the provided callback is invoked when the
         *          response is received or an error occurs. This method returns immediately
         *          without waiting for the response.
         *
         * @param req Rvalue reference to the rest_request object. The request is moved into
         *            the async queue and ownership is transferred to the thread pool.
         * @param callback Optional callback function with signature:
         *                 void(rest_request<>&, std::expected<rest_response<>, int>)
         *                 If not provided, the global callback registered via configure() is used.
         *                 If neither is available, std::invalid_argument is thrown.
         * @return Reference to self to allow method chaining
         *
         * @throws std::invalid_argument if no callback is provided and none was registered via configure()
         * @throws std::runtime_error if the client is not properly initialized
         *
         * @note This method must be implemented by derived classes.
         * @note The callback is invoked from a worker thread; ensure thread-safe operations.
         * @note The request object is moved and should not be used after this call.
         * @note Multiple async requests can be queued and will be processed concurrently.
         *
         * @example
         * @code
         * rest_request<> req = "https://api.example.com/users"_POST;
         * req.setContent({{"name", "John"}});
         *
         * client->sendAsync(std::move(req), [](const auto& req, auto resp) {
         *     if (resp.has_value()) {
         *         std::cout << "Success: " << resp.value().statusCode() << std::endl;
         *     } else {
         *         std::cerr << "Error: " << resp.error() << std::endl;
         *     }
         * });
         * @endcode
         */
        virtual basic_restclient& sendAsync(rest_request<>&&, basic_callbacktype&& = {})          = 0;

        virtual basic_restclient& sendAsyncWithRetry(rest_request<>&&, basic_callbacktype&& = {}) = 0;

    protected:
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

    protected:
        static const uint32_t     READBUFFERSIZE {8192};
        static inline const char* RESTCL_ACCEPT_TYPES[4] {"application/json", "text/json", "*/*", NULL};
        bool                      isInitialized {false};
        uint32_t                  id = __COUNTER__;
        /// @brief Maximum number of retry attempts for failed deliveries
        static const auto RETRY_LIMIT {11};

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
    };

    /**
     * @brief Container to store the rest_request and the optional callback.
     *
     */
    template <typename CharT = char>
    struct RestPoolArgsType
    {
        RestPoolArgsType(rest_request<CharT>&& r, basic_callbacktype& cb)
            : request(std::move(r)) // own the request
            , callback(cb)          // make a copy
        {
        }

        RestPoolArgsType(rest_request<CharT>&& r, basic_callbacktype&& cb)
            : request(std::move(r))   // own the request
            , callback(std::move(cb)) // own the callback
        {
        }

        rest_request<CharT> request;
        basic_callbacktype  callback {};
    };

} // namespace siddiqsoft


#endif // !BASIC_RESTCLIENT_HPP
