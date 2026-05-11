/**
 * @file restcl_win.hpp
 * @author Abdulkareem Siddiq (github@siddiqsoft.com)
 * @brief WinHTTP based implementation of the basic_restclient
 * @version
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 */

#pragma once
#if (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))

#ifndef RESTCL_WIN_HPP
#define RESTCL_WIN_HPP

#include <iostream>
#include <chrono>
#include <string>
#include <functional>
#include <memory>
#include <format>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <deque>
#include <semaphore>
#include <stop_token>
#include <print>

#include "siddiqsoft/GenProcessInfo.hpp"

#include <windows.h>

#if !_WINHTTPX_
#include <winhttp.h>
#else
#pragma message("--- " __FILE__ "--- Skip include winhttp..")
#endif
#pragma comment(lib, "winhttp")


#include "nlohmann/json.hpp"

#include "http_frame.hpp"
#include "basic_restclient.hpp"
#include "rest_request.hpp"
#include "rest_response.hpp"

#include "siddiqsoft/timethis.hpp"
#include "siddiqsoft/string2map.hpp"
#include "siddiqsoft/acw32h.hpp"
#include "siddiqsoft/conversion-utils.hpp"

#include "siddiqsoft/simple_pool.hpp"

namespace siddiqsoft
{
#pragma region WinInet error code map
    static std::map<uint32_t, std::string_view> WinInetErrorCodes {
            {12001, std::string_view("ERROR_INTERNET_OUT_OF_HANDLES: No more handles could be generated at this time.")},
            {12002, std::string_view("ERROR_INTERNET_TIMEOUT: The request has timed out.")},
            {12003,
             std::string_view("ERROR_INTERNET_EXTENDED_ERROR: An extended error was returned from the server. This is typically a "
                              "string or buffer "
                              "containing a verbose error message. Call InternetGetLastResponseInfo to retrieve the error text.")},
            {12004, std::string_view("ERROR_INTERNET_INTERNAL_ERROR: An internal error has occurred.")},
            {12005, std::string_view("ERROR_INTERNET_INVALID_URL:The URL is invalid.")},
            {12006,
             std::string_view("ERROR_INTERNET_UNRECOGNIZED_SCHEME: The URL scheme could not be recognized or is not supported.")},
            {12007, std::string_view("ERROR_INTERNET_NAME_NOT_RESOLVED: The server name could not be resolved.")},
            {12008, std::string_view("ERROR_INTERNET_PROTOCOL_NOT_FOUND: The requested protocol could not be located.")},
            {12009,
             std::string_view("ERROR_INTERNET_INVALID_OPTION: A request to InternetQueryOption or InternetSetOption specified an "
                              "invalid option "
                              "value.")},
            {12010,
             std::string_view("ERROR_INTERNET_BAD_OPTION_LENGTH: The length of an option supplied to InternetQueryOption or "
                              "InternetSetOption is "
                              "incorrect for the type of option specified.")},
            {12011, std::string_view("ERROR_INTERNET_OPTION_NOT_SETTABLE: The request option cannot be set, only queried.")},
            {12012,
             std::string_view("ERROR_INTERNET_SHUTDOWN: The Win32 Internet function support is being shut down or unloaded.")},
            {12013,
             std::string_view("ERROR_INTERNET_INCORRECT_USER_NAME: The request to connect and log on to an FTP server could not be "
                              "completed "
                              "because the supplied user name is incorrect.")},
            {12014,
             std::string_view("ERROR_INTERNET_INCORRECT_PASSWORD: The request to connect and log on to an FTP server could not be "
                              "completed because "
                              "the supplied password is incorrect.")},
            {12015,
             std::string_view("ERROR_INTERNET_LOGIN_FAILURE: The request to connect to and log on to an FTP server failed.")},
            {12016, std::string_view("ERROR_INTERNET_INVALID_OPERATION: The requested operation is invalid.")},
            {12017,
             std::string_view("ERROR_INTERNET_OPERATION_CANCELLED: The operation was canceled, usually because the handle on which "
                              "the request was "
                              "operating was closed before the operation completed.")},
            {12018,
             std::string_view(
                     "ERROR_INTERNET_INCORRECT_HANDLE_TYPE: The type of handle supplied is incorrect for this operation.")},
            {12019,
             std::string_view("ERROR_INTERNET_INCORRECT_HANDLE_STATE: The requested operation cannot be carried out because the "
                              "handle supplied is "
                              "not in the correct state.")},
            {12020, std::string_view("ERROR_INTERNET_NOT_PROXY_REQUEST: The request cannot be made via a proxy.")},
            {12021, std::string_view("ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND: A required registry value could not be located.")},
            {12022,
             std::string_view("ERROR_INTERNET_BAD_REGISTRY_PARAMETER: A required registry value was located but is an incorrect "
                              "type or has an "
                              "invalid value.")},
            {12023, std::string_view("ERROR_INTERNET_NO_DIRECT_ACCESS: Direct network access cannot be made at this time.")},
            {12024,
             std::string_view("ERROR_INTERNET_NO_CONTEXT: An asynchronous request could not be made because a zero context value "
                              "was supplied.")},
            {12025,
             std::string_view("ERROR_INTERNET_NO_CALLBACK: An asynchronous request could not be made because a callback function "
                              "has not been set.")},
            {12026,
             std::string_view("ERROR_INTERNET_REQUEST_PENDING: The required operation could not be completed because one or more "
                              "requests are "
                              "pending.")},
            {12027, std::string_view("ERROR_INTERNET_INCORRECT_FORMAT: The format of the request is invalid.")},
            {12028, std::string_view("ERROR_INTERNET_ITEM_NOT_FOUND: The requested item could not be located.")},
            {12029, std::string_view("ERROR_INTERNET_CANNOT_CONNECT: The attempt to connect to the server failed.")},
            {12030, std::string_view("ERROR_INTERNET_CONNECTION_ABORTED: The connection with the server has been terminated.")},
            {12031, std::string_view("ERROR_INTERNET_CONNECTION_RESET: The connection with the server has been reset.")},
            {12032, std::string_view("ERROR_INTERNET_FORCE_RETRY: Calls for the Win32 Internet function to redo the request.")},
            {12033, std::string_view("ERROR_INTERNET_INVALID_PROXY_REQUEST: The request to the proxy was invalid.")},
            {12036, std::string_view("ERROR_INTERNET_HANDLE_EXISTS: The request failed because the handle already exists.")},
            {12037,
             std::string_view("ERROR_INTERNET_SEC_CERT_DATE_INVALID: SSL certificate date that was received from the server is "
                              "bad. The certificate "
                              "is expired.")},
            {12038,
             std::string_view("ERROR_INTERNET_SEC_CERT_CN_INVALID: SSL certificate common name (host name field) is incorrect. For "
                              "example, if you "
                              "entered www.server.com and the common name on the certificate says www.different.com.")},
            {12039,
             std::string_view("ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR: The application is moving from a non-SSL to an SSL "
                              "connection because of a "
                              "redirect.")},
            {12040,
             std::string_view("ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR: The application is moving from an SSL to an non-SSL "
                              "connection because of a "
                              "redirect.")},
            {12041,
             std::string_view("ERROR_INTERNET_MIXED_SECURITY: Indicates that the content is not entirely secure. Some of the "
                              "content being viewed "
                              "may have come from unsecured servers.")},
            {12042,
             std::string_view("ERROR_INTERNET_CHG_POST_IS_NON_SECURE: The application is posting and attempting to change multiple "
                              "lines of text on "
                              "a server that is not secure.")},
            {12043,
             std::string_view(
                     "ERROR_INTERNET_POST_IS_NON_SECURE: The application is posting data to a server that is not secure.")},
            {12044, std::string_view("ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED: Client certificate is needed.")},
            {12110,
             std::string_view("ERROR_FTP_TRANSFER_IN_PROGRESS: The requested operation cannot be made on the FTP session "
                              "handle because an "
                              "operation is already in progress.")},
            {12111, std::string_view("ERROR_FTP_DROPPED: The FTP operation was not completed because the session was aborted.")},
            {12130,
             std::string_view(
                     "ERROR_GOPHER_PROTOCOL_ERROR: An error was detected while parsing data returned from the gopher server.")},
            {12131, std::string_view("ERROR_GOPHER_NOT_FILE: The request must be made for a file locator.")},
            {12132,
             std::string_view("ERROR_GOPHER_DATA_ERROR: An error was detected while receiving data from the gopher server.")},
            {12133, std::string_view("ERROR_GOPHER_END_OF_DATA: The end of the data has been reached.")},
            {12134, std::string_view("ERROR_GOPHER_INVALID_LOCATOR: The supplied locator is not valid.")},
            {12135,
             std::string_view("ERROR_GOPHER_INCORRECT_LOCATOR_TYPE: The type of the locator is not correct for this operation.")},
            {12136,
             std::string_view("ERROR_GOPHER_NOT_GOPHER_PLUS: The requested operation can only be made against a Gopher+ server or "
                              "with a locator "
                              "that specifies a Gopher+ operation.")},
            {12137, std::string_view("ERROR_GOPHER_ATTRIBUTE_NOT_FOUND: The requested attribute could not be located.")},
            {12138, std::string_view("ERROR_GOPHER_UNKNOWN_LOCATOR: The locator type is unknown.")},
            {12150, std::string_view("ERROR_HTTP_HEADER_NOT_FOUND: The requested header could not be located.")},
            {12151, std::string_view("ERROR_HTTP_DOWNLEVEL_SERVER: The server did not return any headers.")},
            {12152, std::string_view("ERROR_HTTP_INVALID_SERVER_RESPONSE: The server response could not be parsed.")},
            {12153, std::string_view("ERROR_HTTP_INVALID_HEADER: The supplied header is invalid.")},
            {12154, std::string_view("ERROR_HTTP_INVALID_QUERY_REQUEST: The request made to HttpQueryInfo is invalid.")},
            {12155, std::string_view("ERROR_HTTP_HEADER_ALREADY_EXISTS: The header could not be added because it already exists.")},
            {12156,
             std::string_view("ERROR_HTTP_REDIRECT_FAILED: The redirection failed because either the scheme changed (for example, "
                              "HTTP to FTP) or "
                              "all attempts made to redirect failed (default is five attempts). ")},
            {12175, std::string_view("ERROR_WINHTTP_SECURE_FAILURE: Port specification might be invalid.")}};

