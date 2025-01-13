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
#include "private/basic_restclient.hpp"
#ifndef RESTCL_HPP
#define RESTCL_HPP


#if defined(__linux__) || defined(__APPLE__)
#include "private/restcl_unix.hpp"
#elif (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))
#include "private/restcl_win.hpp"
#else
#error "Platform not supported"
#endif

namespace siddiqsoft
{
    [[nodiscard]] static auto CreateClient(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {})
    {
#if defined(__linux__) || defined(__APPLE__)
        return HttpRESTClient::CreateClient(cfg, std::forward<basic_callbacktype&&>(cb));
#elif (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))
        return WinHttpRESTClient::CreateClient(cfg, std::forward<basic_callbacktype&&>(cb));
#else
#error "Platform not supported"
#endif
    }
} // namespace siddiqsoft

#endif // !RESTCL_HPP
