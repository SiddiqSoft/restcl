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

    static LibCurlSingleton g_PostBin;
    class PostBin : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            std::print(std::cerr, "{} - Init the CurlLib singleton.\n", __func__);
            g_PostBin.configure().start();
        }
    };

    
    static std::string CreateBinId()
    {
        restcl wrc;

        rest_request req(HttpMethodType::METHOD_POST, siddiqsoft::Uri(std::format("https://www.postb.in/api/bin")));
        // ),{{HF_ACCEPT, CONTENT_APPLICATION_JSON}, {HF_CONTENT_TYPE, CONTENT_JSON}});
        if (auto ret = wrc.configure({{"trace", false}, {"freshConnect", true}}).send(req); ret.has_value()) {
            auto doc = ret->getContentBodyJSON();
            return doc.at("binId");
        }
        return {};
    }


    TEST_F(PostBin, multiple_simultaneously)
    {
        const unsigned   ITER_COUNT = 9;
        std::atomic_uint passTest   = 0;
        std::atomic_uint callbackCounter {0};

        // First we should get and store the session id..
        SessionBinId = CreateBinId();
        EXPECT_FALSE(SessionBinId.empty());

        EXPECT_NO_THROW({
            restcl wrc;

            wrc.configure({{"freshConnect", true},
                           {"userAgent", std::format("siddiqsoft.restcl.tests/1.0 (Windows NT; x64; s:{})", __FUNCTION__)}},
                          [&](const auto& req, std::expected<rest_response, int> resp) {
                              callbackCounter++;

                              if (resp->success()) {
                                  passTest += resp->statusCode() >= 200;
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
                    rest_request req(HttpMethodType::METHOD_GET,
                                     siddiqsoft::Uri(std::format("https://www.postb.in/api/bin/{}?iteration=3--{}", SessionBinId, i)));
                    wrc.sendAsync(std::move(req));
                }
                else if (i % 2 == 0) {
                    rest_request req(HttpMethodType::METHOD_GET,
                                     siddiqsoft::Uri(std::format("https://www.postb.in/{}?iteration=2--{}", SessionBinId, i)));
                    wrc.sendAsync(std::move(req));
                }
                else {
                    rest_request req(HttpMethodType::METHOD_GET,
                                     siddiqsoft::Uri(std::format("https://www.postb.in/{}?iteration=1--{}", SessionBinId, i)));
                    wrc.sendAsync(std::move(req));
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
