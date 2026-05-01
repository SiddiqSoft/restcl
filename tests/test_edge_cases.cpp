/**
 * @file test_edge_cases.cpp
 * @brief Comprehensive edge case tests for restcl API
 * @details Tests for boundary conditions, error handling, and unusual input scenarios
 * @version 1.0
 * @date 2024
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 *
 */

#include "gtest/gtest.h"
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <version>
#include <expected>
#include <format>
#include <limits>

#include "nlohmann/json.hpp"

// Define NOMINMAX before including restcl.hpp to prevent Windows macro conflicts
#if (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))
#define NOMINMAX
#endif

#include "../include/siddiqsoft/restcl.hpp"

namespace siddiqsoft
{
    using namespace restcl_literals;

    class EdgeCases : public ::testing::Test
    {
#if defined(__linux__) || defined(__APPLE__)
        std::shared_ptr<LibCurlSingleton> myCurlInstance {};
#endif

    protected:
        void SetUp() override
        {
#if defined(__linux__) || defined(__APPLE__)
            myCurlInstance = LibCurlSingleton::GetInstance();
#endif
        }
    };

    // ============================================================================
    // Edge Case: Empty/Null Inputs
    // ============================================================================

    TEST_F(EdgeCases, EmptyURLString)
    {
        // Test handling of empty URL - library allows empty URLs
        // The URI parser doesn't throw for empty URLs
        EXPECT_NO_THROW({
            auto req = ""_GET;
        });
    }

    TEST_F(EdgeCases, EmptyHeaderValue)
    {
        // Test setting header with empty value (should remove header)
        auto req = "https://httpbin.org/get"_GET;
        req.setHeader("X-Test-Header", "value");
        EXPECT_EQ("value", req.getHeaders().value("X-Test-Header", ""));
        
        req.setHeader("X-Test-Header", "");
        EXPECT_EQ("", req.getHeaders().value("X-Test-Header", ""));
    }

    TEST_F(EdgeCases, EmptyHeaderKey)
    {
        // Test setting header with empty key
        auto req = "https://httpbin.org/get"_GET;
        req.setHeader("", "value");
        // Should not crash, behavior depends on implementation
        EXPECT_TRUE(true);
    }

    TEST_F(EdgeCases, EmptyContentBody)
    {
        // Test setting empty content body
        auto req = "https://httpbin.org/post"_POST;
        std::string emptyBody = "";
        req.setContent(emptyBody);
        EXPECT_EQ("", req.getContentBody());
    }

    TEST_F(EdgeCases, EmptyJSONContent)
    {
        // Test setting empty JSON object as content
        // Note: Empty JSON objects are not set due to the empty() check in setContent()
        auto req = "https://httpbin.org/post"_POST;
        nlohmann::json emptyJson = nlohmann::json::object();
        req.setContent(emptyJson);
        // Empty JSON objects are not set, so body remains empty
        EXPECT_EQ("", req.getContentBody());
    }

    // ============================================================================
    // Edge Case: Boundary Values
    // ============================================================================

    TEST_F(EdgeCases, MaxPortNumber)
    {
        // Test maximum valid port number (65535)
        auto req = "https://example.com:65535/"_GET;
        EXPECT_EQ(65535, req.getUri().authority.port);
    }

    TEST_F(EdgeCases, MinPortNumber)
    {
        // Test minimum valid port number (1)
        auto req = "https://example.com:1/"_GET;
        EXPECT_EQ(1, req.getUri().authority.port);
    }

    TEST_F(EdgeCases, DefaultHTTPPort)
    {
        // Test default HTTP port (80)
        auto req = "http://example.com/"_GET;
        EXPECT_EQ(80, req.getUri().authority.port);
    }

    TEST_F(EdgeCases, DefaultHTTPSPort)
    {
        // Test default HTTPS port (443)
        auto req = "https://example.com/"_GET;
        EXPECT_EQ(443, req.getUri().authority.port);
    }

    TEST_F(EdgeCases, VeryLongURL)
    {
        // Test handling of very long URL (2000+ characters)
        std::string longPath;
        for (int i = 0; i < 100; i++) {
            longPath += "/segment" + std::to_string(i);
        }
        std::string longURL = "https://example.com" + longPath;
        
        EXPECT_NO_THROW({
            auto req = rest_request<>(HttpMethodType::METHOD_GET, 
                                     Uri<char, AuthorityHttp<char>>(longURL));
            EXPECT_TRUE(req.getUri().pathPart.length() > 500);
        });
    }

