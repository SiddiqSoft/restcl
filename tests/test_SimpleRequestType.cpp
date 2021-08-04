/*
	restcl
	PROJECTDESCRIPTION

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

#include "gtest/gtest.h"
#include <iostream>

#include "nlohmann/json.hpp"
#include "../src/restcl.hpp"


namespace siddiqsoft
{
	TEST(basics, test1a)
	{
		SimpleRestRequestType<RESTMethodType::Get> srt {"https://www.siddiqsoft.com/"};

		nlohmann::json doc(srt);

		// Checks the implementation of the json implementation
		std::cerr << "Serialized SimpleRequestType: " << doc.dump(3) << std::endl;
	}


	TEST(basics, test1b)
	{
		SimpleRestRequestType<RESTMethodType::Get> srt {"https://www.siddiqsoft.com/"};

		// Checks the implementation of the encode() implementation
		std::cerr << "Wire serialize              : " << srt.encode() << std::endl;
	}


	TEST(basics, test1c)
	{
		SimpleRestRequestType<RESTMethodType::Get> srt {"https://www.siddiqsoft.com/"};

		// Checks the implementation of the std::format implementation
		std::cerr << std::format("Wire serialize              : {}\n", srt);
	}

} // namespace siddiqsoft