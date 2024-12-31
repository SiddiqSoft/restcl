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

        // EXPECT_NO_THROW({
        auto doc = srt.dump(3);
        std::cerr << doc << std::endl;
        //});

        // EXPECT_NO_THROW({
        //  nlohmann::json doc(srt);
        // nlohmann::json doc2 {std::move(srt)};
        nlohmann::json doc2 = srt;
        // Checks the implementation of the json implementation
        std::cerr << "Serialized json: " << doc2.dump(3) << std::endl;
        //});
    }

    TEST(Serializers, test_make_rest_request_POST)
    {
        auto srt = make_rest_request(HttpMethodType::POST, std::format("https://www.siddiqsoft.com/echo?from={}", __func__));
        std::cerr << "Going to check if we can dump.." << std::endl;

        // EXPECT_NO_THROW({
        auto doc = srt.dump(3);
        std::cerr << doc << std::endl;
        //});
        srt.setContent(CONTENT_APPLICATION_TEXT, "Hello");

        std::cerr << srt.dump(3) << std::endl;

        // EXPECT_NO_THROW({
        //  nlohmann::json doc(srt);
         nlohmann::json doc2 {std::move(srt)};
        //nlohmann::json doc2 = srt;
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