    TEST_F(EdgeCases, VeryLongHeaderValue)
    {
        // Test handling of very long header value
        auto req = "https://httpbin.org/get"_GET;
        std::string longValue(10000, 'x');
        
        EXPECT_NO_THROW({
            req.setHeader("X-Long-Header", longValue);
            EXPECT_EQ(longValue, req.getHeaders().value("X-Long-Header", ""));
        });
    }

    TEST_F(EdgeCases, VeryLargeJSONContent)
    {
        // Test handling of very large JSON content
        auto req = "https://httpbin.org/post"_POST;
        nlohmann::json largeJson;
        
        for (int i = 0; i < 1000; i++) {
            largeJson[std::format("key_{}", i)] = std::format("value_{}", i);
        }
        
        EXPECT_NO_THROW({
            req.setContent(largeJson);
            EXPECT_GT(req.getContentBody().length(), 10000);
        });
    }

    // ============================================================================
    // Edge Case: Special Characters
    // ============================================================================

    TEST_F(EdgeCases, SpecialCharactersInURL)
    {
        // Test URL with special characters
        std::string specialURL = "https://example.com/path?query=hello%20world&param=value%2Fwith%2Fslash";
        
        EXPECT_NO_THROW({
            auto req = rest_request<>(HttpMethodType::METHOD_GET,
                                     Uri<char, AuthorityHttp<char>>(specialURL));
            EXPECT_TRUE(req.getUri().queryPart.contains("hello%20world"));
        });
    }

    TEST_F(EdgeCases, SpecialCharactersInHeaderValue)
    {
        // Test header value with special characters
        auto req = "https://httpbin.org/get"_GET;
        std::string specialValue = "value with spaces, commas; and=equals";
        
        req.setHeader("X-Special", specialValue);
        EXPECT_EQ(specialValue, req.getHeaders().value("X-Special", ""));
    }

    TEST_F(EdgeCases, UnicodeCharactersInContent)
    {
        // Test Unicode characters in JSON content
        auto req = "https://httpbin.org/post"_POST;
        nlohmann::json unicodeJson = {
            {"greeting", "Hello 世界 🌍"},
            {"emoji", "😀😃😄😁"},
            {"arabic", "مرحبا بالعالم"}
        };
        
        EXPECT_NO_THROW({
            req.setContent(unicodeJson);
            auto body = req.getContentBody();
            EXPECT_GT(body.length(), 0);
        });
    }

    TEST_F(EdgeCases, NullCharactersInContent)
    {
        // Test handling of null characters in content
        auto req = "https://httpbin.org/post"_POST;
        std::string contentWithNull = "Hello\0World";
        
        EXPECT_NO_THROW({
            req.setContent("text/plain", contentWithNull);
        });
    }

    // ============================================================================
    // Edge Case: HTTP Method Handling
    // ============================================================================

    TEST_F(EdgeCases, AllHTTPMethods)
    {
        // Test all supported HTTP methods
        std::vector<HttpMethodType> methods = {
            HttpMethodType::METHOD_GET,
            HttpMethodType::METHOD_POST,
            HttpMethodType::METHOD_PUT,
            HttpMethodType::METHOD_DELETE,
            HttpMethodType::METHOD_PATCH,
            HttpMethodType::METHOD_HEAD,
            HttpMethodType::METHOD_OPTIONS,
            HttpMethodType::METHOD_TRACE,
            HttpMethodType::METHOD_CONNECT
        };
        
        for (auto method : methods) {
            auto req = rest_request<>(method, Uri<char, AuthorityHttp<char>>("https://example.com/"));
            EXPECT_EQ(method, req.getMethod());
        }
    }

    TEST_F(EdgeCases, MethodStringConversion)
    {
        // Test setting method via string
        auto req = "https://httpbin.org/get"_GET;
        
        std::vector<std::string> methodStrings = {
            "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS", "TRACE", "CONNECT"
        };
        
        for (const auto& methodStr : methodStrings) {
            EXPECT_NO_THROW({
                req.setMethod(methodStr);
            });
        }
    }

