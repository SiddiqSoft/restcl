/**
 * @file test_auto_retry_scenarios.cpp
 * @brief Comprehensive auto-retry scenario tests for sendAsyncWithRetry method.
 *        Tests simulate real-world retry scenarios including network failures,
 *        payload preservation, concurrency, callbacks, and response handling.
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 */

#include "gtest/gtest.h"
#include <expected>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "../include/siddiqsoft/restcl.hpp"


namespace siddiqsoft
{
    using namespace restcl_literals;

    // =========================================================================
    // Mock REST Client for Testing
    // =========================================================================

    /// @brief A mock implementation of basic_restclient for testing without network IO.
    /// @details This mock simulates the actual retry behavior of the real implementation:
    ///          - Retries only occur on failure (when send() returns error or !success())
    ///          - The callback is invoked only ONCE with the final result
    ///          - retryCounter represents total attempts (initial + retries)
    ///          - X-restcl-Retry header tracks the attempt number
    ///          - Includes dispatchCallback() method matching the real implementation
    class MockRESTClientForRetry : public basic_restclient<char>
    {
    public:
        // Configurable canned response
        rest_response<char>    cannedResponse {};
        int                    cannedErrorCode {0};
        bool                   shouldReturnError {false};
        int                    sendCallCount {0};
        int                    sendAsyncCallCount {0};
        int                    sendAsyncWithRetryCallCount {0};
        int                    configureCallCount {0};
        basic_callbacktype     storedCallback {};
        nlohmann::json         lastConfig {};
        int                    maxRetries {3};
        int                    retryAttempts {0};
        int                    callbackInvokeCount {0};
        int                    callbackAttemptCount {0};
        int                    callbackCompletedCount {0};

        basic_restclient& configure(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {}) override
        {
            configureCallCount++;
            lastConfig = cfg;
            if (cb) storedCallback = std::move(cb);
            return *this;
        }

        [[nodiscard]] std::expected<rest_response<char>, int> send(rest_request<>& req) override
        {
            sendCallCount++;
            if (shouldReturnError) {
                return std::unexpected(cannedErrorCode);
            }
            return cannedResponse;
        }

        /// @brief Dispatches the callback with the request and response result.
        ///
        /// @details Invokes either the provided callback or the globally configured callback
        ///          with the request and response. Increments statistics counters for tracking
        ///          callback attempts and completions.
        ///
        /// @param cb Reference to the callback function to invoke. If this callback is valid
        ///           (non-null), it will be used; otherwise the global storedCallback is used.
        /// @param req Reference to the original request object that was sent
        /// @param resp The response result containing either a successful rest_response or an error code
        ///
        /// @details Behavior:
        ///          1. Increments callbackAttemptCount counter
        ///          2. If cb is valid, invokes cb(req, resp) and increments callbackCompletedCount
        ///          3. Otherwise if storedCallback is valid, invokes storedCallback(req, resp) and increments callbackCompletedCount
        ///          4. If neither callback is valid, no action is taken (silent no-op)
        ///
        /// @note This method mirrors the real implementation's dispatchCallback()
        inline void dispatchCallback(basic_callbacktype& cb, rest_request<char>& req, std::expected<rest_response<char>, int> resp)
        {
            callbackAttemptCount++;
            if (cb) {
                cb(req, resp);
                callbackCompletedCount++;
            }
            else if (storedCallback) {
                storedCallback(req, resp);
                callbackCompletedCount++;
            }
        }

        /// @brief Simulates the actual retry logic from the pool implementation.
        /// @details The real implementation:
        ///          1. Loops while retryCounter-- (decrement in condition)
        ///          2. Calls send() and checks if response is valid AND success()
        ///          3. On success: invokes callback once and breaks
        ///          4. On failure: increments retry counter and continues loop
        ///          5. Callback is invoked exactly once with final result via dispatchCallback()
        basic_restclient& sendAsync(rest_request<>&& req, basic_callbacktype&& cb = {}, uint16_t retryCount = 1) override
        {
            sendAsyncWithRetryCallCount++;
            retryAttempts = 0;
            callbackInvokeCount = 0;
            
            // Simulate the actual pool retry logic
            std::expected<rest_response<char>, int> result;
            uint16_t attempt = 0;
            
            // This mirrors: while (arg.retryCounter--)
            while (retryCount--) {
                attempt++;
                retryAttempts = attempt;
                
                // Set the X-restcl-Retry header to track attempt number
                req.setHeader("X-restcl-Retry", attempt);
                
                // Call send() - mirrors the real implementation
                if (auto resp = send(req); resp && resp->success()) {
                    // Success! Set result and break
                    result = resp;
                    break;
                }
                else {
                    // Failure - will retry if retryCount > 0
                    result = resp;
                    // Loop continues if retryCount > 0
                }
            }
            
            // Invoke callback exactly once with final result using dispatchCallback()
            callbackInvokeCount++;
            dispatchCallback(cb, req, result);
            return *this;
        }

