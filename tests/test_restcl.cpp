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
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <iostream>
#include <barrier>
#include <version>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <thread>
#include <vector>


#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/restcl.hpp"
#include "siddiqsoft/ScopeTrace.hpp"
#include "siddiqsoft/RunOnEnd.hpp"

namespace siddiqsoft
{
    using namespace restcl_literals;

    static bool isWellFormedConcurrentUserAgent(const std::string& value, std::string_view prefix)
    {
        if (!value.starts_with(prefix)) return false;

        const auto suffix = value.substr(prefix.size());
        const auto slash  = suffix.find('/');
        if ((slash == std::string::npos) || (slash == 0) || (slash == (suffix.size() - 1))) return false;

        const auto isDigits = [](std::string_view sv) {
            return !sv.empty() && std::all_of(sv.begin(), sv.end(), [](unsigned char ch) { return std::isdigit(ch) != 0; });
        };

        return isDigits(std::string_view {suffix}.substr(0, slash)) && isDigits(std::string_view {suffix}.substr(slash + 1));
    }

    class TestSends : public ::testing::Test
    {
#if defined(__linux__) || defined(__APPLE__)
        std::shared_ptr<LibCurlSingleton> myCurlInstance {};
#endif

    protected:
        void SetUp() override
        {
#if defined(__linux__) || defined(__APPLE__)
            Log.debug("Init the CurlLib singleton.");
            myCurlInstance = LibCurlSingleton::GetInstance();
#endif
        }
    };


    TEST_F(TestSends, test1a)
    {
        std::atomic_bool done     = false;
        std::atomic_bool passTest = false;
        auto             wrc      = GetRESTClient();
        auto             sl0      = Log.sub_scope("test1a", siddiqsoft::LogLevel::trace);

        wrc->configure({{"connectTimeout", 3000}, // timeout for the connect phase
                        {"timeout", 5000},        // timeout for the overall IO phase
                        {"trace", false}})
                .sendAsync("https://www.siddiqsoft.com/"_GET, [&](const auto& req, std::expected<rest_response<>, int> resp) {
                    auto sl = sl0.sub_scope("callback", siddiqsoft::LogLevel::trace);

                    nlohmann::json doc(req);

                    sl.log<siddiqsoft::LogLevel::trace>("From callback Serialized req: {}", doc.dump());
                    if (resp && resp->success()) {
                        passTest = true;
                        sl.log<siddiqsoft::LogLevel::trace>("Response\n{}", *resp);
                    }
                    else if (resp) {
                        auto [ec, emsg] = resp->status();
                        passTest        = ((ec == 12002) || (ec == 12029) || (ec == 400) || (ec == 302));
                        sl.log<siddiqsoft::LogLevel::trace>(" test1a - Got error: {} - {}", ec, emsg);
                    }
                    else {
                        sl.log<siddiqsoft::LogLevel::warning>("Got error: {} -- {}", resp.error(), strerror(resp.error()));
                    }
                    done = true;
                    done.notify_all();
                });

        done.wait(false);
        EXPECT_TRUE(passTest.load());
    }


    TEST_F(TestSends, test1a_OPTIONS)
    {
        std::atomic_bool done     = false;
        std::atomic_bool passTest = false;
        auto             wrc      = GetRESTClient({{"connectTimeout", 3000}, {"timeout", 5000}});

        auto optionsRequest       = "https://reqbin.com/echo/post/json"_OPTIONS;
        optionsRequest.setHeaders({{"From", __func__}}).setContent({{"Hello", "World"}, {"Anyone", "Home"}});

        wrc->sendAsync(std::move(optionsRequest), [&passTest, &done](auto& req, std::expected<rest_response<>, int> resp) {
            // Checks the implementation of the encode() implementation
            // Log.trace("From callback Wire serialize              : {}", req.encode());
            if (passTest = resp ? resp->success() : false; passTest.load()) {
                Log.trace("Response\n{}", *resp);
            }
            else if (resp && resp.has_value()) {
                auto [ec, emsg] = resp->status();
                Log.err("Got HTTP error: {}", ec);
                passTest = true;
            }
            else if (!resp.has_value()) {
                Log.err("Got IO error: {}{}", resp.error(), strerror(resp.error()));
                // Technically we were successfull in our IO.
                passTest = true;
            }
            done = true;
            done.notify_all();
        });

        done.wait(false);
        Log.trace("Checking results..");
        EXPECT_TRUE(passTest.load());
    }