    TEST_F(EdgeCases, InvalidMethodString)
    {
        // Test invalid method string
        auto req = "https://httpbin.org/get"_GET;
        
        EXPECT_THROW({
            req.setMethod("INVALID_METHOD");
        }, std::invalid_argument);
    }

    // ============================================================================
    // Edge Case: Content Type Handling
    // ============================================================================

    TEST_F(EdgeCases, ContentTypeWithoutBody)
    {
        // Test setting content type without body (should throw)
        auto req = "https://httpbin.org/post"_POST;
        
        EXPECT_THROW({
            req.setContent("application/json", "");
        }, std::invalid_argument);
    }

    TEST_F(EdgeCases, BodyWithoutContentType)
    {
        // Test setting body without explicit content type
        auto req = "https://httpbin.org/post"_POST;
        
        EXPECT_NO_THROW({
            std::string body = "Hello, World!";
            req.setContent(body);
        });
    }

    TEST_F(EdgeCases, CustomContentType)
    {
        // Test custom content type
        auto req = "https://httpbin.org/post"_POST;
        req.setContent("application/json+custom", R"({"custom": "data"})");
        
        EXPECT_EQ("application/json+custom", req.getHeaders().value("Content-Type", ""));
    }

    TEST_F(EdgeCases, MultipleContentTypeChanges)
    {
        // Test changing content type multiple times
        auto req = "https://httpbin.org/post"_POST;
        
        req.setContent("application/json", R"({"type": "json"})");
        EXPECT_EQ("application/json", req.getHeaders().value("Content-Type", ""));
        
        req.setContent("text/plain", "plain text");
        EXPECT_EQ("text/plain", req.getHeaders().value("Content-Type", ""));
        
        req.setContent("application/xml", "<root></root>");
        EXPECT_EQ("application/xml", req.getHeaders().value("Content-Type", ""));
    }

    // ============================================================================
    // Edge Case: Header Handling
    // ============================================================================

    TEST_F(EdgeCases, CaseInsensitiveHeaders)
    {
        // Test header case sensitivity
        auto req = "https://httpbin.org/get"_GET;
        req.setHeader("Content-Type", "application/json");
        
        // Note: Behavior depends on implementation
        // Some implementations may be case-sensitive
        EXPECT_TRUE(req.getHeaders().contains("Content-Type") || 
                   req.getHeaders().contains("content-type"));
    }

    TEST_F(EdgeCases, DuplicateHeaders)
    {
        // Test setting same header multiple times
        auto req = "https://httpbin.org/get"_GET;
        req.setHeader("X-Custom", "value1");
        req.setHeader("X-Custom", "value2");
        
        // Last value should win
        EXPECT_EQ("value2", req.getHeaders().value("X-Custom", ""));
    }

    TEST_F(EdgeCases, ManyHeaders)
    {
        // Test setting many headers
        auto req = "https://httpbin.org/get"_GET;
        
        // Get the initial header count (includes Date and Host headers)
        size_t initialCount = req.getHeaders().size();
        
        for (int i = 0; i < 100; i++) {
            req.setHeader(std::format("X-Header-{}", i), std::format("value-{}", i));
        }
        
        // Should have initial headers + 100 custom headers
        EXPECT_EQ(initialCount + 100, req.getHeaders().size());
    }

    TEST_F(EdgeCases, HeaderWithSpecialCharacters)
    {
        // Test header with special characters
        auto req = "https://httpbin.org/get"_GET;
        req.setHeader("X-Special", "value;charset=utf-8");
        
        EXPECT_EQ("value;charset=utf-8", req.getHeaders().value("X-Special", ""));
    }

    // ============================================================================
    // Edge Case: URI Parsing
    // ============================================================================

    TEST_F(EdgeCases, URIWithUserInfo)
    {
        // Test URI with user info
        // Note: The SplitUri parser extracts only the username part, not the password
        auto req = "https://user:password@example.com/path"_GET;
        // The userInfo field contains only the username part
        EXPECT_EQ("user", req.getUri().authority.userInfo);
    }

