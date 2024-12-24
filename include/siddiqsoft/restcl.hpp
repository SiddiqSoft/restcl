
#pragma once
#ifndef RESTCL_HPP
#define RESTCL_HPP

#include "restcl_definitions.hpp"
#include "basic_request.hpp"
#include "basic_response.hpp"
#include "basic_restclient.hpp"
#include "rest_request.hpp"
#include "rest_response.hpp"

#if defined(__linux__) || defined(__APPLE__)
#include "restcl_unix.hpp"
#elif defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
#include "restcl_winhttp.hpp"
#else
#error "Platform not supported"
#endif
#endif // !RESTCL_HPP
