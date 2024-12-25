#if defined(__linux__) || defined(__APPLE__)
#include "gtest/gtest.h"
#include <iostream>
#include <barrier>
#include <version>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/private/openssl_helpers.hpp"

namespace siddiqsoft
{
    TEST(openssl_helpers, libsslsingleton_test1a)
    {
        LibSSLSingleton ossl;

        // configure
        // start
        // get a context object
        EXPECT_NO_THROW({
            auto ctx = ossl.configure().start().getCTX();
            EXPECT_TRUE(ctx);
#if defined(DEBUG) || defined(_DEBUG)
            EXPECT_TRUE(ossl.isInitialized);
#endif
        });
    }

    TEST(openssl_helpers, libcryptosingleton_test1a)
    {
        LibCryptoSingleton ossl;

        // configure
        // start
        // get a context object
        EXPECT_NO_THROW({
            ossl.configure().start();
#if defined(DEBUG) || defined(_DEBUG)
            EXPECT_TRUE(ossl.isInitialized);
#endif
        });
    }

} // namespace siddiqsoft

#endif