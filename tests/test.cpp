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
#include "../src/restcl.hpp"
#include "../src/restcl_winhttp.hpp"

#ifndef __cpp_lib_atomic_flag_test
#pragma message("nothing to do; please update to ensure C++20 support!")
#endif

#ifndef _HAS_CXX20
#pragma message("nothing to do; please update to ensure C++20 support!")
#endif


namespace siddiqsoft
{
    using namespace literals;

    TEST(Serializers, test1a)
    {
        auto srt = "https://www.siddiqsoft.com/"_GET;

        nlohmann::json doc(srt);

        // Checks the implementation of the json implementation
        std::cerr << "Serialized json: " << doc.dump(3) << std::endl;
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


    TEST(Validate, test1)
    {
        auto r1 = "https://www.siddiqsoft.com:65535/"_GET;
        EXPECT_EQ("GET", r1["request"].value("method", ""));
        EXPECT_EQ(65535, r1.uri.authority.port);

        auto r2 = "https://localhost:65535/"_GET;
        EXPECT_EQ("GET", r2["request"].value("method", ""));
        EXPECT_EQ(65535, r2.uri.authority.port);

        auto r3 = "https://reqbin.com:9090/echo/post/json"_OPTIONS;
        EXPECT_EQ("OPTIONS", r3["request"].value("method", ""));
        EXPECT_EQ(9090, r3.uri.authority.port);
    }
} // namespace siddiqsoft

namespace siddiqsoft
{
    using namespace siddiqsoft::literals;

    TEST(TSendRequest, test1a)
    {
        std::atomic_bool  passTest = false;
        WinHttpRESTClient wrc;

        wrc.send("https://www.siddiqsoft.com/"_GET, [&passTest](const auto& req, const auto& resp) {
            nlohmann::json doc(req);

            // std::cerr << "From callback Serialized json: " << req << std::endl;
            if (resp.success()) {
                passTest = true;
                // std::cerr << "Response\n" << nlohmann::json(resp).dump(3) << std::endl;
            }
            else {
                auto [ec, emsg] = resp.status();
                passTest        = ((ec == 12002) || (ec == 12029));
                std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
            }
            passTest.notify_all();
        });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }


    TEST(TSendRequest, test2a)
    {
        std::atomic_bool  passTest = false;
        WinHttpRESTClient wrc(std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __func__));

        wrc.send("https://reqbin.com/echo/post/json"_OPTIONS, [&passTest](const auto& req, auto& resp) {
            // Checks the implementation of the encode() implementation
            std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
            if (passTest = resp.success(); passTest.load()) {
                std::cerr << "Response\n" << resp << std::endl;
            }
            else {
                auto [ec, emsg] = resp.status();
                std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
            }
            passTest.notify_all();
        });

        passTest.wait(false);
        std::cerr << "Checking results..\n";
        EXPECT_TRUE(passTest.load());
    }


    TEST(TSendRequest, test3a)
    {
        std::atomic_bool passTest = false;

        WinHttpRESTClient wrc;
        std::string       responseContentType {};

        wrc.send(ReqPost {"https://httpbin.org/post"_Uri,
                          {{"Content-Type", "application/json"}},
                          std::format("{{ \"email\":\"jolly@email.com\", \"password\":\"123456\", \"date\":\"{:%FT%TZ}\" }}",
                                      std::chrono::system_clock::now())},
                 [&passTest, &responseContentType](const auto& req, const auto& resp) {
                     responseContentType = req["headers"].value("Content-Type", "");
                     //  Checks the implementation of the encode() implementation
                     // std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
                     if (passTest = resp.success(); passTest.load()) {
                         std::cerr << "Response\n" << resp << std::endl;
                     }
                     else {
                         auto [ec, emsg] = resp.status();
                         std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
                     }
                     passTest.notify_all();
                 });

        passTest.wait(false);
        std::cerr << "Checking results..\n";
        EXPECT_EQ("application/json", responseContentType);
        EXPECT_TRUE(passTest.load());
    }

    TEST(TSendRequest, test3b)
    {
        // https://ptsv2.com/t/buzz2
        std::atomic_bool passTest = false;
        // auto auth     = base64encode("aau:paau");

        WinHttpRESTClient wrc(std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__));

        wrc.send(ReqPost {"https://httpbin.org/post"_Uri,
                          {{"Authorization", "Basic YWF1OnBhYXU="}, {"Content-Type", "application/json+custom"}},
                          {{"foo", "bar"}, {"hello", "world"}}},
                 [&passTest](const auto& req, const auto& resp) {
                     EXPECT_EQ("application/json+custom", req["headers"].value("Content-Type", ""));
                     // Checks the implementation of the std::format implementation
                     std::cerr << std::format("From callback Wire serialize              : {}\n", req);
                     if (passTest = resp.success(); passTest.load()) {
                         std::cerr << "Response\n" << resp << std::endl;
                     }
                     else {
                         auto [ec, emsg] = resp.status();
                         std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
                     }
                     passTest.notify_all();
                 });

