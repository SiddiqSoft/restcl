#if defined(__linux__) || defined(__APPLE__)
#include "gtest/gtest.h"
#include <iostream>
#include <barrier>
#include <version>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/private/libcurl_helpers.hpp"

namespace siddiqsoft
{
    TEST(libcurl_helpers, test_init)
    {
        LibCurlSingleton ossl;

        // configure
        // start
        // get a context object
        EXPECT_NO_THROW({
            auto ctx = ossl.getEasyHandle();
            EXPECT_TRUE(ctx);
#if defined(DEBUG) || defined(_DEBUG)
            EXPECT_TRUE(ossl.isInitialized);
#endif
        });
    }


} // namespace siddiqsoft

#endif