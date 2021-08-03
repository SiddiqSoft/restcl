/*
	restcl : PROJECTDESCRIPTION

	BSD 3-Clause License

	Copyright (c) 2021, Siddiq Software LLC
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	   contributors may be used to endorse or promote products derived from
	   this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#ifndef RESTCL_HPP
#define RESTCL_HPP

#include <chrono>
#include <string>
#include <functional>
#include <memory>

#include "nlohmann/json.hpp"

#if defined __cpp_lib_format
#include <format>
#endif

#include "RESTMethodType.hpp"
#include "HttpRequestType.hpp"
#include "Helpers.hpp"
#include "Helper_SplitUri.hpp"

namespace siddiqsoft
{
	template <RESTMethodType ReqType> class SimpleRestRequestType
	{
		nlohmann::json data {{"request", null}, {"headers", null}, {"content", null}};

	public:
		SimpleRestRequestType(const std::string& uri, const nlohmann::json& h = {}, const nlohmann::json& c = {})
		{
			// Decode the uri which has http(s)://x.y.z:port/section/arg?param=&param2
			auto [server, port, query] = SplitUri<std::string>(uri);

			data["headers"]         = h;
			data["content"]         = c;
			data["request"]         = {{"method", ReqType}, {"uri", query}, {"version", "HTTP/1.1"}};
			data["headers"]["Host"] = std::format("{}:{}", server, port);
		}
	};


} // namespace siddiqsoft

#endif // !RESTCL_HPP