    static std::string messageFromWininetCode(uint32_t errCode)
    {
        return WinInetErrorCodes.find(errCode) != WinInetErrorCodes.end()
                     ? std::format("{}-{}", errCode, WinInetErrorCodes[errCode])
                     : std::to_string(errCode);
    }
#pragma endregion

    struct rest_result_error
    {
        uint32_t error {0};

        rest_result_error(const uint32_t ec)
            : error(ec)
        {
        }
             operator std::string() { return messageFromWininetCode(error); }
        auto to_string() const { return messageFromWininetCode(error); }
    };

    /// @brief Windows implementation of the basic_restclient using WinHTTP.
    ///
    /// @details Provides HTTP client functionality for Windows platforms using WinHTTP.
    ///          Features include:
    ///          - Native Windows HTTP API integration
    ///          - Support for all HTTP methods and protocols (HTTP/1.0, HTTP/1.1, HTTP/2)
    ///          - Automatic header and content handling
    ///          - Thread-safe operations via simple_pool
    ///          - Synchronous and asynchronous request execution
    ///          - Comprehensive error handling with WinHTTP error codes
    ///
    /// @note This class is only compiled on Windows platforms
    /// @note Uses WinHTTP API for HTTP operations
    /// @note Thread-safe for concurrent async operations via simple_pool
    class WinHttpRESTClient : public basic_restclient<char>
    {
    public:
        std::string  UserAgent {"siddiqsoft.restcl/2"};
        std::wstring UserAgentW {L"siddiqsoft.restcl/2"};