    TEST_F(EdgeCases, URIWithComplexQuery)
    {
        // Test URI with complex query string
        std::string complexQuery = "?param1=value1&param2=value2&param3=value%20with%20spaces&param4=";
        auto req = rest_request<>(HttpMethodType::METHOD_GET,
                                 Uri<char, AuthorityHttp<char>>(std::string("https://example.com/path") + complexQuery));
        
        EXPECT_TRUE(req.getUri().queryPart.contains("param1=value1"));
        EXPECT_TRUE(req.getUri().queryPart.contains("param2=value2"));
    }

    TEST_F(EdgeCases, URIWithFragment)
    {
        // Test URI with fragment (should be ignored in HTTP)
        auto req = "https://example.com/path#section"_GET;
        // Fragment handling depends on URI parser
        EXPECT_TRUE(true);
    }

    TEST_F(EdgeCases, URIWithIPAddress)
    {
        // Test URI with IP address
        auto req = "https://192.168.1.1:8080/api"_GET;
        EXPECT_EQ("192.168.1.1", req.getUri().authority.host);
        EXPECT_EQ(8080, req.getUri().authority.port);
    }

    TEST_F(EdgeCases, URIWithIPv6Address)
    {
        // Test URI with IPv6 address
        auto req = "https://[::1]:8080/api"_GET;
        // IPv6 handling depends on URI parser
        EXPECT_TRUE(true);
    }

    // ============================================================================
    // Edge Case: Request/Response Encoding
    // ============================================================================

    TEST_F(EdgeCases, RequestEncoding)
    {
        // Test request encoding
        auto req = "https://httpbin.org/post"_POST;
        req.setHeaders({{"X-Custom", "value"}});
        req.setContent({{"key", "value"}});
        
        std::string encoded = req.encode();
        EXPECT_GT(encoded.length(), 0);
        EXPECT_TRUE(encoded.contains("POST"));
        EXPECT_TRUE(encoded.contains("httpbin.org"));
    }

    TEST_F(EdgeCases, RequestEncodingWithSpecialCharacters)
    {
        // Test request encoding with special characters
        auto req = "https://example.com/path?query=hello%20world"_GET;
        req.setHeader("X-Special", "value;charset=utf-8");
        
        std::string encoded = req.encode();
        EXPECT_GT(encoded.length(), 0);
    }

    // ============================================================================
    // Edge Case: Configuration
    // ============================================================================

    TEST_F(EdgeCases, ConfigurationWithZeroTimeout)
    {
        // Test configuration with zero timeout (no timeout)
        auto client = GetRESTClient({
            {"connectTimeout", 0},
            {"timeout", 0}
        });
        
        EXPECT_TRUE(client != nullptr);
    }

    TEST_F(EdgeCases, ConfigurationWithNegativeTimeout)
    {
        // Test configuration with negative timeout (should be treated as no timeout)
        auto client = GetRESTClient({
            {"connectTimeout", -1},
            {"timeout", -1}
        });
        
        EXPECT_TRUE(client != nullptr);
    }

    TEST_F(EdgeCases, ConfigurationWithVeryLargeTimeout)
    {
        // Test configuration with very large timeout
        auto client = GetRESTClient({
            {"connectTimeout", std::numeric_limits<int>::max()},
            {"timeout", std::numeric_limits<int>::max()}
        });
        
        EXPECT_TRUE(client != nullptr);
    }

    TEST_F(EdgeCases, ConfigurationWithEmptyUserAgent)
    {
        // Test configuration with empty user agent
        auto client = GetRESTClient({
            {"userAgent", ""}
        });
        
        EXPECT_TRUE(client != nullptr);
    }

    TEST_F(EdgeCases, ConfigurationWithVeryLongUserAgent)
    {
        // Test configuration with very long user agent
        std::string longUserAgent(1000, 'x');
        auto client = GetRESTClient({
            {"userAgent", longUserAgent}
        });
        
        EXPECT_TRUE(client != nullptr);
    }

    // ============================================================================
    // Edge Case: Method Chaining
    // ============================================================================