        passTest.wait(false);
        std::cerr << "Checking results..\n";
        EXPECT_TRUE(passTest.load());
    }


    TEST(TSendRequest, Fails_1a)
    {
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::literals;

        WinHttpRESTClient wrc(std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__));

        wrc.send("https://www.siddiqsoft.com:65535/"_GET, [&passTest](const auto& req, const auto& resp) {
            // nlohmann::json doc(req);

            // Checks the implementation of the json implementation
            // std::cerr << "From callback Serialized json: " << req << std::endl;
            if (resp.success()) {
                std::cerr << "Response\n" << nlohmann::json(resp).dump(3) << std::endl;
            }
            else {
                auto [ec, emsg] = resp.status();
                passTest        = ((ec == 12002) || (ec == 12029));
                std::cerr << "passTest: " << passTest << "  Got error: " << ec << " --" << emsg << std::endl;
            }
            passTest.notify_all();
        });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST(TSendRequest, Fails_1b)
    {
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::literals;

        WinHttpRESTClient wrc(std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__));

        wrc.send("https://localhost:65535/"_GET, [&passTest](const auto& req, const auto& resp) {
            nlohmann::json doc(req);

            // Checks the implementation of the json implementation
            // std::cerr << "From callback Serialized json: " << req << std::endl;
            if (resp.success()) {
                std::cerr << "Response\n" << nlohmann::json(resp).dump(3) << std::endl;
            }
            else {
                auto [ec, emsg] = resp.status();
                passTest        = ec == 12029;
                // std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
            }
            passTest.notify_all();
        });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST(TSendRequest, Fails_1c)
    {
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::literals;

        WinHttpRESTClient wrc(std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__));

        // The endpoint does not support OPTIONS verb. Moreover, it does not listen on port 9090 either.
        wrc.send("https://httpbin.org:9090/get"_OPTIONS, [&passTest](const auto& req, auto& resp) {
            std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
            if (resp.success()) {
                std::cerr << "Response\n" << resp << std::endl;
            }
            else {
                auto [ec, emsg] = resp.status();
                passTest        = ((ec == 12002) || (ec == 12029));
                std::cerr << "passTest: " << passTest << "  Got error: " << ec << " --" << emsg << std::endl;
            }
            passTest.notify_all();
        });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST(TSendRequest, Fails_2a)
    {
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::literals;

        WinHttpRESTClient wrc(std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__));

        wrc.send("https://google.com/"_OPTIONS, [&passTest](const auto& req, auto& resp) {
            // std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
            if (resp.success()) {
                std::cerr << "Response\n" << resp << std::endl;
            }
            else {
                auto [ec, emsg] = resp.status();
                passTest        = ec == 405;
                // This is a work-around for google which sometimes refuses to send the Reason Phrase!
                if (!emsg.empty()) passTest = passTest && (emsg == "Method Not Allowed");
                // std::cerr << "Got error: [" << ec << ":" << emsg << "] -- " << emsg << std::endl
                //          << nlohmann::json(resp).dump(3) << std::endl;
            }
            passTest.notify_all();
        });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST(TSendRequest, test9a)
    {
        std::atomic_bool  passTest = false;
        WinHttpRESTClient wrc;

        wrc.send("https://www.google.com/"_GET, [&passTest](const auto& req, const auto& resp) {
            // std::cerr << "From callback Serialized json: " << req << std::endl;
            if (resp.success()) {
                passTest = resp["response"].value("status", 0) == 200;
                // std::cerr << "Response\n" << resp << std::endl;
            }
            else {
                auto [ec, emsg] = resp.status();
                std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
            }
            passTest.notify_all();
        });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }


    TEST(restcl, MoveConstructor)
    {
        std::atomic_uint                           passTest {0};
        std::vector<siddiqsoft::WinHttpRESTClient> clients;

        for (auto i = 0; i < 4; i++) {
            clients.push_back(siddiqsoft::WinHttpRESTClient {});
        }

        EXPECT_EQ(4, clients.size());

        // Send data over each client (if we mess up the move constructors this will fail)
        std::for_each(clients.begin(), clients.end(), [&](auto& wrc) {
            wrc.send("https://www.google.com/"_GET, [&](const auto& req, const auto& resp) {
                // std::cerr << "From callback Serialized json: " << req << std::endl;
                if (resp.success()) {
                    passTest += resp["response"].value("status", 0) == 200;
                    // std::cerr << "Response\n" << resp << std::endl;
                }
                else {
                    auto [ec, emsg] = resp.status();
                    std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
                }
            });
        });

        std::this_thread::sleep_for(std::chrono::seconds(2));

        EXPECT_EQ(4, passTest.load());
    }


    TEST(Threads, test_1)
    {
        const unsigned   ITER_COUNT = 29;
        std::atomic_uint passTest   = 0;
        std::cerr << std::format("Starting..\n");

        {
            WinHttpRESTClient wrc;

            std::cerr << std::format("Post wrc..\n");
            basic_callbacktype valid = [&passTest](const auto& req, const auto& resp) {
                // std::cerr << "From callback Serialized json: " << req << std::endl;
                if (resp.success()) {
                    passTest += resp.status().code == 200;
                    passTest.notify_all();
                    // std::cerr << "Response\n" << resp << std::endl;
                }
                else {
                    auto [ec, emsg] = resp.status();
                    std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
                }
            };

#ifdef _DEBUG
            std::cerr << std::format("Adding {} items..\n", ITER_COUNT);
#endif

            for (auto i = 0; i < ITER_COUNT; i++) {
                if (i % 3 == 0) {
                    wrc.send("https://www.yahoo.com/"_GET, valid);
#ifdef _DEBUG
                    std::cerr << std::format("Added i:{}  i%3:{}  \n", i, (i % 3));
#endif
                }
                else if (i % 2 == 0) {
                    wrc.send("https://www.bing.com/"_GET, valid);
#ifdef _DEBUG
                    std::cerr << std::format("Added i:{}  i%2:{}  \n", i, (i % 2));
#endif
                }
                else {
                    wrc.send("https://www.google.com/"_GET, valid);
#ifdef _DEBUG
                    std::cerr << std::format("Added i:{}  ......  \n", i);
#endif
                }
            }
#ifdef _DEBUG
            std::cerr << std::format("Finished adding {} items..\n", ITER_COUNT);
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(9900));
        }
        // std::this_thread::sleep_for(std::chrono::seconds(5));
        EXPECT_EQ(ITER_COUNT, passTest.load());
    }


} // namespace siddiqsoft
