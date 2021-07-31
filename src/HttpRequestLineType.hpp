#pragma once
#ifndef HTTPREQUESTLINETYPE_HPP
#define HTTPREQUESTLINETYPE_HPP

#include <chrono>
#include <string>
#include <functional>
#include <memory>

#include "nlohmann/json.hpp"

#if defined __cpp_lib_format
#include <format>
#endif

#include "RESTMethodType.hpp"

namespace siddiqsoft
{
	struct HttpRequestLineType
	{
		RESTMethodType method {RESTMethodType::Unknown};
		std::string    path {"/"};
		std::string    version {"HTTP/1.1"};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(HttpRequestLineType, method, path, version)
	};
} // namespace siddiqsoft

#endif // !HTTPREQUESTLINETYPE_HPP
