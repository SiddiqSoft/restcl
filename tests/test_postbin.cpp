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
    TEST(postbin, test1a)
    {
        std::atomic_bool passTest = false;
        restcl           wrc;

        wrc.configure({{"trace", true}, {"freshConnection", true}});
        // Get the code..
        rest_request req(HttpMethodType::METHOD_GET,
                         siddiqsoft::Uri(std::format("https://postb.in/{}",
                                                     std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))));
        auto ret = wrc.send(req);
        if (ret.has_value()) {
            std::print(std::cerr, "{}", nlohmann::json(*ret).dump(2));
        }
        else if (ret) {
            // Error..
            std::print(std::cerr, "{} - Error: {} - {}\n", __FUNCTION__, ret.error(), curl_easy_strerror((CURLcode)ret.error()));
        }
    }
} // namespace siddiqsoft
