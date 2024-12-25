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

#if defined (WIN32) || defined (_WIN32) || defined (WIN64) || defined (_WIN64)
#include "gtest/gtest.h"
#include <iostream>
#include <barrier>
#include <version>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/restcl.hpp"


namespace siddiqsoft
{
    using namespace restcl_literals;

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
        using namespace siddiqsoft::splituri_literals;
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
        using namespace siddiqsoft::splituri_literals;

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
        using namespace siddiqsoft::splituri_literals;

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
        using namespace siddiqsoft::splituri_literals;

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
        using namespace siddiqsoft::splituri_literals;

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
        using namespace siddiqsoft::splituri_literals;

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
        const unsigned   ITER_COUNT = 9;
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
                    std::cerr << "Got error: " << ec << " for " << req.uri.authority.host << " -- " << emsg << std::endl;
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
            //std::this_thread::sleep_for(std::chrono::milliseconds(9900));
        }

        std::this_thread::sleep_for(std::chrono::seconds(4));
        EXPECT_EQ(ITER_COUNT, passTest.load());
    }

    TEST(Threads, Test_Send_Synchronous)
    {
        const unsigned   ITER_COUNT = 1;
        std::atomic_uint passTest   = 0;
        std::cerr << std::format("Starting..\n");

        try {
            using namespace std::chrono_literals;
            using namespace siddiqsoft::restcl_literals;

            siddiqsoft::WinHttpRESTClient wrc {"pmd4-drift-check"};
            nlohmann::json                myStats {"Test", "drift-check"};

            auto req = "https://time.akamai.com/?iso"_GET;
            if (siddiqsoft::basic_response resp = wrc.send(req); resp.success()) {
                passTest++;
                auto timeNow                = std::chrono::system_clock::now();
                myStats["timeRemoteSource"] = "https://time.akamai.com/?iso";
                myStats["timeRemoteTS"]     = resp.encode();

                auto [deltaMS, deltastr] = siddiqsoft::DateUtils::diff(timeNow, siddiqsoft::DateUtils::parseISO8601(resp.encode()));
                myStats["timeDriftMillis"] = std::to_string(deltaMS.count());
                myStats["timeDrift"]       = deltastr;
                myStats["timeNow"]         = siddiqsoft::DateUtils::ISO8601(timeNow);

                std::cerr << std::format("Time drift check {}", myStats.dump()) << std::endl;

                if ((deltaMS > 1500ms) || (deltaMS < -1500ms)) {
                    std::cerr << "  Found drift from clock more than 1500ms" << std::endl;
                }
            }
        }
        catch (const std::exception& ex) {
            std::cerr << std::format("Housekeeping exception: {}", ex.what()) << std::endl;
        }

        EXPECT_EQ(ITER_COUNT, passTest.load());
    }
} // namespace siddiqsoft
#endif