/**
 * @file restcl.hpp
 * @author Siddiq Software
 * @brief Main public header for the restcl REST client library.
 * @version 1.0
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 * This is the primary entry point for the restcl library. It provides platform-specific
 * implementations:
 * - Unix/Linux/macOS: HttpRESTClient using libcurl
 * - Windows: WinHttpRESTClient using WinHTTP
 *
 * @example
 * @code
 * using namespace siddiqsoft::restcl_literals;
 * auto client = siddiqsoft::GetRESTClient();
 * auto resp = client->send("https://api.example.com/users"_GET);
 * @endcode
 */
#pragma once
#ifndef RESTCL_HPP
#define RESTCL_HPP

#include "private/basic_restclient.hpp"

#if defined(__linux__) || defined(__APPLE__)
#include "private/restcl_unix.hpp"
#elif (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))
#include "private/restcl_win.hpp"
#else
#error "Platform not supported"
#endif

namespace siddiqsoft
{
    /**
     * @brief Factory function to obtain a platform-specific REST client instance.
     *
     * Creates and returns a REST client appropriate for the current platform:
     * - Unix/Linux/macOS: HttpRESTClient (libcurl-based)
     * - Windows: WinHttpRESTClient (WinHTTP-based)
     *
     * @param cfg Optional JSON configuration object with keys:
     *            - "userAgent": User-Agent header string (default: "siddiqsoft.restcl/2")
     *            - "trace": Enable verbose logging (default: false)
     *            - "connectTimeout": Connection timeout in milliseconds (default: 0)
     *            - "timeout": Overall request timeout in milliseconds (default: 0)
     *            - "verifyPeer": SSL peer verification (default: 1)
     *            - "freshConnect": Force new connections (default: false)
     *
     * @param cb Optional global callback for async operations. Can be overridden per-request.
     *           Signature: void(rest_request<>&, std::expected<rest_response<>, int>)
     *
     * @return Shared pointer to platform-specific REST client implementation.
     *         - Unix/Linux/macOS: std::shared_ptr<HttpRESTClient>
     *         - Windows: std::shared_ptr<WinHttpRESTClient>
     *
     * @throws std::runtime_error if platform initialization fails
     *
     * @see basic_restclient for interface documentation
     * @see rest_request for request building
     * @see rest_response for response handling
     */
    [[nodiscard]] static auto GetRESTClient(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {})
    {
#if defined(__linux__) || defined(__APPLE__)
        return HttpRESTClient::CreateInstance(cfg, std::forward<basic_callbacktype&&>(cb));
#elif (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))
        return WinHttpRESTClient::CreateInstance(cfg, std::forward<basic_callbacktype&&>(cb));
#else
#error "Platform not supported"
#endif
    }
} // namespace siddiqsoft

#endif // !RESTCL_HPP