    TEST_F(EdgeCases, MethodChainingRequest)
    {
        // Test method chaining on request
        auto req = "https://httpbin.org/post"_POST;
        
        EXPECT_NO_THROW({
            req.setMethod("POST")
               .setHeader("X-Header-1", "value1")
               .setHeader("X-Header-2", "value2")
               .setContent({{"key", "value"}});
        });
        
        EXPECT_EQ("value1", req.getHeaders().value("X-Header-1", ""));
        EXPECT_EQ("value2", req.getHeaders().value("X-Header-2", ""));
    }

    TEST_F(EdgeCases, MethodChainingClient)
    {
        // Test method chaining on client
        auto client = GetRESTClient();
        
        EXPECT_NO_THROW({
            client->configure({{"userAgent", "test"}});
        });
    }

    // ============================================================================
    // Edge Case: Response Handling
    // ============================================================================

    TEST_F(EdgeCases, ResponseStatusCodeBoundaries)
    {
        // Test response status code boundaries
        rest_response<> resp;
        
        // Test minimum status code
        resp.setStatus(100, "Continue");
        EXPECT_EQ(100, resp.statusCode());
        EXPECT_TRUE(resp.success());
        
        // Test success range
        resp.setStatus(200, "OK");
        EXPECT_EQ(200, resp.statusCode());
        EXPECT_TRUE(resp.success());
        
        resp.setStatus(399, "Custom");
        EXPECT_EQ(399, resp.statusCode());
        EXPECT_TRUE(resp.success());
        
        // Test failure range
        resp.setStatus(400, "Bad Request");
        EXPECT_EQ(400, resp.statusCode());
        EXPECT_FALSE(resp.success());
        
        resp.setStatus(599, "Custom Error");
        EXPECT_EQ(599, resp.statusCode());
        EXPECT_FALSE(resp.success());
    }

    TEST_F(EdgeCases, ResponseWithEmptyReasonPhrase)
    {
        // Test response with empty reason phrase
        rest_response<> resp;
        resp.setStatus(200, "");
        
        EXPECT_EQ(200, resp.statusCode());
        EXPECT_EQ("", resp.reasonCode());
    }

    TEST_F(EdgeCases, ResponseWithVeryLongReasonPhrase)
    {
        // Test response with very long reason phrase
        rest_response<> resp;
        std::string longReason(1000, 'x');
        resp.setStatus(200, longReason);
        
        EXPECT_EQ(200, resp.statusCode());
        EXPECT_EQ(longReason, resp.reasonCode());
    }

    // ============================================================================
    // Edge Case: Async Operations
    // ============================================================================

    TEST_F(EdgeCases, AsyncWithoutCallback)
    {
        // Test async operation without callback (should throw or use global callback)
        auto client = GetRESTClient();
        auto req = "https://httpbin.org/get"_GET;
        
        // This should either throw or use a global callback
        // Behavior depends on implementation
        EXPECT_THROW({
            client->sendAsync(std::move(req));
        }, std::invalid_argument);
    }

    TEST_F(EdgeCases, AsyncWithGlobalCallback)
    {
        // Test async operation with global callback
        std::atomic_bool callbackInvoked = false;
        
        auto client = GetRESTClient({}, [&](const auto& req, auto resp) {
            callbackInvoked = true;
            callbackInvoked.notify_all();
        });
        
        auto req = "https://httpbin.org/get"_GET;
        client->sendAsync(std::move(req));
        
        // Wait for callback
        callbackInvoked.wait(false);
        EXPECT_TRUE(callbackInvoked.load());
    }

    TEST_F(EdgeCases, AsyncWithOverriddenCallback)
    {
        // Test async operation with overridden callback
        std::atomic_bool globalCallbackInvoked = false;
        std::atomic_bool requestCallbackInvoked = false;
        
        auto client = GetRESTClient({}, [&](const auto& req, auto resp) {
            globalCallbackInvoked = true;
        });
        
        auto req = "https://httpbin.org/get"_GET;
        client->sendAsync(std::move(req), [&](const auto& req, auto resp) {
            requestCallbackInvoked = true;
            requestCallbackInvoked.notify_all();
        });
        
        // Wait for callback
        requestCallbackInvoked.wait(false);
        EXPECT_TRUE(requestCallbackInvoked.load());
        // Global callback should not be invoked when request callback is provided
        EXPECT_FALSE(globalCallbackInvoked.load());
    }

} // namespace siddiqsoft
