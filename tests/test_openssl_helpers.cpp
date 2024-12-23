#if defined(__linux__) || defined(__APPLE__)
#include "gtest/gtest.h"
#include <iostream>
#include <barrier>
#include <version>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/openssl_helpers.hpp"

namespace siddiqsoft
{
    using namespace restcl_literals;

    TEST(openssl_helpers, test1a)
    {
        OpenSSLSingleton ossl;

        ossl.configure().start();
        auto ctx = ossl.getCTX();
        EXPECT_TRUE(ctx);
    }
} // namespace siddiqsoft

#endif