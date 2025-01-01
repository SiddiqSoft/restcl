/**
 * @file test_restcl.cpp
 * @author Abdulkareem Siddiq (github@siddiqsoft.com)
 * @brief
 * @version 0.1
 * @date 2024-12-24
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 */

#include "gtest/gtest.h"
#include <atomic>
#include <iostream>
#include <barrier>
#include <version>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/restcl.hpp"
#include "siddiqsoft/RunOnEnd.hpp"

namespace siddiqsoft
{
    using namespace restcl_literals;

    TEST(TSendRequest, test1a)
    {
        std::atomic_bool passTest = false;
        restcl           wrc;

        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __func__)))
                .send("https://www.siddiqsoft.com/"_GET, [&passTest](const auto& req, const auto& resp) {
                    nlohmann::json doc(req);

                    std::cerr << "From callback Serialized json: " << req << std::endl;
                    if (resp.success()) {
                        passTest = true;
                        std::cerr << "Response\n" << resp << std::endl;
                    }
                    else {
                        auto [ec, emsg] = resp.status();
                        passTest        = ((ec == 12002) || (ec == 12029) || (ec == 400));
                        std::cerr << "Got error: " << ec << " -- `" << emsg << "`.." << std::endl;
                    }
                    passTest.notify_all();
                });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }


    TEST(TSendRequest, test2a)
    {
        std::atomic_bool passTest = false;
        restcl           wrc;

        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __func__)))
                .send("https://reqbin.com/echo/post/json"_OPTIONS, [&passTest](const auto& req, auto& resp) {
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

        restcl      wrc;
        std::string responseContentType {};

        wrc.send(rest_request {HttpMethodType::POST,
                               "https://httpbin.org/post"_Uri,
                               {{"Content-Type", "application/json"}},
                               std::format("{{ \"email\":\"jolly@email.com\", \"password\":\"123456\", \"date\":\"{:%FT%TZ}\" }}",
                                           std::chrono::system_clock::now())},
                 [&passTest, &responseContentType](const auto& req, const auto& resp) {
                     responseContentType = req.getHeaders().value("Content-Type", "");
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

        restcl wrc;

        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)))
                .send(rest_request {HttpMethodType::POST,
                                    "https://httpbin.org/post"_Uri,
                                    {{"Authorization", "Basic YWF1OnBhYXU="}, {"Content-Type", "application/json+custom"}},
                                    {{"foo", "bar"}, {"hello", "world"}}},
                      [&passTest](const auto& req, const auto& resp) {
                          EXPECT_EQ("application/json+custom", req.getHeaders().value("Content-Type", ""));
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

        restcl wrc;

        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)))
                .send("https://www.siddiqsoft.com:65535/"_GET, [&passTest](const auto& req, const auto& resp) {
                    // nlohmann::json doc(req);

                    // Checks the implementation of the json implementation
                    // std::cerr << "From callback Serialized json: " << req << std::endl;
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

    TEST(TSendRequest, Fails_1b)
    {
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::splituri_literals;

        restcl wrc;

        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)))
                .send("https://localhost:65535/"_GET, [&passTest](const auto& req, const auto& resp) {
                    nlohmann::json doc(req);

                    // Checks the implementation of the json implementation
                    // std::cerr << "From callback Serialized json: " << req << std::endl;
                    if (resp.success()) {
                        std::cerr << "Response\n" << resp << std::endl;
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

        restcl wrc;

        // The endpoint does not support OPTIONS verb. Moreover, it does not listen on port 9090 either.
        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)))
                .send("https://httpbin.org:9090/get"_OPTIONS, [&passTest](const auto& req, auto& resp) {
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

        restcl wrc;

        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)))
                .send("https://google.com/"_OPTIONS, [&passTest](const auto& req, auto& resp) {
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
        std::atomic_bool passTest = false;
        restcl           wrc;

        wrc.configure((std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)))
                .send("https://www.google.com/"_GET, [&passTest](const auto& req, const auto& resp) {
                    // std::cerr << "From callback Serialized json: " << req << std::endl;
                    if (resp.success()) {
                        passTest = resp.statusCode() == 200;
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
        std::atomic_uint                passTest {0};
        std::vector<siddiqsoft::restcl> clients;

        for (auto i = 0; i < 4; i++) {
            clients.push_back(siddiqsoft::restcl {});
        }

        EXPECT_EQ(4, clients.size());

        // Send data over each client (if we mess up the move constructors this will fail)
        std::for_each(clients.begin(), clients.end(), [&](auto& wrc) {
            wrc.configure(std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__),
                          [&](const auto& req, const auto& resp) {
                              // std::cerr << "From callback Serialized json: " << req << std::endl;
                              if (resp.success()) {
                                  passTest += resp.statusCode() == 200;
                                  // std::cerr << "Response\n" << resp << std::endl;
                              }
                              else {
                                  auto [ec, emsg] = resp.status();
                                  std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
                              }
                          })
                    .send("https://www.google.com/"_GET);
        });

        std::this_thread::sleep_for(std::chrono::seconds(2));

        EXPECT_EQ(4, passTest.load());
    }


    TEST(Threads, test_1)
    {
        const unsigned   ITER_COUNT = 9;
        std::atomic_uint passTest   = 0;
        std::atomic_uint callbackCounter {0};

        {
            restcl               wrc;
            siddiqsoft::RunOnEnd roe([&]() { std::cerr << "Final Stats of the client: " << wrc << std::endl; });

            std::cerr << std::format("Post wrc..\n");
            basic_callbacktype valid = [&](const auto& req, const auto& resp) {
                callbackCounter++;
                // std::cerr << "From callback Serialized json: " << req << std::endl;
                if (resp.success()) {
                    passTest += resp.statusCode() == 200;
                    passTest.notify_all();
                    std::cerr << "Response\n" << resp << std::endl;
                }
                else {
                    std::cerr << "Got error: " << resp.statusCode() << " for " << req.uri.authority.host << " -- "
                              << resp.reasonCode() << std::endl;
                }

                std::cerr << "Stats of the client #" << callbackCounter << "..: " << wrc << std::endl;
            };

            wrc.configure(std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__), std::move(valid));

#ifdef _DEBUG0
            std::cerr << std::format("Adding {} items..\n", ITER_COUNT);
#endif

            for (auto i = 0; i < ITER_COUNT; i++) {
                if (i % 3 == 0) {
                    wrc.send("https://www.yahoo.com/"_GET);
#ifdef _DEBUG0
                    std::cerr << std::format("Added i:{}  i%3:{}  \n", i, (i % 3));
#endif
                }
                else if (i % 2 == 0) {
                    wrc.send("https://www.bing.com/"_GET);
#ifdef _DEBUG0
                    std::cerr << std::format("Added i:{}  i%2:{}  \n", i, (i % 2));
#endif
                }
                else {
                    wrc.send("https://www.google.com/"_GET);
#ifdef _DEBUG0
                    std::cerr << std::format("Added i:{}  ......  \n", i);
#endif
                }
            }
#ifdef _DEBUG0
            std::cerr << std::format("Finished adding {} items..\n", ITER_COUNT);
#endif
            std::this_thread::sleep_for(std::chrono::milliseconds(9900));
        }

        std::this_thread::sleep_for(std::chrono::seconds(9));

        std::cerr << "Wrapup; ITER_COUNT: " << ITER_COUNT << "\npassTest: " << passTest.load()
                  << "\ncallbackCounter: " << callbackCounter.load() << std::endl;

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

            siddiqsoft::restcl wrc;
            nlohmann::json     myStats {"Test", "drift-check"};

            wrc.configure(std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__));
            auto req = "https://time.akamai.com/?iso"_GET;
            if (siddiqsoft::rest_response resp = wrc.send(req); resp.success()) {
                std::cerr << resp << std::endl;

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
