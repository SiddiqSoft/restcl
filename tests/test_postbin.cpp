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


#include <chrono>
#include <iostream>
#include <barrier>
#include <version>
#include <expected>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "../include/siddiqsoft/restcl.hpp"

#include "gtest/gtest.h"

namespace siddiqsoft
{
    static std::string SessionBinId {};


    class PostBin : public ::testing::Test
    {
        // std::shared_ptr<LibCurlSingleton> myCurlInstance;

    protected:
        void SetUp() override
        {
            // std::print(std::cerr, "{} - Init the CurlLib singleton.\n", __func__);
            //  configure
            //  start
            //  get a context object
            // myCurlInstance = LibCurlSingleton::GetInstance();
        }

    public:
        auto listResources() -> nlohmann::json
        {
            // https://designer.mocky.io/design
            auto wrc = CreateRESTClient({{"trace", false}, {"freshConnect", true}});

            rest_request req {HttpMethodType::METHOD_GET,
                              siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts")),
                              {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}}};

            if (auto ret = wrc->send(req); ret.has_value()) {
                if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) {
                    return doc;
                }
            }

            return nullptr;
        }

        auto createResource(const nlohmann::json& d) -> nlohmann::json
        {
            // https://designer.mocky.io/design
            auto wrc = CreateRESTClient({{"trace", false}, {"freshConnect", true}});

            rest_request req {HttpMethodType::METHOD_POST,
                              siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts")),
                              {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}}};
            req.setContent(d);
            if (auto ret = wrc->send(req); ret.has_value()) {
                if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) {
                    return doc;
                }
            }

            return nullptr;
        }

        auto updateResource(const std::string id, const nlohmann::json& d) -> nlohmann::json
        {
            // https://designer.mocky.io/design
            auto wrc = CreateRESTClient({{"trace", false}, {"freshConnect", true}});

            rest_request req {HttpMethodType::METHOD_PUT,
                              siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts/{}", id)),
                              {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}}};
            req.setContent(d);
            if (auto ret = wrc->send(req); ret.has_value()) {
                if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) {
                    return doc;
                }
            }

            return nullptr;
        }

        auto patchResource(const std::string id, const nlohmann::json& d) -> nlohmann::json
        {
            // https://designer.mocky.io/design
            auto wrc = CreateRESTClient({{"trace", false}, {"freshConnect", true}});

            rest_request req {HttpMethodType::METHOD_PATCH,
                              siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts/{}", id)),
                              {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}}};
            req.setContent(d);
            if (auto ret = wrc->send(req); ret.has_value()) {
                if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) {
                    return doc;
                }
            }

            return nullptr;
        }

        auto deleteResource(const std::string id) -> nlohmann::json
        {
            // https://designer.mocky.io/design
            auto wrc = CreateRESTClient({{"trace", false}, {"freshConnect", true}});

            rest_request req {HttpMethodType::METHOD_DELETE,
                              siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts/{}", id)),
                              {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}}};
            if (auto ret = wrc->send(req); ret.has_value()) {
                if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) {
                    return doc;
                }
            }

            return nullptr;
        }
    };


    static std::string CreateBinId()
    {
        auto wrc = CreateRESTClient({{"trace", false}, {"freshConnect", true}});

        rest_request req {HttpMethodType::METHOD_POST,
                          siddiqsoft::Uri(std::format("https://www.postb.in/api/bin")),
                          {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}}};

        if (auto ret = wrc->send(req); ret.has_value()) {
            if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) return doc.at("binId");
        }

        return {};
    }

    TEST_F(PostBin, verb_GET_1)
    {
        // https://designer.mocky.io/design
        auto wrc = CreateRESTClient({{"trace", false}, {"freshConnect", true}});

        rest_request req {HttpMethodType::METHOD_GET,
                          siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts/1")),
                          {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}}};

        if (auto ret = wrc->send(req); ret.has_value()) {
            std::println(std::cerr, "{} - Raw response:\n{}", __func__, nlohmann::json(*ret).dump(2));
            if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) {
                std::println(std::cerr, "{} - Response:\n{}", __func__, doc.dump(2));
                EXPECT_EQ(1, doc.at("userId"));
            }
        }
    }


    TEST_F(PostBin, verb_GET_All)
    {
        // https://designer.mocky.io/design
        auto wrc = CreateRESTClient({{"trace", false}, {"freshConnect", true}});

        rest_request req {HttpMethodType::METHOD_GET,
                          siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts")),
                          {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}}};

        if (auto ret = wrc->send(req); ret.has_value()) {
            std::println(std::cerr, "{} - Raw response:\n{}", __func__, nlohmann::json(*ret).dump(2));
            if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) {
                // std::println(std::cerr, "{} - Response:\n{}", __func__, doc.dump(2));
                EXPECT_TRUE(doc.is_array());
                std::println(std::cerr, "{} - Array Size: {}", __func__, doc.size());
                EXPECT_TRUE(doc.size() > 1);
            }
        }
    }

    TEST_F(PostBin, verb_POST_1)
    {
        // https://jsonplaceholder.typicode.com/guide/
        auto wrc = CreateRESTClient({{"trace", false}, {"freshConnect", true}});

        rest_request req {
                HttpMethodType::METHOD_POST,
                siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts/1")),
                {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}, {HF_EXPECT, nullptr}},
                {{"id", 1}, {"title", "foo"}, {"body", "foobar"}, {"userId", 1}, {"source", __func__}, {"index", __COUNTER__}}};

        if (auto ret = wrc->send(req); ret.has_value()) {
            std::println(std::cerr, "{} - Raw response:\n{}", __func__, nlohmann::json(*ret).dump(2));
            if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) {
                std::println(std::cerr, "{} - Response:\n{}", __func__, doc.dump(2));
                EXPECT_EQ(1, doc.at("id"));
            }
            else {
                EXPECT_FALSE(true) << ret.error();
            }
        }
        else {
            EXPECT_FALSE(true) << ret.error();
        }
    }


    TEST_F(PostBin, verb_PUT_1)
    {
        // https://jsonplaceholder.typicode.com/guide/
        auto wrc = CreateRESTClient({{"trace", true}, {"freshConnect", true}});

        rest_request req {
                HttpMethodType::METHOD_PUT,
                siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts/1")),
                {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}, {HF_EXPECT, nullptr}},
                {{"id", 1}, {"title", "foo"}, {"body", "foobar"}, {"userId", 1}, {"source", __func__}, {"index", __COUNTER__}}};

        if (auto ret = wrc->send(req); ret.has_value()) {
            std::println(std::cerr, "{} - Raw response:\n{}", __func__, nlohmann::json(*ret).dump(2));
            if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) {
                std::println(std::cerr, "{} - Response:\n{}", __func__, doc.dump(2));
                EXPECT_EQ(1, doc.at("id"));
            }
            else {
                EXPECT_FALSE(true) << ret.error();
            }
        }
        else {
            EXPECT_FALSE(true) << ret.error();
        }
    }

    TEST_F(PostBin, verb_PATCH_1)
    {
        // https://designer.mocky.io/design
        auto wrc = CreateRESTClient({{"trace", true}, {"freshConnect", true}});

        rest_request req {HttpMethodType::METHOD_PATCH,
                          siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts/1")),
                          {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}},
                          {{"title", __func__}}};

        if (auto ret = wrc->send(req); ret.has_value()) {
            std::println(std::cerr, "{} - Raw response:\n{}", __func__, nlohmann::json(*ret).dump(2));
            if (auto doc = ret->getContentBodyJSON(); !doc.empty() && !doc.is_null()) {
                std::println(std::cerr, "{} - Response:\n{}", __func__, doc.dump(2));
            }
            else {
                EXPECT_FALSE(true) << ret.error();
            }
        }
        else {
            EXPECT_FALSE(true) << ret.error();
        }
    }

    TEST_F(PostBin, verb_DELETE_1)
    {
        // https://designer.mocky.io/design
        auto wrc = CreateRESTClient({{"trace", false}, {"freshConnect", true}});

        rest_request req {HttpMethodType::METHOD_DELETE,
                          siddiqsoft::Uri(std::format("https://jsonplaceholder.typicode.com/posts/1")),
                          {{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_APPLICATION_JSON}}};

        if (auto ret = wrc->send(req); ret.has_value()) {
            std::println(std::cerr, "{} - Raw response:\n{}", __func__, nlohmann::json(*ret).dump(2));
            if (ret.has_value()) {
                std::println(std::cerr, "{} - StatusCode:{}", __func__, (*ret).statusCode());
                EXPECT_EQ(200, ret->statusCode()) << ret.error();
            }
            else {
                EXPECT_FALSE(true) << ret.error();
            }
        }
        else if (ret.has_value()) {
            std::println(std::cerr, "{} - StatusCode:{}", __func__, (*ret).statusCode());
            EXPECT_EQ(200, ret->statusCode()) << ret.error();
        }
    }

    TEST_F(PostBin, multiple_simultaneously_1)
    {
        const unsigned   ITER_COUNT = 1;
        std::atomic_uint passTest   = 0;
        std::atomic_uint callbackCounter {0};

        // First we should get and store the session id..
        SessionBinId = CreateBinId();
        EXPECT_FALSE(SessionBinId.empty());

        EXPECT_NO_THROW({
            restcl wrc = CreateRESTClient();

            wrc->configure({{"freshConnect", true},
                            {"userAgent", std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)}},
                           [&](const auto& req, std::expected<rest_response, int> resp) {
                               callbackCounter++;

                               if (resp->success()) {
                                   passTest += resp->statusCode() >= 200;
                                   passTest.notify_all();
                               }
                               else if (resp.has_value()) {
                                   passTest += resp->statusCode() != 0;
                                   /*std::print(std::cerr,
                                              "{} Threads::test_1 - Got error: {} for {} -- {}\n",
                                              __func__,
                                              resp->statusCode(),
                                              req.getUri().authority.host,
                                              resp->reasonCode());*/
                               }
                               else {
                                   std::print(std::cerr, "{} Threads::test_1 - Unknown error!\n", __func__);
                               }
                           });
            /*
                        rest_request req(HttpMethodType::METHOD_GET,
                                         siddiqsoft::Uri(std::format("https://www.postb.in/api/bin/{}?iteration=000",
               SessionBinId))); wrc->sendAsync(std::move(req));
            */
            wrc->sendAsync(
                    rest_request {HttpMethodType::METHOD_GET,
                                  siddiqsoft::Uri(std::format("https://www.postb.in/api/bin/{}?iteration=000", SessionBinId))});

            auto limitCount = ITER_COUNT;
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


    TEST_F(PostBin, multiple_simultaneously_2)
    {
        const unsigned   ITER_COUNT = 9;
        std::atomic_uint passTest   = 0;
        std::atomic_uint callbackCounter {0};

        // First we should get and store the session id..
        SessionBinId = CreateBinId();
        EXPECT_FALSE(SessionBinId.empty());

        EXPECT_NO_THROW({
            restcl wrc = CreateRESTClient();

            wrc->configure({{"freshConnect", true},
                            {"userAgent", std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)}},
                           [&](const auto& req, std::expected<rest_response, int> resp) {
                               callbackCounter++;

                               if (resp->success()) {
                                   passTest += resp->statusCode() >= 200;
                                   passTest.notify_all();
                               }
                               else if (resp.has_value()) {
                                   passTest += resp->statusCode() != 0;
                                   /*std::print(std::cerr,
                                              "{} Threads::test_1 - Got error: {} for {} -- {}\n",
                                              __func__,
                                              resp->statusCode(),
                                              req.getUri().authority.host,
                                              resp->reasonCode());*/
                               }
                               else {
                                   std::print(std::cerr, "{} Threads::test_1 - Unknown error!\n", __func__);
                               }
                           });

            for (auto i = 0; i < ITER_COUNT; i++) {
                if (i % 3 == 0) {
                    rest_request req(
                            HttpMethodType::METHOD_GET,
                            siddiqsoft::Uri(std::format("https://www.postb.in/api/bin/{}?iteration=3--{}", SessionBinId, i)));
                    wrc->sendAsync(std::move(req));
                }
                else if (i % 2 == 0) {
                    rest_request req(HttpMethodType::METHOD_GET,
                                     siddiqsoft::Uri(std::format("https://www.postb.in/{}?iteration=2--{}", SessionBinId, i)));
                    wrc->sendAsync(std::move(req));
                }
                else {
                    rest_request req(HttpMethodType::METHOD_GET,
                                     siddiqsoft::Uri(std::format("https://www.postb.in/{}?iteration=1--{}", SessionBinId, i)));
                    wrc->sendAsync(std::move(req));
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
