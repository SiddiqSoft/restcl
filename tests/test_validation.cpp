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


#include <iostream>
#include <version>
#include <expected>
#include <format>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "../include/siddiqsoft/restcl.hpp"

#include "gtest/gtest.h"

namespace siddiqsoft
{
    using namespace restcl_literals;

    class Validation : public ::testing::Test
    {
#if defined(__linux__) || defined(__APPLE__)
        std::shared_ptr<LibCurlSingleton> myCurlInstance {};
#endif
    protected:
        void SetUp() override
        {
#if defined(__linux__) || defined(__APPLE__)
            std::print(std::cerr, "{} - Init the CurlLib singleton.\n", __func__);
            myCurlInstance = LibCurlSingleton::GetInstance();
#endif
        }
    };

    TEST_F(Validation, test_rest_result_error)
    {
        DWORD             cc {12001};
        rest_result_error rre {cc};
        std::print(std::cerr, "Error code -> {}\n", rest_result_error {cc});
        EXPECT_EQ("12001-ERROR_INTERNET_OUT_OF_HANDLES: No more handles could be generated at this time.", rre.to_string());
    }

    TEST_F(Validation, restrequest_checks)
    {
        auto r1 = "https://www.siddiqsoft.com:65535/"_GET;
        EXPECT_EQ(HttpMethodType::METHOD_GET, r1.getMethod());
#if defined(DEBUG)
        EXPECT_EQ(65535, r1.uri.authority.port);
#endif
    }

    TEST_F(Validation, restrequest_checks2)
    {
        auto r2 = "https://localhost:65535/"_GET;
        EXPECT_EQ(HttpMethodType::METHOD_GET, r2.getMethod());
#if defined(DEBUG)
        EXPECT_EQ(65535, r2.uri.authority.port);
#endif

        auto r3 = "https://user.name@reqbin.com:9090/echo/post/json?source=Validation::restrequest_checks&param=r3"_OPTIONS;
        EXPECT_EQ(HttpMethodType::METHOD_OPTIONS, r3.getMethod());

        EXPECT_EQ(9090, r3.getUri().authority.port);
        EXPECT_EQ("user.name", r3.getUri().authority.userInfo);
        EXPECT_EQ(9090, r3.getUri().authority.port);
        EXPECT_EQ("/echo/post/json", r3.getUri().pathPart);
        EXPECT_EQ("source=Validation::restrequest_checks&param=r3", r3.getUri().queryPart);

        // nlohmann::json doc {r3};
        // std::cerr << "Serialized (encoded): " << r3 << std::endl;
        // std::cerr << "Serialized (json'd) : " << doc.dump(2) << std::endl;
    }