    TEST_F(TestSends, test2a_POST)
    {
        std::atomic_bool done     = false;
        std::atomic_bool passTest = false;
        auto             wrc      = GetRESTClient({{"connectTimeout", 3000}, {"timeout", 5000}});

        auto optionsRequest       = "https://reqbin.com/echo/post/json"_POST;
        optionsRequest.setHeaders({{"From", __func__}}).setContent({{"Hello", "World"}, {"Anyone", "Home"}});

        wrc->configure().sendAsync(std::move(optionsRequest),
                                   [&passTest, &done](auto& req, std::expected<rest_response<>, int> resp) {
                                       // Checks the implementation of the encode() implementation
                                       // Log.trace("From callback Wire serialize              : {}", req.encode());
                                       if (passTest = resp ? resp->success() : false; passTest.load()) {
                                           Log.trace("Response\n{}", *resp);
                                       }
                                       else if (resp && resp.has_value()) {
                                           passTest        = true;
                                           auto [ec, emsg] = resp->status();
                                           Log.err("Got HTTP error: {}", ec);
                                       }
                                       else if (!resp.has_value()) {
                                           Log.err("Got IO error: {}{}", resp.error(), strerror(resp.error()));

                                           // Technically we were successfull in our IO.
                                           passTest = true;
                                       }
                                       done = true;
                                       done.notify_all();
                                   });

        done.wait(false);
        Log.trace("Checking results..");
        EXPECT_TRUE(passTest.load());
    }


    TEST_F(TestSends, test3a)
    {
        using namespace siddiqsoft::splituri_literals;
        std::atomic_bool done     = false;
        std::atomic_bool passTest = false;

        auto        wrc           = GetRESTClient({{"connectTimeout", 3000}, {"timeout", 5000}});
        std::string responseContentType {};

        wrc->sendAsync(rest_request {HttpMethodType::METHOD_POST,
                                     "https://httpbin.org/post"_Uri,
                                     {{"Content-Type", "application/json"}},
                                     std::format(R"({{ "email": "jolly@email.com", "password": "123456", "date": "{:%FT%TZ}" }})",
                                                 std::chrono::system_clock::now())},
                       [&passTest, &done, &responseContentType](auto& req, std::expected<rest_response<>, int> resp) {
                           responseContentType = req.getHeaders().value("Content-Type", "");
                           if (resp.has_value() && resp->success()) {
                               passTest = true;
                               Log.trace("Response\n{}", *resp);
                           }
                           else if (resp.has_value()) {
                               passTest = true;
                               Log.err("Got HTTP error: {}", resp->statusCode());
                           }
                           else {
                               passTest = true;
                               Log.err("Got IO error: {}", resp.error());
                           }
                           done = true;
                           done.notify_all();
                       });

        done.wait(false);
        EXPECT_EQ("application/json", responseContentType);
        EXPECT_TRUE(passTest.load());
    }

    TEST_F(TestSends, test3b)
    {
        using namespace siddiqsoft::splituri_literals;

        std::atomic_bool done     = false;
        std::atomic_bool passTest = false;

        auto wrc                  = GetRESTClient({{"connectTimeout", 3000}, {"timeout", 5000}});

        wrc->configure().sendAsync(
                rest_request {HttpMethodType::METHOD_POST,
                              "https://httpbin.org/post"_Uri,
                              {{"Authorization", "Basic YWF1OnBhYXU="}, {"Content-Type", "application/json+custom"}},
                              {{"foo", "bar"}, {"hello", "world"}, {"bin", __LINE__}}},
                [&passTest, &done](auto& req, std::expected<rest_response<>, int> resp) {
                    // The request must be the same as we configured!
                    EXPECT_EQ("application/json+custom", req.getHeaders().value("Content-Type", ""));
                    // Checks the implementation of the std::format implementation
                    Log.trace("From callback Wire serialize              : {}", req);
                    if (resp.has_value() && resp->success()) {
                        passTest = true;
                        Log.trace("Response\n{}", *resp);
                        // EXPECT_EQ("application/json+custom", resp->getHeaders().value("Content-Type", ""));
                    }
                    else if (resp.has_value()) {
                        passTest        = true;
                        auto [ec, emsg] = resp->status();
                        Log.err("Got error: {} -- {}", ec, emsg);
                    }
                    else {
                        passTest = true;
                        Log.err("Got error: {} -- {}", resp.error(), strerror(resp.error()));
                    }
                    done = true;
                    done.notify_all();
                });

        done.wait(false);
        EXPECT_TRUE(passTest.load());
    }