    protected:
        /// @brief Shared session for the entire class. This is also used by the threadpool as it send()s the data.
        ACW32HINTERNET hSession {};
        uint32_t       id = __COUNTER__;

        basic_callbacktype _callback {};

        nlohmann::json _config {{"userAgent", "siddiqsoft.restcl/2"},
                                {RESTCL_CONFIG_TRACE, false},
                                {"id", id},
                                {"connectTimeout", 0L},
                                {"timeout", 0L},
                                {"downloadDirectory", nullptr},
                                {"headers", nullptr}};


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

        /// @brief Implements a threadpool that supports invoking a REST call until success
        siddiqsoft::simple_pool<RestPoolArgsType<char>> pool {[&](RestPoolArgsType<char>&& arg) -> void {
            // This function is invoked any time we have an item
            // The arg is moved here and belongs to use. Once this
            // method completes the lifetime of the object ends;
            // typically this is *after* we invoke the callback.
            // The logic here is to invoke the REST request until we get a
            // success response.
            uint16_t retryCount = 0;
            uint16_t failCount  = 0;

            if (arg.retryCounter == 0) {
                arg.retryCounter = _config[RESTCL_CONFIG_AUTO_REST_RETRY_COUNTER];
                if (arg.retryCounter > MAX_AUTO_RETRY_SEND_LIMIT) arg.retryCounter = MAX_AUTO_RETRY_SEND_LIMIT;
            }

            while (arg.retryCounter--) {
#if defined(DEBUG)
                std::print(std::cerr,
                           "pool io callback - Sending.. retryCount:{}  arg.retryCount:{} \n",
                           retryCount,
                           arg.retryCounter);
                if (retryCount > 0) arg.request.setHeader("X-restcl-Retry", retryCount);
                if (failCount > 0) arg.request.setHeader("X-restcl-FailCount", failCount);
#endif
                timethis ttx {};

                if (auto resp = send(arg.request); resp && resp->success()) {
                    // Only add the header if we "Retry".. we should not add for the first attempt if it succeeds.
                    if (retryCount > 0) resp->setHeader("X-restcl-Retry", retryCount);
                    if (failCount > 0) resp->setHeader("X-restcl-FailCount", failCount);

#if defined(DEBUG)
                    auto debugLine = std::format("callback#:{}/{}, retry:{}/{}",
                                                 callbackAttempt.load(),
                                                 callbackCompleted.load(),
                                                 arg.retryCounter,
                                                 MAX_AUTO_RETRY_SEND_LIMIT);
                    resp->setHeader("X-restcl-pool-debug", debugLine);
                    resp->setHeader("X-restcl-io-ttx", ttx.lap<std::chrono::milliseconds>());
#endif

                    try {
                        dispatchCallback(arg.callback, arg.request, resp);
                    }
                    catch (std::system_error& se) {
                        // Failed; dispatch anyways and let the client figure out the issue.
                        std::print(std::cerr,
                                   "simple_pool - processing {} pool handler \\033[48;5;1m got exception: {}\n",
                                   callbackAttempt.load(),
                                   se.what());
                        dispatchCallback(arg.callback, arg.request, std::unexpected<int>(se.code().value()));
                    }
                    catch (std::exception& ex) {
                        callbackFailed++;
                        std::print(std::cerr,
                                   "simple_pool - processing {} pool handler \\033[48;5;1m got exception: {}\n",
                                   callbackAttempt.load(),
                                   ex.what());
                    }

                    // We're done with this.. break out..
                    break;
                } // if send competed.
                else {
                    failCount++;
#if defined(DEBUG)
                    auto debugLine = std::format("simple_pool - IO Failed#:{}, retry:{}/{} ! ! ! ! ! !",
                                                 failCount,
                                                 arg.retryCounter,
                                                 MAX_AUTO_RETRY_SEND_LIMIT);
                    std::println(std::cerr, "{}\n{}", debugLine, resp ? nlohmann::json(*resp).dump(2) : "N/A");
                    if (resp) {
                        resp->setHeader("X-restcl-pool-debug", debugLine);
                        resp->setHeader("X-restcl-io-ttx", ttx.lap<std::chrono::milliseconds>());
                    }
#endif
                    // Inform the client of the failure by invoking the registred callback..
                    dispatchCallback(arg.callback, arg.request, std::unexpected<int>(0));
                }

                // Increment the retry here
                ++retryCount;

            } // while

#if defined(DEBUG)
            std::print(std::cerr,
                       "simple_pool - COMPLETED IO with callback#:{}/{}, retry:{}/{}",
                       callbackAttempt.load(),
                       callbackCompleted.load(),
                       arg.retryCounter,
                       MAX_AUTO_RETRY_SEND_LIMIT);
#endif
        }};


