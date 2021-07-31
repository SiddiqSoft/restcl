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
#ifndef RESTMETHOD_HPP
#define RESTMETHOD_HPP

#include "nlohmann/json.hpp"


namespace siddiqsoft
{
	enum class RESTMethod
	{
		Get,
		Patch,
		Post,
		Put,
		Delete,
		Info,
		Unknown
	};

	NLOHMANN_JSON_SERIALIZE_ENUM(RESTMethod,
	                             {{RESTMethod::Get, "GET"},
	                              {RESTMethod::Patch, "PATCH"},
	                              {RESTMethod::Post, "POST"},
	                              {RESTMethod::Put, "PUT"},
	                              {RESTMethod::Delete, "DELETE"},
	                              {RESTMethod::Info, "INFO"},
	                              {RESTMethod::Unknown, nullptr}});


} // namespace siddiqsoft

#endif // !RESTMETHOD_HPP
