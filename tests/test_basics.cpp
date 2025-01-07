/*
    restcl : Tests

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
#include <barrier>
#include <version>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "../include/siddiqsoft/restcl.hpp"

namespace siddiqsoft
{
    using namespace restcl_literals;

    TEST(Serializers, test_GET)
    {
        auto srt = "https://www.siddiqsoft.com/"_GET;
        std::cerr << "Going to check if we can dump.." << std::endl;

        std::cerr << srt << std::endl;

        // EXPECT_NO_THROW({
        //  nlohmann::json doc(srt);
        // nlohmann::json doc2 {std::move(srt)};
        nlohmann::json doc2 = srt;
        // Checks the implementation of the json implementation
        std::cerr << "Serialized json: " << doc2.dump(3) << std::endl;
        //});
    }


    TEST(Serializers, test1b)
    {
        auto srt = "https://www.siddiqsoft.com/"_GET;

        // Checks the implementation of the encode() implementation
        std::cerr << "Wire serialize              : " << srt.encode() << std::endl;
    }


    TEST(Serializers, test1c)
    {
        auto srt = "https://www.siddiqsoft.com/"_GET;

        // Checks the implementation of the std::formatter implementation
        std::cerr << std::format("Wire serialize              : {}\n", srt);
    }


    TEST(Validation, test1)
    {
        auto r1 = "https://www.siddiqsoft.com:65535/"_GET;
        EXPECT_EQ(HttpMethodType::METHOD_GET, r1.getMethod());
#if defined(DEBUG)
        EXPECT_EQ(65535, r1.uri.authority.port);
#endif

        auto r2 = "https://localhost:65535/"_GET;
        EXPECT_EQ(HttpMethodType::METHOD_GET, r2.getMethod());
#if defined(DEBUG)
        EXPECT_EQ(65535, r2.uri.authority.port);
#endif

        auto r3 = "https://user.name@reqbin.com:9090/echo/post/json?source=Validate::test1&param=r3"_OPTIONS;
        EXPECT_EQ(HttpMethodType::METHOD_OPTIONS, r3.getMethod());

#if defined(DEBUG)
        EXPECT_EQ(9090, r3.uri.authority.port);
#endif

        nlohmann::json doc {r3};
        std::cerr << "Serialized (encoded): " << r3 << std::endl;
        std::cerr << "Serialized (json'd) : " << doc.dump(2) << std::endl;
    }

    TEST(Validation, test_positive_google_com)
    {
        std::atomic_bool passTest = false;
        restcl           wrc;

        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __func__)))
                .sendAsync("https://www.google.com/"_GET, [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                    if (resp && resp->success()) {
                        passTest = true;
                        // std::cerr << "Response\n" << *resp << std::endl;
                    }
                    else if (resp) {
                        auto [ec, emsg] = resp->status();
                        passTest        = ((ec == 12002) || (ec == 12029) || (ec == 400));
                        std::cerr << "Got error: " << ec << " -- `" << emsg << "`.." << std::endl;
                    }
                    else {
                        std::cerr << "Got error: " << resp.error() << " -- " << strerror(resp.error()) << std::endl;
                    }
                    passTest.notify_all();
                });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST(Validation, test_positive_bing_com)
    {
        std::atomic_bool passTest = false;
        restcl           wrc;

        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __func__)))
                .sendAsync("https://www.bing.com/"_GET, [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                    if (resp && resp->success()) {
                        passTest = true;
                        // std::cerr << "Response\n" << *resp << std::endl;
                    }
                    else if (resp) {
                        auto [ec, emsg] = resp->status();
                        passTest        = ((ec == 12002) || (ec == 12029) || (ec == 400));
                        std::cerr << "Got error: " << ec << " -- `" << emsg << "`.." << std::endl;
                    }
                    else {
                        passTest = true;
                        std::cerr << std::format("{}: failed:{}\n", __func__, resp.error());
                    }
                    passTest.notify_all();
                });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST(Validation, test_positive_httpbin)
    {
        std::atomic_bool passTest = false;
        restcl           wrc;
        auto             postRequest = "https://httpbin.org/post"_POST;

        postRequest.setContent({{"Hello", "World"}, {"Welcome", "From"}});

        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __func__)))
                .sendAsync(std::move(postRequest), [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                    if (resp && resp->success()) {
                        passTest = true;
                        // std::cerr << "Response\n" << *resp << std::endl;
                    }
                    else if (resp) {
                        auto [ec, emsg] = resp->status();
                        passTest        = ((ec == 12002) || (ec == 12029) || (ec == 400));
                        std::cerr << "Got error: " << ec << " -- `" << emsg << "`.." << std::endl;
                    }
                    else {
                        passTest = true;
                        std::cerr << std::format("{}: failed:{}\n", __func__, resp.error());
                    }
                    passTest.notify_all();
                });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }
} // namespace siddiqsoft