    protected:
        WinHttpRESTClient(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {})
        {
            configure(cfg, std::forward<basic_callbacktype&&>(cb));
        }

    public:
        ~WinHttpRESTClient() { }

    public:
        WinHttpRESTClient(const WinHttpRESTClient&)            = delete;
        WinHttpRESTClient& operator=(const WinHttpRESTClient&) = delete;

        /// @brief Configures the WinHTTP client with platform-specific settings.
        ///
        /// @details Initializes the WinHTTP session handle and applies configuration options such as
        ///          HTTP/2 protocol support, automatic decompression, and user agent settings.
        ///          This method should be called once before any send() or sendAsync() operations.
        ///
        /// @param cfg JSON configuration object with optional settings:
        ///            - "userAgent": User-Agent header string (default: "siddiqsoft.restcl/2")
        ///            - "connectTimeout": Connection timeout in milliseconds (default: 0 = no timeout)
        ///            - "timeout": Overall request timeout in milliseconds (default: 0 = no timeout)
        ///            - RESTCL_CONFIG_TRACE: Enable verbose tracing (default: false)
        ///            - "headers": Additional headers to include in requests
        /// @param cb Optional global callback function for async operations.
        ///           If provided, this callback will be used for all sendAsync() calls
        ///           that don't provide their own callback.
        /// @return Reference to self to allow method chaining
        ///
        /// @details The method performs the following initialization steps:
        ///          1. Updates internal configuration with provided settings
        ///          2. Sets the global callback if provided
        ///          3. Extracts and converts the User-Agent string to wide character format
        ///          4. Creates a WinHTTP session handle if not already created
        ///          5. Enables HTTP/2 protocol support on the session
        ///          6. Enables automatic decompression for response bodies
        ///
        /// @note WinHTTP session handles are reused across multiple requests for efficiency.
        /// @note HTTP/2 and decompression are enabled by default if supported by the system.
        /// @note Configuration can be called multiple times to update settings.
        ///
        /// @example
        /// @code
        /// auto client = WinHttpRESTClient::CreateInstance();
        /// client->configure({
        ///     {"userAgent", "MyApp/1.0"},
        ///     {"timeout", 5000},
        ///     {RESTCL_CONFIG_TRACE, false}
        /// }, [](const auto& req, std::expected<rest_response<>, int> resp) {
        ///     // Handle response
        /// });
        /// @endcode
        basic_restclient<char>& configure(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {}) override
        {
            if (!cfg.is_null() && !cfg.empty()) _config.update(cfg);
            if (cb) _callback = std::move(cb);

            UserAgent  = _config.value("userAgent", _config.value("/headers/User-Agent"_json_pointer, ""));
            UserAgentW = ConversionUtils::convert_to<char, wchar_t>(UserAgent);

            if (hSession == NULL) {
#if defined(DEBUG)
                std::print(std::cerr, "{} Calling WinHttpOpen....tid:{}\n", __func__, ::GetCurrentThreadId());
#endif
                hSession = std::move(WinHttpOpen(UserAgentW.c_str(), WINHTTP_ACCESS_TYPE_NO_PROXY, NULL, NULL, 0));
                if (hSession) {
                    const DWORD enableHTTP2Flag = WINHTTP_PROTOCOL_FLAG_HTTP2;
                    const DWORD decompression   = WINHTTP_DECOMPRESSION_FLAG_ALL;

                    // We're initialized; the rest is optional.
                    isInitialized = true;

#if defined(DEBUG)
                    std::print(std::cerr, "{} Successful WinHttpOpen....\n", __func__);
#endif
                    // Enable HTTP/2 protocol
                    if (!WinHttpSetOption(
                                hSession, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, (LPVOID)&enableHTTP2Flag, sizeof(enableHTTP2Flag)))
                    {
#if defined(DEBUG)
                        std::print(std::cerr, "{} Failed set HTTP/2 flag; err:{}\n", __func__, GetLastError());
#endif
                    }

                    // Enable decompression
                    if (!WinHttpSetOption(hSession, WINHTTP_OPTION_DECOMPRESSION, (LPVOID)&decompression, sizeof(decompression))) {
#ifdef _DEBUG
                        std::print(std::cerr, "{} Failed set decompression flag; err:{}\n", __func__, GetLastError());
#endif
                    }
                }
            }

#if defined(DEBUG)
            std::print(std::cerr, "{} Completed threadid:{}\n", __func__, siddiqsoft::GenProcessInfo::GetThreadId());
#endif
            return *this;
        }