    TEST_F(Validation, GET_google_com)
    {
        std::atomic_bool passTest = false;
        restcl           wrc      = CreateRESTClient();

        wrc->configure().sendAsync(
                "https://www.google.com/"_GET, [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                    if (resp && resp->success()) {
                        passTest = true;
                        // std::print(std::cerr,
                        //            "{} - Response\n{}\n-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*\n",
                        //            __func__,
                        //            nlohmann::json(*resp).dump(3));
                    }
                    else if (resp.has_value()) {
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

    TEST_F(Validation, GET_duckduckgo_com)
    {
        std::atomic_bool passTest = false;
        restcl           wrc      = CreateRESTClient();

        wrc->configure({{"userAgent", std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __func__)}})
                .sendAsync("https://duckduckgo.com"_GET, [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                    if (resp && resp->success()) {
                        passTest = true;
                        // nlohmann::json doc(*resp);
                        // std::cerr << "Response\n" << doc.dump(3) << std::endl;
                    }
                    else if (resp.has_value()) {
                        auto [ec, emsg] = resp->status();
                        passTest        = ((ec == 12002) || (ec == 12029) || (ec == 400));
                        std::cerr << "Got error: " << ec << " -- `" << emsg << "`.." << std::endl;
                    }
                    else {
                        passTest = true;
                        std::print(std::cerr, "{}: failed: du{}\n", __func__, resp.error());
                    }
                    passTest.notify_all();
                });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST_F(Validation, POST_httpbin)
    {
        std::atomic_int passTest    = 0;
        restcl          wrc         = CreateRESTClient({{"trace", false},
                                                        {"userAgent", std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __func__)},
                                                        {"headers", {{"Accept", CONTENT_APPLICATION_JSON}}}});
        auto            postRequest = "https://httpbin.org/post"_POST;

        postRequest.setContent({{"Hello", "World"}, {"Welcome", "From"}, {"Source", {__LINE__, __COUNTER__}}});

        wrc->sendAsync(std::move(postRequest), [&passTest](const auto& req, std::expected<rest_response, int> resp) {
            if (resp.has_value() && resp->success()) {
                passTest = 1;
                // nlohmann::json doc(*resp);
                // std::print(std::cerr, "{} - POSITIVE Response\n{}\n", __func__, doc.dump(3));
            }
            else if (resp.has_value()) {
                nlohmann::json doc(*resp);

                auto [ec, emsg] = resp->status();
                passTest        = ((ec == 12002) || (ec == 12029) || (ec == 400)) ? 1 : -1;
                std::print(std::cerr, "{} - Got error: {} -- `{}`..\n{}\n", __func__, ec, emsg, doc.dump(2));
                // std::print(std::cerr, "{} - Got error:\n{}\n", __func__, doc.dump(2));
            }
            else {
                passTest = -1;
                std::print(std::cerr, "{}: failed:{}\n", __func__, resp.error());
            }
            passTest.notify_all();
        });

        passTest.wait(0);
        EXPECT_TRUE(passTest.load());
    }


    TEST_F(Validation, GET_Akamai_Time)
    {
        using namespace std::chrono_literals;
        using namespace siddiqsoft::restcl_literals;

        const unsigned   ITER_COUNT = 1;
        std::atomic_uint passTest   = 0;

        try {
            siddiqsoft::restcl wrc = CreateRESTClient();
            nlohmann::json     myStats {{"Test", "drift-check"}};

            auto req = "https://time.akamai.com/?iso"_GET;
            if (auto resp = wrc->send(req); resp->success()) {
                std::cerr << nlohmann::json(*resp).dump(2) << std::endl;
                EXPECT_EQ("Akamai/Time Server", resp->getHeader("Server"));
                // Expect the contents are date time stamp between 18-20 chars.
                EXPECT_TRUE(resp->getContent()->length > 16);

                passTest++;
                auto timeNow                = std::chrono::system_clock::now();
                myStats["timeRemoteSource"] = "https://time.akamai.com/?iso";
                myStats["timeRemoteTS"]     = resp->getContent()->body;

                auto [deltaMS, deltastr] =
                        siddiqsoft::DateUtils::diff(timeNow, siddiqsoft::DateUtils::parseISO8601(resp->getContent()->body));
                myStats["timeDriftMillis"] = std::to_string(deltaMS.count());
                myStats["timeDrift"]       = deltastr;
                myStats["timeNow"]         = siddiqsoft::DateUtils::ISO8601(timeNow);

                std::print(std::cerr, "{} - Time drift check:\n{}\n", __func__, myStats.dump(3));

                if ((deltaMS > 1500ms) || (deltaMS < -1500ms)) {
                    std::cerr << "  Found drift from clock more than 1500ms" << std::endl;
                }
            }
        }
        catch (const std::exception& ex) {
            std::print(std::cerr, "Housekeeping exception: {}", ex.what());
        }

        std::this_thread::sleep_for(1000ms);
        EXPECT_EQ(ITER_COUNT, passTest.load());
    }

    TEST_F(Validation, MoveConstructor)
    {
        int                             CLIENT_COUNT = 3;
        std::atomic_uint                passTest {0};
        std::vector<siddiqsoft::restcl> clients;
        int                             clientIndex {0};

        // std::print(std::cerr, "{} - Adding {} clients to vector...............\n", __FUNCTION__, CLIENT_COUNT);
        for (auto i = 0; i < CLIENT_COUNT; i++) {
            clients.push_back(CreateRESTClient({{"trace", false},
                                                {"freshConnect", true},
                                                {"userAgent",
                                                 std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; {1}:{2}; s:{0})",
                                                             "Validation, MoveConstructor",
                                                             i,
                                                             CLIENT_COUNT)}}));
        }

        EXPECT_EQ(CLIENT_COUNT, clients.size());

        // Send data over each client (if we mess up the move constructors this will fail)
        std::for_each(clients.begin(), clients.end(), [&](auto& wrc) {
            /*std::print(std::cerr,
                       "{} - Configuring client {}/{} individually...............\n",
                       __FUNCTION__,
                       clientIndex,
                       CLIENT_COUNT);*/
            wrc->sendAsync("https://reqbin.com/"_GET, [&](const auto& req, std::expected<rest_response, int> resp) {
                if (resp.has_value() && resp->success()) {
                    // We sometimes get a 403 and 200
                    passTest += (resp->statusCode() == 200) || (resp->statusCode() == 403);
                    // EXPECT_TRUE(resp->getHeaders().contains("X-EventID"));
                }
                else {
                    auto [ec, emsg] = resp->status();
                    passTest++;
                    std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
                }
            });
        });

        // This sleep is important otherwise we will end up destroying the IO workers before
        // any activity has had a chance to complete!
        // The underlying libraries do not "hold" and spool out their
        // transactions!
        std::this_thread::sleep_for(std::chrono::seconds(2 * CLIENT_COUNT));

        EXPECT_EQ(CLIENT_COUNT, passTest.load());
    }
} // namespace siddiqsoft
