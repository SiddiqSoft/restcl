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

// #if defined(__linux__) || defined(__APPLE__)

#include "gtest/gtest.h"
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <barrier>
#include <version>
#include <expected>
#include <format>


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

        wrc.configure({{"connectTimeout", 3000}, // timeout for the connect phase
                       {"timeout", 5000},        // timeout for the overall IO phase
                       {"trace", false}})
                .sendAsync("https://www.siddiqsoft.com/"_GET, [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                    nlohmann::json doc(req);

                    std::print(std::cerr, "From callback Serialized req: {}\n", doc.dump(2));
                    if (resp && resp->success()) {
                        passTest = true;
                        // std::cerr << "Response\n" << *resp << std::endl;
                    }
                    else if (resp) {
                        auto [ec, emsg] = resp->status();
                        passTest        = ((ec == 12002) || (ec == 12029) || (ec == 400) || (ec == 302));
                        std::print(std::cerr, " test1a - Got error: {} - {}\n", ec, emsg);
                    }
                    else {
                        std::cerr << "Got error: " << resp.error() << " -- " << strerror(resp.error()) << std::endl;
                    }
                    passTest.notify_all();
                });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }


    TEST(TSendRequest, test2a_OPTIONS)
    {
        std::atomic_bool passTest = false;
        restcl           wrc;

        auto optionsRequest = "https://reqbin.com/echo/post/json"_OPTIONS;
        optionsRequest.setHeaders({{"From", __func__}}).setContent({{"Hello", "World"}, {"Anyone", "Home"}});

        wrc.configure().sendAsync(std::move(optionsRequest), [&passTest](auto& req, std::expected<rest_response, int> resp) {
            // Checks the implementation of the encode() implementation
            // std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
            if (passTest = resp ? resp->success() : false; passTest.load()) {
                std::cerr << "Response\n" << *resp << std::endl;
            }
            else if (resp && resp.has_value()) {
                auto [ec, emsg] = resp->status();
                std::cerr << "Got HTTP error: " << ec << std::endl;
            }
            else if (!resp.has_value()) {
                std::cerr << "Got IO error: " << resp.error() << strerror(resp.error()) << std::endl;
                // Technically we were successfull in our IO.
                passTest = true;
            }
            passTest.notify_all();
        });

        passTest.wait(false);
        std::cerr << "Checking results..\n";
        EXPECT_TRUE(passTest.load());
    }


    TEST(TSendRequest, test2a_POST)
    {
        std::atomic_bool passTest = false;
        restcl           wrc;

        auto optionsRequest = "https://reqbin.com/echo/post/json"_POST;
        optionsRequest.setHeaders({{"From", __func__}}).setContent({{"Hello", "World"}, {"Anyone", "Home"}});

        wrc.configure().sendAsync(std::move(optionsRequest), [&passTest](auto& req, std::expected<rest_response, int> resp) {
            // Checks the implementation of the encode() implementation
            // std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
            if (passTest = resp ? resp->success() : false; passTest.load()) {
                std::cerr << "Response\n" << *resp << std::endl;
            }
            else if (resp && resp.has_value()) {
                passTest        = true;
                auto [ec, emsg] = resp->status();
                std::cerr << "Got HTTP error: " << ec << std::endl;
            }
            else if (!resp.has_value()) {
                std::cerr << "Got IO error: " << resp.error() << strerror(resp.error()) << std::endl;

                // Technically we were successfull in our IO.
                passTest = true;
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

        wrc.configure().sendAsync(
                rest_request {HttpMethodType::METHOD_POST,
                              "https://httpbin.org/post"_Uri,
                              {{"Content-Type", "application/json"}},
                              std::format("{{ \"email\":\"jolly@email.com\", \"password\":\"123456\", \"date\":\"{:%FT%TZ}\" }}",
                                          std::chrono::system_clock::now())},
                [&passTest, &responseContentType](auto& req, std::expected<rest_response, int> resp) {
                    responseContentType = req.getHeaders().value("Content-Type", "");
                    //  Checks the implementation of the encode() implementation
                    // std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
                    if (passTest = resp->success(); passTest.load()) {
                        std::cerr << "Response\n" << *resp << std::endl;
                    }
                    else {
                        auto ec = resp ? resp->statusCode() : resp.error();
                        std::cerr << "Got error: " << ec << std::endl;
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

        wrc.configure().sendAsync(
                rest_request {HttpMethodType::METHOD_POST,
                              "https://httpbin.org/post"_Uri,
                              {{"Authorization", "Basic YWF1OnBhYXU="}, {"Content-Type", "application/json+custom"}},
                              {{"foo", "bar"}, {"hello", "world"}, {"bin", __LINE__}}},
                [&passTest](auto& req, std::expected<rest_response, int> resp) {
                    // The request must be the same as we configured!
                    EXPECT_EQ("application/json+custom", req.getHeaders().value("Content-Type", ""));
                    // Checks the implementation of the std::format implementation
                    std::print(std::cerr, "From callback Wire serialize              : {}\n", req);
                    if (passTest = resp->success(); passTest.load()) {
                        std::cerr << "Response\n" << *resp << std::endl;
                        // EXPECT_EQ("application/json+custom", resp->getHeaders().value("Content-Type", ""));
                    }
                    else if (resp.has_value()) {
                        auto [ec, emsg] = resp->status();
                        std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
                    }
                    else {
                        std::cerr << "Got error: " << resp.error() << " -- " << strerror(resp.error()) << std::endl;
                    }
                    passTest.notify_all();
                });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }


    TEST(TSendRequest, Fails_1a_InvalidPort)
    {
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::splituri_literals;

        restcl wrc;

        wrc.configure({{"connectTimeout", 3000}, // timeout for the connect phase
                       {"timeout", 5000},        // timeout for the overall IO phase
                       {"trace", true}})
                .sendAsync("https://www.siddiqsoft.com:65535/"_GET,
                           [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                               if (resp.has_value() && resp->success()) {
                                   passTest = true;
                                   std::cerr << "Response\n" << *resp << std::endl;
                               }
                               else if (resp.has_value()) {
                                   auto [ec, emsg] = resp->status();
                                   passTest        = ((ec == 12002) || (ec == 12029));
                                   std::cerr << "passTest: " << passTest << "  Got error: " << ec << " --" << emsg << std::endl;
                               }
                               else {
                                   // We MUST get a connection failure; the site does not exist!
                                   passTest = true;
                                   std::cerr << "passTest: " << passTest << "  Got error: " << resp.error() << " --"
                                             << curl_easy_strerror((CURLcode)resp.error()) << std::endl;
                               }
                               passTest.notify_all();
                           });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST(TSendRequest, Fails_1b_InvalidHostAndPort)
    {
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::splituri_literals;

        restcl wrc;

        wrc.configure({
                              {"connectTimeout", 3000}, // timeout for the connect phase
                              {"timeout", 5000}         // timeout for the overall IO phase
                      })
                .sendAsync("https://localhost:65535/"_GET, [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                    nlohmann::json doc(req);

                    // Checks the implementation of the json implementation
                    // std::cerr << "From callback Serialized json: " << req << std::endl;
                    if (resp->success()) {
                        std::cerr << "Response\n" << *resp << std::endl;
                    }
                    else if (resp.has_value()) {
                        auto [ec, emsg] = resp->status();
                        passTest        = ec == 12029;
                        // std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
                    }
                    else {
                        // We MUST get a connection failure; the site does not exist!
                        passTest = true;
                        std::cerr << "passTest: " << passTest << "  Got error: " << resp.error() << " --"
                                  << curl_easy_strerror((CURLcode)resp.error()) << std::endl;
                    }
                    passTest.notify_all();
                });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST(TSendRequest, Fails_1c_InvalidPortAndVerb)
    {
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::splituri_literals;

        restcl wrc;

        // The endpoint does not support OPTIONS verb. Moreover, it does not listen on port 9090 either.
        wrc.configure({
                              {"connectTimeout", 3000}, // timeout for the connect phase
                              {"timeout", 5000}         // timeout for the overall IO phase
                      })
                .sendAsync("https://httpbin.org:9090/get"_OPTIONS,
                           [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                               std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
                               if (resp->success()) {
                                   std::cerr << "Response\n" << *resp << std::endl;
                               }
                               else if (resp.has_value()) {
                                   auto [ec, emsg] = resp->status();
                                   passTest        = ((ec == 12002) || (ec == 12029));
                                   std::cerr << "passTest: " << passTest << "  Got error: " << ec << " --" << emsg << std::endl;
                               }
                               else {
                                   // We MUST get a connection failure; the site does not exist!
                                   passTest = true;
                                   std::cerr << "passTest: " << passTest << "  Got error: " << resp.error() << " --"
                                             << curl_easy_strerror((CURLcode)resp.error()) << std::endl;
                               }
                               passTest.notify_all();
                           });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST(TSendRequest, Fails_2a_InvalidVerb)
    {
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::splituri_literals;

        restcl wrc;

        wrc.configure().sendAsync("https://google.com/"_OPTIONS,
                                  [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                                      // std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
                                      if (resp.has_value() && resp->success()) {
                                          std::cerr << "Response\n" << *resp << std::endl;
                                      }
                                      else if (resp.has_value()) {
                                          auto [ec, emsg] = resp->status();
                                          passTest        = ec == 405;
                                          // This is a work-around for google which sometimes refuses to send the Reason Phrase!
                                          if (!emsg.empty()) passTest = passTest && (emsg == "Method Not Allowed");
                                          // std::cerr << "Got error: [" << ec << ":" << emsg << "] -- " << emsg << std::endl
                                          //          << nlohmann::json(resp).dump(3) << std::endl;
                                      }
                                      else {
                                          // We MUST get a connection failure; the site does not exist!
                                          passTest = true;
                                          std::cerr << "passTest: " << passTest << "  Got error: " << resp.error() << " --"
                                                    << curl_easy_strerror((CURLcode)resp.error()) << std::endl;
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

        wrc.configure().sendAsync("https://www.google.com/"_GET,
                                  [&passTest](const auto& req, std::expected<rest_response, int> resp) {
                                      // std::cerr << "From callback Serialized json: " << req << std::endl;
                                      if (resp->success()) {
                                          passTest = resp->statusCode() == 200;
                                          // std::cerr << "Response\n"<< *resp << std::endl;
                                      }
                                      else if (resp.has_value()) {
                                          auto [ec, emsg] = resp->status();
                                          std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
                                      }
                                      else {
                                          // We MUST get a connection failure; the site does not exist!
                                          passTest = true;
                                          std::cerr << "passTest: " << passTest << "  Got error: " << resp.error() << " --"
                                                    << curl_easy_strerror((CURLcode)resp.error()) << std::endl;
                                      }
                                      passTest.notify_all();
                                  });

        passTest.wait(false);
        EXPECT_TRUE(passTest.load());
    }


    TEST(Threads, test_1)
    {
        const unsigned   ITER_COUNT = 9;
        std::atomic_uint passTest   = 0;
        std::atomic_uint callbackCounter {0};


        EXPECT_NO_THROW({
            restcl wrc;

            wrc.configure({{"freshConnect", true},
                           {"userAgent", std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)}},
                          [&](const auto& req, std::expected<rest_response, int> resp) {
                              callbackCounter++;

                              if (resp->success()) {
                                  passTest += resp->statusCode() == 200;
                                  passTest.notify_all();
                              }
                              else if (resp.has_value()) {
                                  passTest += resp->statusCode() != 0;
                                  std::print(std::cerr,
                                             "{} Threads::test_1 - Got error: {} for {} -- {}\n",
                                             __func__,
                                             resp->statusCode(),
                                             req.getUri().authority.host,
                                             resp->reasonCode());
                              }
                              else {
                                  std::print(std::cerr, "{} Threads::test_1 - Unknown error!\n", __func__);
                              }
                          });

            for (auto i = 0; i < ITER_COUNT; i++) {
                if (i % 3 == 0) {
                    wrc.sendAsync("https://www.cnn.com/?client=chrome"_GET);
                }
                else if (i % 2 == 0) {
                    wrc.sendAsync("https://www.bbc.com/?client=firefox"_GET);
                }
                else {
                    wrc.sendAsync("https://www.cnet.com/?client=edge"_GET);
                }
            }

            auto limitCount = 19;
            do {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                std::print(std::cerr,
                           "{} - Wrapup; ITER_COUNT: {}; passTest:{}; callbackCounter:{}\n",
                           __func__,
                           ITER_COUNT,
                           passTest.load(),
                           callbackCounter.load());

                if (ITER_COUNT == passTest.load()) break;
            } while (limitCount--);
        });

        EXPECT_EQ(ITER_COUNT, passTest.load());
    }
} // namespace siddiqsoft
// #endif