        /// @brief Move constructor. We have the object hSession which must be transferred to our instance.
        /// @param src Source object is "cleared"
        WinHttpRESTClient(WinHttpRESTClient&& src) noexcept
            : hSession(std::move(src.hSession))
            , _config(src._config)
            , UserAgent(src.UserAgent)
            , UserAgentW(src.UserAgentW)
            , _callback(std::move(src._callback))
        {
        }

        /// @brief Implements an asynchronous invocation of the send() method.
        ///
        /// @details Queues an HTTP request for asynchronous processing using the internal thread pool.
        ///          The request is executed in a background worker thread, and the provided callback
        ///          is invoked when the response is received or an error occurs. This method returns
        ///          immediately without waiting for the response.
        ///
        /// @param req Rvalue reference to the rest_request object. The request is moved into
        ///            the async queue and ownership is transferred to the thread pool.
        /// @param callback Optional callback function with signature:
        ///                 void(rest_request<>&, std::expected<rest_response<>, int>)
        ///                 If not provided, the global callback registered via configure() is used.
        ///                 If neither is available, std::invalid_argument is thrown.
        /// @param retryCount When this value is greater than 1 and limited to MAX_AUTO_RETRY_SEND_LIMIT,
        ///                   the request will be automatically retried until a success response is received.
        ///                   Default is 0, which uses the configured auto-retry counter.
        /// @return Reference to self to allow method chaining
        ///
        /// @throws std::invalid_argument if no callback is provided and none was registered via configure()
        /// @throws std::runtime_error if the client is not properly initialized
        ///
        /// @details The method performs the following operations:
        ///          1. Validates that a callback is available (either provided or configured)
        ///          2. Checks that the client is properly initialized
        ///          3. Normalizes the retry count to not exceed MAX_AUTO_RETRY_SEND_LIMIT
        ///          4. Queues the request to the internal thread pool for processing
        ///          5. Returns immediately to the caller
        ///
        /// @note The callback is invoked from a worker thread; ensure thread-safe operations.
        /// @note The request object is moved and should not be used after this call.
        /// @note Multiple async requests can be queued and will be processed concurrently by the pool.
        /// @note Retry logic will automatically retry failed requests up to the specified count.
        /// @note Each retry attempt includes X-restcl-Retry and X-restcl-FailCount headers for tracking.
        ///
        /// @example
        /// @code
        /// rest_request<> req = "https://api.example.com/users"_POST;
        /// req.setContent({{"name", "John"}});
        ///
        /// client->sendAsync(std::move(req), [](const auto& req, auto resp) {
        ///     if (resp.has_value()) {
        ///         std::cout << "Success: " << resp.value().statusCode() << std::endl;
        ///     } else {
        ///         std::cerr << "Error: " << resp.error() << std::endl;
        ///     }
        /// }, 3); // Retry up to 3 times on failure
        /// @endcode
        basic_restclient& sendAsync(rest_request<>&& req, basic_callbacktype&& callback = {}, uint16_t retryCount = 0) override
        {
            if (!_callback && !callback)
                throw std::invalid_argument("Async operation requires you to handle the response; register callback via "
                                            "configure() or provide callback at point of invocation.");

            if (!isInitialized) throw std::runtime_error("Initialization failed/incomplete!");

            if (retryCount == 0) retryCount = _config[RESTCL_CONFIG_AUTO_REST_RETRY_COUNTER];
            // If the user provides a value of greater than 1 the we retry as many times as asked
            retryCount = (retryCount >= MAX_AUTO_RETRY_SEND_LIMIT) ? MAX_AUTO_RETRY_SEND_LIMIT : retryCount;

            req.setHeader("X-restcl-maxAutoRetryCount", MAX_AUTO_RETRY_SEND_LIMIT);

            // Queue the request.. the queue worker will handle the retry based on our
            pool.queue(RestPoolArgsType {std::move(req), callback ? std::move(callback) : _callback, retryCount});

            return *this;
        }


