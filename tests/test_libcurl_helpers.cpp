#include <cstdint>
#include <cstdlib>
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

    TEST(libcurl_helpers, test_rest_result_error_curlcode)
    {
        CURLcode          cc {CURLE_ABORTED_BY_CALLBACK};
        rest_result_error rre {cc};
        std::print(std::cerr, "Error code -> {}\n", rest_result_error {cc});
        EXPECT_EQ("Operation was aborted by an application callback", rre.to_string());
    }

    TEST(libcurl_helpers, test_rest_result_error_curlmcode)
    {
        CURLMcode         cc {CURLM_CALL_MULTI_SOCKET};
        rest_result_error rre {cc};
        std::print(std::cerr, "Error code -> {}\n", rest_result_error {cc});
        EXPECT_EQ("Please call curl_multi_perform() soon", rre.to_string());
    }

    TEST(libcurl_helpers, test_rest_result_error_curlhcode)
    {
        CURLHcode          cc {CURLHE_MISSING};
        rest_result_error rre {cc};
        std::print(std::cerr, "Error code -> {}\n", rest_result_error {cc});
        EXPECT_EQ("No such header exists.", rre.to_string());
    }

    TEST(libcurl_helpers, test_rest_result_error_curlshcode)
    {
        CURLSHcode          cc {CURLSHE_BAD_OPTION};
        rest_result_error rre {cc};
        std::print(std::cerr, "Error code -> {}\n", rest_result_error {cc});
        EXPECT_EQ("Unknown share option", rre.to_string());
    }


    TEST(libcurl_helpers, test_rest_result_error_curlucode)
    {
        CURLUcode          cc {CURLUE_BAD_FILE_URL};
        rest_result_error rre {cc};
        std::print(std::cerr, "Error code -> {}\n", rest_result_error {cc});
        EXPECT_EQ("Bad file:// URL", rre.to_string());
    }

    TEST(libcurl_helpers, test_rest_result_error_other)
    {
        uint32_t          cc {EXIT_FAILURE};
        rest_result_error rre {cc};
        std::print(std::cerr, "Error code -> {}\n", rest_result_error {cc});
        EXPECT_EQ("Operation not permitted", rre.to_string());
    }

} // namespace siddiqsoft

#endif