    TEST_F(TestSends, Fails_1a_InvalidPort)
    {
        std::atomic_bool done     = false;
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::splituri_literals;

        auto wrc = GetRESTClient();

        wrc->configure({{"connectTimeout", 3000}, // timeout for the connect phase
                        {"timeout", 5000},        // timeout for the overall IO phase
                        {"trace", true}})
                .sendAsync("https://www.siddiqsoft.com:65535/"_GET,
                           [&passTest, &done](const auto& req, std::expected<rest_response<>, int> resp) {
                               if (resp.has_value() && resp->success()) {
                                   passTest = true;
                                   Log.trace("Response\n{}", *resp);
                               }
                               else if (resp.has_value()) {
                                   auto [ec, emsg] = resp->status();
                                   passTest        = ((ec == 12002) || (ec == 12029));
                                   Log.err("passTest: {}  Got error: {} --{}", passTest.load(), ec, emsg);
                               }
                               else {
                                   // We MUST get a connection failure; the site does not exist!
                                   passTest = true;
                                   // Log.err("passTest: {}  Got error: {} --{}", passTest.load(), resp.error(), curl_easy_strerror((CURLcode)resp.error()));
                               }
                               done = true;
                               done.notify_all();
                           });

        done.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST_F(TestSends, Fails_1b_InvalidHostAndPort)
    {
        std::atomic_bool done     = false;
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::splituri_literals;

        auto wrc = GetRESTClient({{"connectTimeout", 3000}, {"timeout", 5000}});

        wrc->configure({
                               {"connectTimeout", 3000}, // timeout for the connect phase
                               {"timeout", 5000}         // timeout for the overall IO phase
                       })
                .sendAsync("https://localhost:65535/"_GET,
                           [&passTest, &done](const auto& req, std::expected<rest_response<>, int> resp) {
                               auto sl = gRCL.sub_scope("test_restcl", siddiqsoft::LogLevel::trace);
                               nlohmann::json doc(req);

                               // Checks the implementation of the json implementation
                               Log.trace("From callback Serialized json: {}", req);
                               if (resp.has_value() && resp->success()) {
                                   sl.trace( "Response\n{}", *resp);
                               }
                               else if (resp.has_value()) {
                                   auto [ec, emsg] = resp->status();
                                   passTest        = ec == 12029;
                                   sl.warn( "Got error: {} -- {}", ec, emsg);
                               }
                               else {
                                   // We MUST get a connection failure; the site does not exist!
                                   passTest = true;
                                   sl.warn( "passTest: {}  Got error: {} -- {}", passTest.load(), resp.error(), curl_easy_strerror((CURLcode)resp.error()));
                               }
                               done = true;
                               done.notify_all();
                           });

        done.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST_F(TestSends, Fails_1c_InvalidPortAndVerb)
    {
        std::atomic_bool done     = false;
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::splituri_literals;

        auto wrc = GetRESTClient();

        // The endpoint does not support OPTIONS verb. Moreover, it does not listen on port 9090 either.
        wrc->configure({{"connectTimeout", 3000}, {"timeout", 5000}});
        wrc->sendAsync("https://httpbin.org:9090/get"_OPTIONS,
                       [&passTest, &done](const auto& req, std::expected<rest_response<>, int> resp) {
                           if (resp.has_value() && resp->success()) {
                               Log.trace("Response\n{}", nlohmann::json(*resp).dump());
                           }
                           else if (resp.has_value()) {
                               auto [ec, emsg] = resp->status();
                               passTest        = ((ec == 12002) || (ec == 12029) || (ec == 403));
                               Log.err("ec: {}  Response\n{}", ec, nlohmann::json(*resp).dump());
                           }
                           else {
                               // We MUST get a connection failure; the site does not exist!
                               passTest = true;
                               // Log.err("passTest: {}  Got error: {} --{}", passTest.load(), resp.error(), curl_easy_strerror((CURLcode)resp.error()));
                           }
                           done = true;
                           done.notify_all();
                       });

        done.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST_F(TestSends, Fails_2a_InvalidVerb)
    {
        std::atomic_bool done     = false;
        std::atomic_bool passTest = false;
        using namespace siddiqsoft::splituri_literals;

        auto wrc = GetRESTClient();

        wrc->configure({
                               {"connectTimeout", 3000}, // timeout for the connect phase
                               {"timeout", 5000}         // timeout for the overall IO phase
                       })
                .sendAsync("https://google.com/"_OPTIONS,
                           [&passTest, &done](const auto& req, std::expected<rest_response<char>, int> resp) {
                               // Log.trace("From callback Wire serialize              : {}", req.encode());
                               if (resp.has_value() && resp->success()) {
                                   Log.trace("{} - Response\n{}", __func__, *resp);
                               }
                               else if (resp.has_value()) {
                                   auto [ec, emsg] = resp->status();
                                   passTest        = ec == 405 || ec == 403;
                                   // This is a work-around for google which sometimes refuses to send the Reason Phrase!
                                   if (!emsg.empty()) passTest = passTest && (emsg == "Method Not Allowed");
                                   Log.err("Fails_2a_InvalidVerb - Got error: [{} : {}]\n{}", ec, emsg, *resp);
                               }
                               else {
                                   // We MUST get a connection failure; the site does not exist!
                                   passTest = true;
                                   // Log.err("passTest: {}  Got error: {} --{}", passTest.load(), resp.error(), curl_easy_strerror((CURLcode)resp.error()));
                               }
                               done = true;
                               done.notify_all();
                           });

        done.wait(false);
        EXPECT_TRUE(passTest.load());
    }

    TEST_F(TestSends, test9a)
    {
        std::atomic_bool done     = false;
        std::atomic_bool passTest = false;
        auto             wrc      = GetRESTClient({{"connectTimeout", 3000}, {"timeout", 5000}});

        wrc->sendAsync("https://www.google.com/"_GET,
                       [&passTest, &done](const auto& req, std::expected<rest_response<>, int> resp) {
                           // Log.trace("From callback Serialized json: {}", req);
                           if (resp.has_value() && resp->success()) {
                               passTest = resp->statusCode() == 200;
                               // Log.trace("Response\n{}", *resp);
                           }
                           else if (resp.has_value()) {
                               auto [ec, emsg] = resp->status();
                               Log.err("Got error: {} -- {}", ec, emsg);
                           }
                           else {
                               // We MUST get a connection failure; the site does not exist!
                               passTest = true;
                               // Log.err("passTest: {}  Got error: {} --{}", passTest.load(), resp.error(), curl_easy_strerror((CURLcode)resp.error()));
                           }
                           done = true;
                           done.notify_all();
                       });

        done.wait(false);
        EXPECT_TRUE(passTest.load());
    }


    TEST_F(TestSends, ConcurrentConfigureAndSendDoesNotRace)
    {
        constexpr unsigned       threadCount         = 4;
        constexpr unsigned       iterationsPerThread = 10;
        std::barrier             startBarrier(threadCount + 1);
        std::atomic_uint         completedCallbacks {0};
        std::vector<std::thread> workers;
        workers.reserve(threadCount);

        auto wrc = GetRESTClient({{"trace", false}});

        for (unsigned threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
            workers.emplace_back([&, threadIndex] {
                startBarrier.arrive_and_wait();
                for (unsigned iter = 0; iter < iterationsPerThread; ++iter) {
                    wrc->configure({{"userAgent", std::format("race-test/{}/{}", threadIndex, iter)}});
                    wrc->sendAsync("http://127.0.0.1:1/"_GET, [&](const auto&, std::expected<rest_response<>, int> resp) {
                        (void)resp;
                        completedCallbacks.fetch_add(1, std::memory_order_relaxed);
                    });
                }
            });
        }

        startBarrier.arrive_and_wait();

        for (auto& worker : workers) {
            worker.join();
        }

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(25);
        while (completedCallbacks.load(std::memory_order_relaxed) < (threadCount * iterationsPerThread) &&
               std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_EQ(threadCount * iterationsPerThread, completedCallbacks.load(std::memory_order_relaxed));
    }

#if (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))
    TEST_F(TestSends, ConcurrentConfigureAndSendDoesNotRace_Windows)
    {
        constexpr unsigned       threadCount         = 4;
        constexpr unsigned       iterationsPerThread = 4;
        std::barrier             startBarrier(threadCount + 1);
        std::atomic_uint         completedCallbacks {0};
        std::vector<std::thread> workers;
        workers.reserve(threadCount);

        auto wrc = GetRESTClient({{"trace", false}});

        for (unsigned threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
            workers.emplace_back([&, threadIndex] {
                startBarrier.arrive_and_wait();
                for (unsigned iter = 0; iter < iterationsPerThread; ++iter) {
                    wrc->configure({{"userAgent", std::format("win-race-test/{}/{}", threadIndex, iter)}});
                    wrc->sendAsync("http://127.0.0.1:1/"_GET, [&](const auto&, std::expected<rest_response<>, int> resp) {
                        (void)resp;
                        completedCallbacks.fetch_add(1, std::memory_order_relaxed);
                    });
                }
            });
        }

        startBarrier.arrive_and_wait();

        for (auto& worker : workers) {
            worker.join();
        }

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (completedCallbacks.load(std::memory_order_relaxed) < (threadCount * iterationsPerThread) &&
               std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_GE(completedCallbacks.load(std::memory_order_relaxed), threadCount * iterationsPerThread);
    }

    TEST_F(TestSends, ConcurrentConfigureAndSendAsync_Windows_UserAgentStaysWellFormed)
    {
        constexpr unsigned         threadCount         = 4;
        constexpr unsigned         iterationsPerThread = 8;
        constexpr std::string_view userAgentPrefix {"win-race-test/"};

        std::barrier             startBarrier(threadCount + 1);
        std::atomic_uint         completedCallbacks {0};
        std::atomic_uint         malformedHeaders {0};
        std::vector<std::thread> workers;
        workers.reserve(threadCount);

        auto wrc = GetRESTClient({{"trace", false}});

        for (unsigned threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
            workers.emplace_back([&, threadIndex] {
                startBarrier.arrive_and_wait();
                for (unsigned iter = 0; iter < iterationsPerThread; ++iter) {
                    wrc->configure({{"userAgent", std::format("{}{}/{}", userAgentPrefix, threadIndex, iter)}});
                    wrc->sendAsync("http://127.0.0.1:1/"_GET, [&](const auto& req, std::expected<rest_response<>, int> resp) {
                        //(void)resp;
                        auto header = req.getHeaders().value("User-Agent", "");
                        if (!isWellFormedConcurrentUserAgent(header, userAgentPrefix)) {
                            malformedHeaders.fetch_add(1, std::memory_order_relaxed);
                        }
                        completedCallbacks.fetch_add(1, std::memory_order_relaxed);
                    });
                }
            });
        }

        startBarrier.arrive_and_wait();

        for (auto& worker : workers) {
            worker.join();
        }

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        while (completedCallbacks.load(std::memory_order_relaxed) < (threadCount * iterationsPerThread) &&
               std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        EXPECT_EQ(threadCount * iterationsPerThread, completedCallbacks.load(std::memory_order_relaxed));
        EXPECT_EQ(0u, malformedHeaders.load(std::memory_order_relaxed));
    }

    TEST_F(TestSends, ConcurrentConfigureAndSendSync_Windows_UserAgentStaysWellFormed)
    {
        constexpr unsigned         threadCount         = 4;
        constexpr unsigned         iterationsPerThread = 8;
        constexpr std::string_view userAgentPrefix {"win-sync-race-test/"};

        std::barrier             startBarrier(threadCount + 1);
        std::atomic_uint         malformedHeaders {0};
        std::vector<std::thread> workers;
        workers.reserve(threadCount);

        auto wrc = GetRESTClient({{"trace", false}});

        for (unsigned threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
            workers.emplace_back([&, threadIndex] {
                startBarrier.arrive_and_wait();
                for (unsigned iter = 0; iter < iterationsPerThread; ++iter) {
                    wrc->configure({{"userAgent", std::format("{}{}/{}", userAgentPrefix, threadIndex, iter)}});

                    auto req  = "http://127.0.0.1:1/"_GET;
                    auto resp = wrc->send(req);
                    (void)resp;

                    auto header = req.getHeaders().value("User-Agent", "");
                    if (!isWellFormedConcurrentUserAgent(header, userAgentPrefix)) {
                        malformedHeaders.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }

        startBarrier.arrive_and_wait();

        for (auto& worker : workers) {
            worker.join();
        }

        EXPECT_EQ(0u, malformedHeaders.load(std::memory_order_relaxed));
    }

    TEST_F(TestSends, ConcurrentConfigureAndSendSync_Windows_SessionPublicationIsAtomic)
    {
        auto wrc = GetRESTClient({{"trace", false}});

        wrc->resetSessionForTesting();

        std::atomic_bool hookEntered {false};
        std::atomic_bool releaseHook {false};

        auto cleanup                                = RunOnEnd([&] {
            WinHttpRESTClient::beforePublishSessionHook = {};
            releaseHook.store(true, std::memory_order_release);
            releaseHook.notify_all();
        });

        WinHttpRESTClient::beforePublishSessionHook = [&] {
            hookEntered.store(true, std::memory_order_release);
            hookEntered.notify_all();
            releaseHook.wait(false);
        };

        std::jthread configureThread([&] { wrc->configure({{"userAgent", "win-publication-test/1"}}); });

        hookEntered.wait(false);

        std::expected<rest_response<>, int> resp;
        std::jthread                        sendThread([&] {
            auto req = "http://127.0.0.1:1/"_GET;
            resp     = wrc->send(req);
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        releaseHook.store(true, std::memory_order_release);
        releaseHook.notify_all();

        configureThread.join();
        sendThread.join();

        ASSERT_FALSE(resp.has_value());
        EXPECT_NE(static_cast<int>(E_FAIL), resp.error());
    }

    TEST_F(TestSends, ConcurrentConfigureAndMixedSend_Windows_UserAgentStaysWellFormed)
    {
        constexpr unsigned         threadCount         = 6;
        constexpr unsigned         iterationsPerThread = 8;
        constexpr std::string_view syncPrefix {"win-mixed-sync/"};
        constexpr std::string_view asyncPrefix {"win-mixed-async/"};

        std::barrier             startBarrier(threadCount + 1);
        std::atomic_uint         asyncCallbacks {0};
        std::atomic_uint         malformedHeaders {0};
        std::vector<std::thread> workers;
        workers.reserve(threadCount);

        auto wrc = GetRESTClient({{"trace", false}});

        for (unsigned threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
            workers.emplace_back([&, threadIndex] {
                startBarrier.arrive_and_wait();
                const bool useAsync = (threadIndex % 2) == 0;
                for (unsigned iter = 0; iter < iterationsPerThread; ++iter) {
                    auto prefix = useAsync ? asyncPrefix : syncPrefix;
                    wrc->configure({{"userAgent", std::format("{}{}/{}", prefix, threadIndex, iter)}});

                    if (useAsync) {
                        wrc->sendAsync("http://127.0.0.1:1/"_GET, [&](const auto& req, std::expected<rest_response<>, int> resp) {
                            (void)resp;
                            auto header = req.getHeaders().value("User-Agent", "");
                            if (!isWellFormedConcurrentUserAgent(header, asyncPrefix) &&
                                !isWellFormedConcurrentUserAgent(header, syncPrefix))
                            {
                                malformedHeaders.fetch_add(1, std::memory_order_relaxed);
                            }
                            asyncCallbacks.fetch_add(1, std::memory_order_relaxed);
                        });
                    }
                    else {
                        auto req  = "http://127.0.0.1:1/"_GET;
                        auto resp = wrc->send(req);
                        (void)resp;

                        auto header = req.getHeaders().value("User-Agent", "");
                        if (!isWellFormedConcurrentUserAgent(header, asyncPrefix) &&
                            !isWellFormedConcurrentUserAgent(header, syncPrefix))
                        {
                            malformedHeaders.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                }
            });
        }

        startBarrier.arrive_and_wait();

        for (auto& worker : workers) {
            worker.join();
        }

        const auto expectedAsyncCallbacks = (threadCount / 2 + (threadCount % 2)) * iterationsPerThread;
        auto       deadline               = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (asyncCallbacks.load(std::memory_order_relaxed) < expectedAsyncCallbacks &&
               std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_EQ(expectedAsyncCallbacks, asyncCallbacks.load(std::memory_order_relaxed));
        EXPECT_EQ(0u, malformedHeaders.load(std::memory_order_relaxed));
    }

    TEST_F(TestSends, AsyncCallbacksCanReconfigureWhileOtherThreadsSend_Windows)
    {
        constexpr unsigned         callbackRequestCount = 12;
        constexpr unsigned         syncRequestCount     = 12;
        constexpr std::string_view callbackPrefix {"win-callback-race/"};
        constexpr std::string_view foregroundPrefix {"win-foreground-race/"};

        std::atomic_uint callbackCount {0};
        std::atomic_uint callbackMalformed {0};
        std::atomic_uint syncMalformed {0};

        auto wrc = GetRESTClient({{"trace", false}});

        std::jthread syncWorker([&] {
            for (unsigned iter = 0; iter < syncRequestCount; ++iter) {
                wrc->configure({{"userAgent", std::format("{}{}/{}", foregroundPrefix, 0, iter)}});

                auto req  = "http://127.0.0.1:1/"_GET;
                auto resp = wrc->send(req);
                (void)resp;

                auto header = req.getHeaders().value("User-Agent", "");
                if (!isWellFormedConcurrentUserAgent(header, foregroundPrefix) &&
                    !isWellFormedConcurrentUserAgent(header, callbackPrefix))
                {
                    syncMalformed.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });

        for (unsigned iter = 0; iter < callbackRequestCount; ++iter) {
            wrc->configure({{"userAgent", std::format("{}{}/{}", callbackPrefix, 0, iter)}});
            wrc->sendAsync("http://127.0.0.1:1/"_GET, [&, iter](const auto& req, std::expected<rest_response<>, int> resp) {
                (void)resp;
                auto header = req.getHeaders().value("User-Agent", "");
                if (!isWellFormedConcurrentUserAgent(header, callbackPrefix) &&
                    !isWellFormedConcurrentUserAgent(header, foregroundPrefix))
                {
                    callbackMalformed.fetch_add(1, std::memory_order_relaxed);
                }

                wrc->configure({{"userAgent", std::format("{}{}/{}", callbackPrefix, 1, iter)}});
                callbackCount.fetch_add(1, std::memory_order_relaxed);
            });
        }

        syncWorker.join();

        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (callbackCount.load(std::memory_order_relaxed) < callbackRequestCount && std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_EQ(callbackRequestCount, callbackCount.load(std::memory_order_relaxed));
        EXPECT_EQ(0u, callbackMalformed.load(std::memory_order_relaxed));
        EXPECT_EQ(0u, syncMalformed.load(std::memory_order_relaxed));
    }
#endif

    TEST_F(TestSends, StressSitesParallel)
    {
        const unsigned   ITER_COUNT = 12;
        std::atomic_uint passTest   = 0;
        std::atomic_uint callbackCounter {0};


        EXPECT_NO_THROW({
            auto wrc = GetRESTClient({
                    {"connectTimeout", 3000}, // timeout for the connect phase
                    {"timeout", 5000}         // timeout for the overall IO phase
            });

            wrc->configure({{"freshConnect", true},
                            {"userAgent", std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)}},
                           [&](const auto& req, std::expected<rest_response<>, int> resp) {
                               callbackCounter++;

                               // The stress test validates that all callbacks are invoked
                               // regardless of the HTTP status or IO outcome.
                               if (resp.has_value()) {
                                   passTest++;
                                   if (!resp->success()) {
                                       Log.err("{} Threads::test_1 - HTTP {} for {} -- {}",
                                                    __func__,
                                                    resp->statusCode(),
                                                    req.getUri().authority.host,
                                                    resp->reasonCode());
                                   }
                               }
                               else {
                                   // IO error (connection refused, timeout, etc.) still counts
                                   // as a completed request for the stress test.
                                   passTest++;
                                   Log.err("{} Threads::test_1 - IO error: {} for {}",
                                                __func__,
                                                resp.error(),
                                                req.getUri().authority.host);
                               }
                               passTest.notify_all();
                           });

            for (unsigned i = 0; i < ITER_COUNT; i++) {
                if (i % 3 == 0) {
                    wrc->sendAsync("https://www.cnn.com/?client=chrome"_GET);
                }
                else if (i % 2 == 0) {
                    wrc->sendAsync("https://www.bbc.com/?client=firefox"_GET);
                }
                else {
                    wrc->sendAsync("https://www.cnet.com/?client=edge"_GET);
                }
            }

            auto limitCount = 19;
            do {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                Log.trace("{} - Wrapup; ITER_COUNT: {}; passTest:{}; callbackCounter:{}",
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
