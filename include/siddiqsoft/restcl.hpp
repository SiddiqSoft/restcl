/**
 * @file restcl.hpp
 * @author Abdulkareem Siddiq (github@siddiqsoft.com)
 * @brief Simple REST client using LibCURL 8.7 for Unix/Linux and Windows WinHTTP library for Windows platform
 * @version 0.1
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
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
     * @brief Obtain a REST client from internal pool (where caching available).
     *
     * @param cfg Optional configuraiton for the underlying provider.
     * @param cb The global callback handler. Note that if you invoke sendAsycn() with a callback then
     *           this callback is not used for that single request and subsequent sendAsync() with
     *           empty callback param will use this registered callback.
     *           If no callback is registered here or provided during call to sendAsync then sendAsync
     *           throws exception.
     * @return auto For Linux and UNIX the return is HttpRESTClient which uses libCURL implementation.
     *              For Windows the return is WinHttpRESTClient based on WinHTTP library.
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