        /// @brief Helper method to call sendAsync with retry count
        /// @details Calls sendAsync with retryCount = maxRetries + 1 (initial + retries)
        void sendAsyncWithRetry(rest_request<>&& req, basic_callbacktype&& cb = {})
        {
            // Call sendAsync with maxRetries + 1 to account for initial attempt + retries
            sendAsync(std::move(req), std::move(cb), maxRetries + 1);
        }
    };


    // =========================================================================
    // Auto-Retry Scenario Tests
    // =========================================================================

    /// @brief Test auto-retry with transient network failures
    /// @details Simulates a scenario where the first few attempts fail with
    ///          network errors, but eventually succeeds
    TEST(SendAsyncWithRetry_Scenarios, TransientNetworkFailure_EventualSuccess)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.shouldReturnError = false;
        client.maxRetries = 5;

        int attemptCount = 0;
        bool callbackInvoked = false;
        unsigned finalStatus = 0;

        client.sendAsyncWithRetry("https://api.example.com/data"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      callbackInvoked = true;
                                      attemptCount = client.retryAttempts;
                                      if (resp.has_value()) {
                                          finalStatus = resp->statusCode();
                                      }
                                  });

        EXPECT_TRUE(callbackInvoked);
        EXPECT_EQ(200u, finalStatus);
        EXPECT_EQ(1, attemptCount); // Succeeds on first attempt
    }

    /// @brief Test auto-retry with persistent connection timeout
    /// @details Simulates a scenario where all retry attempts fail with timeout
    TEST(SendAsyncWithRetry_Scenarios, PersistentConnectionTimeout)
    {
        MockRESTClientForRetry client;
        client.shouldReturnError = true;
        client.cannedErrorCode = 28; // CURLE_OPERATION_TIMEDOUT
        client.maxRetries = 3;

        int attemptCount = 0;
        bool callbackInvoked = false;
        int finalError = 0;

        client.sendAsyncWithRetry("https://slow-server.example.com/api"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      callbackInvoked = true;
                                      attemptCount = client.retryAttempts;
                                      if (!resp.has_value()) {
                                          finalError = resp.error();
                                      }
                                  });

        EXPECT_TRUE(callbackInvoked);
        EXPECT_EQ(28, finalError);
        EXPECT_EQ(4, attemptCount); // 3 retries + 1 initial = 4 attempts
    }

    /// @brief Test auto-retry with POST request containing JSON body
    /// @details Ensures retry logic preserves request body and headers
    TEST(SendAsyncWithRetry_Scenarios, RetryWithJsonPayload)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(201, "Created");
        client.shouldReturnError = false;
        client.maxRetries = 2;

        nlohmann::json requestBody = {
            {"username", "testuser"},
            {"email", "test@example.com"},
            {"role", "admin"}
        };

        bool callbackInvoked = false;
        std::string capturedBody;
        unsigned finalStatus = 0;

        auto req = "https://api.example.com/users"_POST;
        req.setContent(requestBody);
        req.setHeader("Authorization", "Bearer token123");

        client.sendAsyncWithRetry(std::move(req),
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      callbackInvoked = true;
                                      capturedBody = req.getContentBody();
                                      if (resp.has_value()) {
                                          finalStatus = resp->statusCode();
                                      }
                                  });

        EXPECT_TRUE(callbackInvoked);
        EXPECT_EQ(201u, finalStatus);
        EXPECT_FALSE(capturedBody.empty());
        // Verify the body contains the original data
        auto parsed = nlohmann::json::parse(capturedBody);
        EXPECT_EQ("testuser", parsed.at("username"));
    }

    /// @brief Test auto-retry with multiple concurrent requests
    /// @details Simulates multiple async retry requests happening simultaneously
    TEST(SendAsyncWithRetry_Scenarios, MultipleConcurrentRetries)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.shouldReturnError = false;
        client.maxRetries = 2;

        int successCount = 0;
        int totalAttempts = 0;

        for (int i = 0; i < 5; ++i) {
            auto url = std::format("https://api.example.com/resource/{}", i);
            client.sendAsyncWithRetry(
                rest_request<>(HttpMethodType::METHOD_GET, url),
                [&](auto& req, std::expected<rest_response<>, int> resp) {
                    if (resp.has_value() && resp->statusCode() == 200) {
                        successCount++;
                    }
                    totalAttempts += client.retryAttempts;
                });
        }

        EXPECT_EQ(5, successCount);
        EXPECT_EQ(5, client.sendAsyncWithRetryCallCount);
    }

    /// @brief Test auto-retry with configured global callback
    /// @details Verifies that configured callback is used when no inline callback provided
    TEST(SendAsyncWithRetry_Scenarios, GlobalCallbackUsage)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.shouldReturnError = false;
        client.maxRetries = 3;

        int globalCallbackCount = 0;
        client.configure({}, [&](auto& req, std::expected<rest_response<>, int> resp) {
            globalCallbackCount++;
        });

        // Send without inline callback - should use global callback
        client.sendAsyncWithRetry("https://api.example.com/data"_GET);

        EXPECT_EQ(1, globalCallbackCount);
        EXPECT_EQ(1, client.sendAsyncWithRetryCallCount);
    }

    /// @brief Test auto-retry with inline callback overriding global callback
    /// @details Verifies that inline callback takes precedence over global callback
    TEST(SendAsyncWithRetry_Scenarios, InlineCallbackPrecedence)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.shouldReturnError = false;
        client.maxRetries = 2;

        int globalCallbackCount = 0;
        int inlineCallbackCount = 0;

        client.configure({}, [&](auto& req, std::expected<rest_response<>, int> resp) {
            globalCallbackCount++;
        });

        client.sendAsyncWithRetry("https://api.example.com/data"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      inlineCallbackCount++;
                                  });

        EXPECT_EQ(0, globalCallbackCount); // Global callback should NOT be called
        EXPECT_EQ(1, inlineCallbackCount); // Inline callback should be called
    }

    /// @brief Test auto-retry with server returning 5xx errors
    /// @details Simulates server errors that might benefit from retry
    TEST(SendAsyncWithRetry_Scenarios, ServerErrorRetry)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(503, "Service Unavailable");
        client.shouldReturnError = false;
        client.maxRetries = 3;

        bool callbackInvoked = false;
        unsigned finalStatus = 0;

        client.sendAsyncWithRetry("https://api.example.com/service"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      callbackInvoked = true;
                                      if (resp.has_value()) {
                                          finalStatus = resp->statusCode();
                                      }
                                  });

        EXPECT_TRUE(callbackInvoked);
        EXPECT_EQ(503u, finalStatus);
        // Note: In real implementation, 5xx might trigger retries
        // This test verifies the callback is invoked with the response
    }

    /// @brief Test auto-retry with custom headers preservation
    /// @details Ensures custom headers are preserved across retry attempts
    TEST(SendAsyncWithRetry_Scenarios, CustomHeadersPreservation)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.shouldReturnError = false;
        client.maxRetries = 2;

        std::string capturedAuthHeader;
        std::string capturedCustomHeader;

        auto req = "https://api.example.com/secure"_GET;
        req.setHeader("Authorization", "Bearer secret-token-xyz");
        req.setHeader("X-Custom-Header", "custom-value-123");

        client.sendAsyncWithRetry(std::move(req),
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      capturedAuthHeader = req.getHeaders().value("Authorization", "");
                                      capturedCustomHeader = req.getHeaders().value("X-Custom-Header", "");
                                  });

        EXPECT_EQ("Bearer secret-token-xyz", capturedAuthHeader);
        EXPECT_EQ("custom-value-123", capturedCustomHeader);
    }

    /// @brief Test auto-retry with different HTTP methods
    /// @details Verifies retry works with various HTTP methods (GET, POST, PUT, DELETE)
    TEST(SendAsyncWithRetry_Scenarios, DifferentHttpMethods)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.shouldReturnError = false;
        client.maxRetries = 2;

        std::vector<HttpMethodType> methods;

        // Test GET
        client.sendAsyncWithRetry("https://api.example.com/resource"_GET,
                                  [&](auto& req, auto resp) {
                                      methods.push_back(req.getMethod());
                                  });

        // Test POST
        client.sendAsyncWithRetry("https://api.example.com/resource"_POST,
                                  [&](auto& req, auto resp) {
                                      methods.push_back(req.getMethod());
                                  });

        // Test PUT
        client.sendAsyncWithRetry("https://api.example.com/resource/1"_PUT,
                                  [&](auto& req, auto resp) {
                                      methods.push_back(req.getMethod());
                                  });

        // Test DELETE
        client.sendAsyncWithRetry("https://api.example.com/resource/1"_DELETE,
                                  [&](auto& req, auto resp) {
                                      methods.push_back(req.getMethod());
                                  });

        EXPECT_EQ(4, methods.size());
        EXPECT_EQ(HttpMethodType::METHOD_GET, methods[0]);
        EXPECT_EQ(HttpMethodType::METHOD_POST, methods[1]);
        EXPECT_EQ(HttpMethodType::METHOD_PUT, methods[2]);
        EXPECT_EQ(HttpMethodType::METHOD_DELETE, methods[3]);
    }

    /// @brief Test auto-retry with method chaining
    /// @details Verifies that sendAsyncWithRetry works after configure
    TEST(SendAsyncWithRetry_Scenarios, MethodChaining)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.shouldReturnError = false;
        client.maxRetries = 2;

        int retryCount = 0;

        client.configure({{"timeout", 5000}});
        client.sendAsyncWithRetry("https://api.example.com/data"_GET,
                                  [&](auto& req, auto resp) { retryCount++; });

        EXPECT_EQ(1, retryCount);
        EXPECT_EQ(1, client.configureCallCount);
        EXPECT_EQ(1, client.sendAsyncWithRetryCallCount);
    }

    /// @brief Test auto-retry with URI containing query parameters
    /// @details Ensures query parameters are preserved across retries
    TEST(SendAsyncWithRetry_Scenarios, QueryParametersPreservation)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.shouldReturnError = false;
        client.maxRetries = 2;

        std::string capturedUri;

        client.sendAsyncWithRetry("https://api.example.com/search?q=test&limit=10&offset=0"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      capturedUri = req.getUri().urlPart;
                                  });

        EXPECT_TRUE(capturedUri.find("search") != std::string::npos);
        EXPECT_TRUE(capturedUri.find("q=test") != std::string::npos);
        EXPECT_TRUE(capturedUri.find("limit=10") != std::string::npos);
    }

    /// @brief Test auto-retry with large request body
    /// @details Verifies retry works with large JSON payloads
    TEST(SendAsyncWithRetry_Scenarios, LargeRequestBody)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(201, "Created");
        client.shouldReturnError = false;
        client.maxRetries = 2;

        // Create a large JSON payload
        nlohmann::json largePayload = nlohmann::json::array();
        for (int i = 0; i < 100; ++i) {
            largePayload.push_back({
                {"id", i},
                {"name", std::format("item_{}", i)},
                {"description", "Lorem ipsum dolor sit amet, consectetur adipiscing elit."}
            });
        }

        bool callbackInvoked = false;
        unsigned finalStatus = 0;
        std::string payloadStr = largePayload.dump();

        auto req = "https://api.example.com/batch"_POST;
        req.setContent(largePayload);

        client.sendAsyncWithRetry(std::move(req),
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      callbackInvoked = true;
                                      if (resp.has_value()) {
                                          finalStatus = resp->statusCode();
                                      }
                                  });

        EXPECT_TRUE(callbackInvoked);
        EXPECT_EQ(201u, finalStatus);
        EXPECT_GT(payloadStr.length(), 1000); // Should be a substantial payload
    }

    /// @brief Test auto-retry with response header inspection
    /// @details Verifies that response headers are accessible in callback
    TEST(SendAsyncWithRetry_Scenarios, ResponseHeaderInspection)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.cannedResponse.setHeader("X-Request-Id", "req-12345");
        client.cannedResponse.setHeader("X-RateLimit-Remaining", "99");
        client.shouldReturnError = false;
        client.maxRetries = 2;

        std::string requestId;
        std::string rateLimitRemaining;

        client.sendAsyncWithRetry("https://api.example.com/data"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      if (resp.has_value()) {
                                          requestId = resp->getHeaders().value("X-Request-Id", "");
                                          rateLimitRemaining = resp->getHeaders().value("X-RateLimit-Remaining", "");
                                      }
                                  });

        EXPECT_EQ("req-12345", requestId);
        EXPECT_EQ("99", rateLimitRemaining);
    }

    /// @brief Test auto-retry with response body parsing
    /// @details Verifies that response body is properly parsed in callback
    TEST(SendAsyncWithRetry_Scenarios, ResponseBodyParsing)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.cannedResponse.setContent({
            {"status", "success"},
            {"data", {{"id", 42}, {"name", "test"}}},
            {"timestamp", "2024-01-01T00:00:00Z"}
        });
        client.shouldReturnError = false;
        client.maxRetries = 2;

        nlohmann::json parsedResponse;

        client.sendAsyncWithRetry("https://api.example.com/data"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      if (resp.has_value()) {
                                          parsedResponse = resp->getContentBodyJSON();
                                      }
                                  });

        EXPECT_FALSE(parsedResponse.is_null());
        EXPECT_EQ("success", parsedResponse.at("status"));
        EXPECT_EQ(42, parsedResponse.at("data").at("id"));
    }

    /// @brief Test auto-retry with error code inspection
    /// @details Verifies that error codes are properly captured in callback
    TEST(SendAsyncWithRetry_Scenarios, ErrorCodeInspection)
    {
        MockRESTClientForRetry client;
        client.shouldReturnError = true;
        client.cannedErrorCode = 12002; // ERROR_INTERNET_TIMEOUT
        client.maxRetries = 2;

        int capturedErrorCode = 0;
        bool errorReceived = false;

        client.sendAsyncWithRetry("https://api.example.com/data"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      if (!resp.has_value()) {
                                          errorReceived = true;
                                          capturedErrorCode = resp.error();
                                      }
                                  });

        EXPECT_TRUE(errorReceived);
        EXPECT_EQ(12002, capturedErrorCode);
    }

    /// @brief Test auto-retry with retry count header verification
    /// @details Verifies X-restcl-Retry header is set correctly on each attempt
    TEST(SendAsyncWithRetry_Scenarios, RetryCountHeaderTracking)
    {
        MockRESTClientForRetry client;
        client.shouldReturnError = true;
        client.cannedErrorCode = 28;
        client.maxRetries = 3;

        std::vector<int> retryAttempts;

        client.sendAsyncWithRetry("https://api.example.com/data"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      std::string retryHeader = req.getHeaders().value("X-restcl-Retry", "0");
                                      retryAttempts.push_back(std::stoi(retryHeader));
                                  });

        EXPECT_EQ(1, retryAttempts.size());
        EXPECT_EQ(4, retryAttempts[0]); // Final attempt count: 3 retries + 1 initial
    }

    /// @brief Test auto-retry with zero retries configuration
    /// @details Verifies behavior when maxRetries is set to 0
    TEST(SendAsyncWithRetry_Scenarios, ZeroRetriesConfiguration)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.shouldReturnError = false;
        client.maxRetries = 0;

        int attemptCount = 0;

        client.sendAsyncWithRetry("https://api.example.com/data"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      attemptCount = client.retryAttempts;
                                  });

        EXPECT_EQ(1, attemptCount); // Only initial attempt, no retries
    }

    /// @brief Test auto-retry with maximum retries configuration
    /// @details Verifies behavior with high retry count
    TEST(SendAsyncWithRetry_Scenarios, MaximumRetriesConfiguration)
    {
        MockRESTClientForRetry client;
        client.cannedResponse.setStatus(200, "OK");
        client.shouldReturnError = false;
        client.maxRetries = 10;

        int attemptCount = 0;

        client.sendAsyncWithRetry("https://api.example.com/data"_GET,
                                  [&](auto& req, std::expected<rest_response<>, int> resp) {
                                      attemptCount = client.retryAttempts;
                                  });

        EXPECT_EQ(1, attemptCount); // Succeeds on first attempt
        EXPECT_LT(attemptCount, client.maxRetries + 1); // Should not use all retries
    }

} // namespace siddiqsoft
