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

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "gtest/gtest.h"
#include <iostream>

#include "nlohmann/json.hpp"
#include "../src/WinHttpRESTClient.hpp"


namespace siddiqsoft
{
	using namespace siddiqsoft::literals;

	TEST(TSendRequest, test1a)
	{
		bool              passTest = false;
		WinHttpRESTClient wrc;

		wrc.send("https://www.siddiqsoft.com/"_GET, [&passTest](const auto& req, const auto& resp) {
			nlohmann::json doc(req);

			std::cerr << "From callback Serialized json: " << req << std::endl;
			if (resp.success())
			{
				passTest = true;
				std::cerr << "Response\n" << nlohmann::json(resp).dump(3) << std::endl;
			}
			else
			{
				auto [ec, emsg] = resp.status();
				std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
			}
		});

		EXPECT_TRUE(passTest);
	}


	TEST(TSendRequest, test2a)
	{
		bool              passTest = false;
		WinHttpRESTClient wrc;

		wrc.send("https://reqbin.com/echo/post/json"_OPTIONS, [&passTest](const auto& req, auto& resp) {
			// Checks the implementation of the encode() implementation
			std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
			if (resp.success())
			{
				passTest = true;
				std::cerr << "Response\n" << resp << std::endl;
			}
			else
			{
				auto [ec, emsg] = resp.status();
				std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
			}
		});

		EXPECT_TRUE(passTest);
	}


	TEST(TSendRequest, test3a)
	{
		bool passTest = false;

		WinHttpRESTClient wrc;

		wrc.send(RESTRequestType {RESTMethodType::Post,
		                          SplitUri(std::format("https://ptsv2.com/t/buzz2/post?function={}", __FUNCTION__)),
		                          {{"Authorization", "Basic YWF1OnBhYXU="}, {"Content-Type", "application/xml"}},
		                          std::format("<root><p>Hello-world</p><p name=\"date\">{:%FT%TZ}</p></root>",
		                                      std::chrono::system_clock::now())},
		         [&passTest](const auto& req, const auto& resp) {
					 // Checks the implementation of the encode() implementation
					 std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
					 if (resp.success())
					 {
						 passTest = true;
						 std::cerr << "Response\n" << resp << std::endl;
					 }
					 else
					 {
						 auto [ec, emsg] = resp.status();
						 std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
					 }
				 });

		EXPECT_TRUE(passTest);
	}

	TEST(TSendRequest, test3b)
	{
		// https://ptsv2.com/t/buzz2
		bool passTest = false;
		//auto auth     = base64encode("aau:paau");

		WinHttpRESTClient wrc;

		wrc.send({RESTMethodType::Post,
		          "https://ptsv2.com/t/buzz2/post"_Uri,
		          {{"Authorization", "Basic YWF1OnBhYXU="}},
		          {{"foo", "bar"}, {"hello", "world"}}},
		         [&passTest](const auto& req, const auto& resp) {
					 // Checks the implementation of the std::format implementation
					 std::cerr << std::format("From callback Wire serialize              : {}\n", req);
					 if (resp.success())
					 {
						 passTest = true;
						 std::cerr << "Response\n" << resp << std::endl;
					 }
					 else
					 {
						 auto [ec, emsg] = resp.status();
						 std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
					 }
				 });

		EXPECT_TRUE(passTest);
	}


	TEST(TSendRequest, Fails_1a)
	{
		bool passTest = false;

		WinHttpRESTClient wrc;

		wrc.send("https://www.siddiqsoft.com:65535/"_GET, [&passTest](const auto& req, const auto& resp) {
			nlohmann::json doc(req);

			// Checks the implementation of the json implementation
			std::cerr << "From callback Serialized json: " << req << std::endl;
			if (resp.success()) { std::cerr << "Response\n" << nlohmann::json(resp).dump(3) << std::endl; }
			else
			{
				auto [ec, emsg] = resp.status();
				passTest        = ec == 12002;
				std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
			}
		});

		EXPECT_TRUE(passTest);
	}

	TEST(TSendRequest, Fails_1b)
	{
		bool passTest = false;

		WinHttpRESTClient wrc;

		wrc.send("https://localhost:65535/"_GET, [&passTest](const auto& req, const auto& resp) {
			nlohmann::json doc(req);

			// Checks the implementation of the json implementation
			std::cerr << "From callback Serialized json: " << req << std::endl;
			if (resp.success()) { std::cerr << "Response\n" << nlohmann::json(resp).dump(3) << std::endl; }
			else
			{
				auto [ec, emsg] = resp.status();
				passTest        = ec == 12029;
				std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
			}
		});

		EXPECT_TRUE(passTest);
	}

	TEST(TSendRequest, Fails_1c)
	{
		bool passTest = false;

		WinHttpRESTClient wrc;

		wrc.send("https://reqbin.com:9090/echo/post/json"_OPTIONS, [&passTest](const auto& req, auto& resp) {
			std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
			if (resp.success()) { std::cerr << "Response\n" << resp << std::endl; }
			else
			{
				auto [ec, emsg] = resp.status();
				passTest        = ec == 12002;
				std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
			}
		});

		EXPECT_TRUE(passTest);
	}

	TEST(TSendRequest, Fails_2a)
	{
		bool passTest = false;

		WinHttpRESTClient wrc;

		wrc.send("https://google.com/"_OPTIONS, [&passTest](const auto& req, auto& resp) {
			std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
			if (resp.success()) { std::cerr << "Response\n" << resp << std::endl; }
			else
			{
				auto [ec, emsg] = resp.status();
				passTest        = ec == 405;
				std::cerr << "Got error: " << ec << " -- " << emsg << std::endl << nlohmann::json(resp).dump(3) << std::endl;
			}
		});

		EXPECT_TRUE(passTest);
	}

} // namespace siddiqsoft