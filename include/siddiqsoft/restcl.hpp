/**
 * @file restcl.hpp
 * @author Abdulkareem Siddiq (github@siddiqsoft.com)
 * @brief Simple REST client using OpenSSL 3.x for Unix/Linux and Windows WinHTTP library for Windows platform
 * @version 0.1
 * @date 2024-12-24
 * 
 * @copyright Copyright (c) 2024 Siddiq Software
 * 
 */
#pragma once
#ifndef RESTCL_HPP
#define RESTCL_HPP


#if defined(__linux__) || defined(__APPLE__)
#include "private/restcl_unix.hpp"
#elif defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
#include "private/restcl_winhttp.hpp"
#else
#error "Platform not supported"
#endif
#endif // !RESTCL_HPP