        /// @brief Implements a synchronous send of the request.
        /// @param req Request object
        /// @return Response object only if the callback is not provided to emulate synchronous invocation
        [[nodiscard]] std::expected<rest_response<>, int> send(rest_request<char>& req)
        {
            rest_response<char> resp {};

            /// @brief Lambda to Parse the first line from the HTTP response into its parts: version, status and the reason
            /// phrase
            /// @param src The buffer as wstring
            /// @return A tuple with httpVersion, statusCode, reasonPhrase and the last is the offset to the start of the header
            /// section just past the end of the response line.
            auto extractResponseLine = [](const std::wstring& src) -> std::tuple<std::string, uint32_t, std::string, size_t> {
                // Given the first line is of the format: `HTTP_VERSION STATUS_CODE REASON_PHRASE\r\n`
                // Return the particles.

                // TODO: Optimize..
                auto firstPart    = src.find_first_of(L' ');
                auto httpVersion  = src.substr(0, firstPart);
                auto secondPart   = src.find_first_of(L' ', ++firstPart);
                auto statusCode   = src.substr(firstPart, secondPart - firstPart);
                auto lastPart     = src.find_first_of(L"\r\n", ++secondPart);
                auto reasonPhrase = src.substr(secondPart, lastPart - secondPart);

                return {ConversionUtils::convert_to<wchar_t, char>(httpVersion),
                        std::stoi(statusCode),
                        ConversionUtils::convert_to<wchar_t, char>(reasonPhrase),
                        lastPart + 2}; // start of the header section
            };

            HRESULT  hr {E_FAIL};
            DWORD    dwBytesRead {0}, dwError {0};
            uint32_t nRetry {0}, nError {0};
            char     cBuf[READBUFFERSIZE] {};
            DWORD    dwFlagsSize = 0;

            // First order - adjust the UserAgent
            if (!req.getHeaders().contains("User-Agent")) req.getHeaders()["User-Agent"] = UserAgent;
            auto strUserAgent =
                    req.getHeaders().contains("User-Agent") ? req.getHeaders().value("User-Agent", UserAgent) : UserAgent;

            if (hSession != NULL) {
                auto& strServer = req.uri.authority.host;
                if (ACW32HINTERNET hConnect {WinHttpConnect(
                            hSession, ConversionUtils::convert_to<char, wchar_t>(strServer).c_str(), req.uri.authority.port, 0)};
                    hConnect != NULL)
                {
                    std::string strMethod  = to_string(req.getMethod());
                    auto        strUrl     = req.uri.urlPart;
                    std::string strVersion = to_string(req.getProtocol());

                    if (ACW32HINTERNET hRequest {WinHttpOpenRequest(hConnect,
                                                                    ConversionUtils::convert_to<char, wchar_t>(strMethod).c_str(),
                                                                    ConversionUtils::convert_to<char, wchar_t>(strUrl).c_str(),
                                                                    ConversionUtils::convert_to<char, wchar_t>(strVersion).c_str(),
                                                                    NULL,
                                                                    RESTCL_ACCEPT_TYPES_W,
                                                                    (req.uri.scheme == UriScheme::WebHttps)
                                                                            ? WINHTTP_FLAG_SECURE | WINHTTP_FLAG_REFRESH
                                                                            : WINHTTP_FLAG_REFRESH)};
                        hRequest != NULL)
                    {
                        auto         contentLength  = req.getHeaders().value("Content-Length", 0);
                        std::wstring requestHeaders = ConversionUtils::convert_to<char, wchar_t>(req.encodeHeaders());
                        nError                      = WinHttpAddRequestHeaders(hRequest,
                                                                               requestHeaders.c_str(),
                                                                               static_cast<DWORD>(requestHeaders.length()),
                                                                               WINHTTP_ADDREQ_FLAG_ADD);

                        dwError                     = ERROR_SUCCESS;
                        // Send the request
                        // Note: use getContentBody() which returns a reference to avoid
                        // dangling pointer from encodeContent() which returns a temporary copy.
                        auto& contentBody = req.getContentBody();
                        nError            = WinHttpSendRequest(hRequest,
                                                               WINHTTP_NO_ADDITIONAL_HEADERS,
                                                               0,
                                                               contentLength > 0 ? LPVOID(contentBody.c_str()) : NULL,
                                                               contentLength,
                                                               contentLength,
                                                               NULL);

                        if (nError == FALSE && (dwError = GetLastError()) == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED) {
                            nError = WinHttpSetOption(
                                    hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);
                            if (nError == TRUE) {
                                nError = WinHttpSendRequest(hRequest,
                                                            WINHTTP_NO_ADDITIONAL_HEADERS,
                                                            0,
                                                            contentLength > 0 ? LPVOID(contentBody.c_str()) : NULL,
                                                            contentLength,
                                                            contentLength,
                                                            NULL);
                            }
                        }

                        // *************
                        // Receive phase
                        // *************
                        // Get the "response" and the headers..
                        if (nError == FALSE) {
                            dwError = GetLastError();
                        }
                        else {
                            // Signals we should finish the request and wait for response from server.
                            nError = WinHttpReceiveResponse(hRequest, NULL);
                            if (nError == FALSE) {
                                dwError = GetLastError();
                            }
                            else {
                                // Get the headers next.. including the response line
                                DWORD dwSize = 0;

                                // First, use WinHttpQueryHeaders to obtain the size of the buffer.
                                WinHttpQueryHeaders(hRequest,
                                                    WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                                    WINHTTP_HEADER_NAME_BY_INDEX,
                                                    NULL,
                                                    &dwSize,
                                                    WINHTTP_NO_HEADER_INDEX);

                                // Allocate memory for the buffer.
                                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                                    // A silly decision by the library to always return the headers as wchar_t
                                    // despite the fact that over the wire we've got UTF-8/ASCII encoded headers
                                    // The body is where encode varies and here in our library we focus on utf-8 json.
                                    // The buffer must be wchar_t and we must adjust the size to account for the
                                    // actual number of characters since the dwSize is in bytes not wchar_t characters!
                                    auto lpOutBuffer = std::make_unique<wchar_t[]>(dwSize);

                                    // Now, use WinHttpQueryHeaders to retrieve the header.
                                    nError = WinHttpQueryHeaders(hRequest,
                                                                 WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                                                 WINHTTP_HEADER_NAME_BY_INDEX,
                                                                 lpOutBuffer.get(),
                                                                 &dwSize,
                                                                 WINHTTP_NO_HEADER_INDEX);
                                    if (nError == TRUE && (lpOutBuffer != nullptr)) {
                                        // The data is wchar_t but the dwSize returns bytes; adjustmend required.
                                        dwSize /= sizeof(wchar_t);
                                        // We need to skip the first line of the respose as it is the status response line.
                                        // Decode the CRLF string into key-value elements.
                                        std::wstring src {lpOutBuffer.get(), dwSize};

                                        auto [ver, status, reason, startOfHeaders] = extractResponseLine(src);
                                        src.erase(0, startOfHeaders);
                                        resp.setStatus(status, reason);
                                        resp.setProtocol(ver);

                                        // Parse the response into json object..
                                        // Extract the heads into a map<string,string> where the source is wstring and the
                                        // output is string This is then fed to the json which will create a headers object.
                                        resp.setHeaders(
                                                string2map::parse<std::wstring, std::string, std::map<std::string, std::string>>(
                                                        src, L": ", L"\r\n"));
                                    }
                                }
                            }
                        }

                        // Next stage is to check for any errors and if none, get the body
                        if (dwError == ERROR_WINHTTP_NAME_NOT_RESOLVED) {
                            return std::unexpected(static_cast<int>(dwError));
                        }
                        else if ((dwError == ERROR_WINHTTP_CANNOT_CONNECT) || (dwError == ERROR_WINHTTP_CONNECTION_ERROR) ||
                                 (dwError == ERROR_WINHTTP_OPERATION_CANCELLED) || (dwError == ERROR_WINHTTP_LOGIN_FAILURE) ||
                                 (dwError == ERROR_WINHTTP_INVALID_SERVER_RESPONSE) || (dwError == ERROR_WINHTTP_RESEND_REQUEST) ||
                                 (dwError == ERROR_WINHTTP_SECURE_FAILURE) || (dwError == ERROR_WINHTTP_TIMEOUT))
                        {
                            return std::unexpected {static_cast<int>(dwError)};
                        }
                        else if (dwError == ERROR_WINHTTP_INVALID_URL) {
                            return std::unexpected {static_cast<int>(dwError)};
                        }
                        else if (dwError != ERROR_FILE_NOT_FOUND) {
                            nRetry = 0;
                            // while (dwError == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED)
                            //{
                            //	nError = WinHttpSetOption(
                            //			hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);
                            //
                            //	dwError = ERROR_SUCCESS;
                            //	nError  = WinHttpSendRequest(hRequest,
                            //  !requestHeaders.empty() ? requestHeaders.c_str() : NULL,
                            //  -1,
                            //  argBody.has_value() ? (void*)argBody.value_or("").c_str() : NULL,
                            //  DWORD(argBody.value_or("").length()),
                            //  DWORD(argBody.value_or("").length()),
                            //  NULL);
                            //	dwError = GetLastError();
                            //
                            //	nRetry++;
                            //
                            //	if (nRetry > HTTPS_MAXRETRY) break;
                            //}

                            std::string rawResponse {};

                            do {
                                // Returns byte stream; accumulate until we're out of data.
                                hr = WinHttpReadData(hRequest, cBuf, READBUFFERSIZE - 1, &dwBytesRead)
                                           ? S_OK
                                           : HRESULT_FROM_WIN32(GetLastError());
                                if (dwBytesRead) {
                                    cBuf[dwBytesRead] = '\0';
                                    rawResponse.append(cBuf, dwBytesRead);
                                }
                            } while (dwBytesRead > 0);

                            hr = S_OK;
                            resp.setContent(rawResponse);

                            // Invoke the callback
                            return resp;
                        }
                        else {
                            return std::unexpected {static_cast<int>(dwError)};
                        }
                    }
                    else {
                        return std::unexpected {static_cast<int>(hr)};
                    }
                }
                else {
                    return std::unexpected {static_cast<int>(hr)};
                }
            }
            else {
                return std::unexpected {static_cast<int>(hr)};
            }
        }

    public:
        [[nodiscard]] static auto CreateInstance(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {})
                -> std::shared_ptr<WinHttpRESTClient>
        {
            std::shared_ptr<WinHttpRESTClient> rcl(new WinHttpRESTClient(cfg, std::forward<basic_callbacktype&&>(cb)));
#if defined(DEBUG) || defined(_DEBUG)
            std::print(std::cerr, "{} - New WinHttpRESTClient Instance..id:{}\n", __FUNCTION__, rcl->id);
#endif
            return rcl;
        }
    };

    using restcl = std::shared_ptr<WinHttpRESTClient>;
} // namespace siddiqsoft


template <>
struct std::formatter<siddiqsoft::rest_result_error> : std::formatter<std::string>
{
    template <class FC>
    auto format(const siddiqsoft::rest_result_error& rrc, FC& ctx) const
    {
        return std::formatter<std::string>::format(rrc.to_string(), ctx);
    }
};

#else
#pragma message("Windows required")
#endif

#endif // !RESTCLWINHTTP_HPP
