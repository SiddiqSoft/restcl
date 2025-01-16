#include <memory>
#if defined(__linux__) || defined(__APPLE__)
#include "gtest/gtest.h"
#include <iostream>
#include <barrier>
#include <version>

#include "nlohmann/json.hpp"
#include "../include/siddiqsoft/private/restcl_unix.hpp"

namespace siddiqsoft
{
    TEST(libcurl_helpers, test_init)
    {
        // configure
        // start
        // get a context object
        auto myCurlInstance = LibCurlSingleton::GetInstance();
        EXPECT_TRUE(myCurlInstance);

        EXPECT_NO_THROW({
            auto ctx = myCurlInstance->getEasyHandle();
            // EXPECT_TRUE(ctx);
#if defined(DEBUG) || defined(_DEBUG)
            EXPECT_TRUE(myCurlInstance->isInitialized);
#endif
        });
    }

    TEST(libcurl_helpers, test_result_code)
    {
        CURLcode cc {CURLE_ABORTED_BY_CALLBACK};
        rest_result_error rre{cc};
        std::print("Error code -> {}\n", rest_result_error {cc});
        EXPECT_EQ("Operation was aborted by an application callback", rre.to_string());
    }

} // namespace siddiqsoft

#endif