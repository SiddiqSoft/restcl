# restcl Testing Documentation

## Overview

This document provides comprehensive guidance on testing the **restcl** library, including unit tests, integration tests, and testing best practices.

## Table of Contents

1. [Test Architecture](#test-architecture)
2. [Test Organization](#test-organization)
3. [Running Tests](#running-tests)
4. [Test Categories](#test-categories)
5. [Mock Testing](#mock-testing)
6. [Auto-Retry Testing](#auto-retry-testing)
7. [Integration Testing](#integration-testing)
8. [Writing New Tests](#writing-new-tests)
9. [Debugging Tests](#debugging-tests)
10. [Coverage Analysis](#coverage-analysis)

## Test Architecture

### Framework

The project uses **Google Test (gtest)** v1.17.0 for all testing:

```cpp
#include "gtest/gtest.h"

TEST(TestSuite, TestName) {
    // Test implementation
    EXPECT_EQ(expected, actual);
}
```

### Test Organization

```
tests/
├── test_serializers.cpp           # JSON serialization tests
├── test_validation.cpp            # Request/response validation
├── test_postbin.cpp               # Integration tests (external services)
├── test_libcurl_helpers.cpp       # Unix/Linux-specific tests
├── test_restcl.cpp                # Core functionality tests
├── test_edge_cases.cpp            # Edge case tests
├── test_mock_and_coverage.cpp     # Mock-based tests
├── test_auto_retry_scenarios.cpp  # Auto-retry scenario tests
└── results/                       # Test results (XML output)
```

### Build Configuration

Tests are built with:
- **C++23 Standard** - Required for modern features
- **ASAN/Leak Detection** - Address Sanitizer on Linux/Clang Debug builds
- **Code Coverage** - Instrumentation on Debug builds
- **Platform-Specific** - Conditional compilation for Windows/Unix

## Test Organization

### Test Suites

| Suite | File | Purpose | Count |
|-------|------|---------|-------|
| ContentType_Tests | test_mock_and_coverage.cpp | ContentType class functionality | 10 |
| EnumConversions | test_mock_and_coverage.cpp | Enum to_string conversions | 6 |
| RequestLiterals | test_mock_and_coverage.cpp | User-defined literal operators | 1 |
| RestRequest | test_mock_and_coverage.cpp | Request construction/encoding | 6 |
| HttpFrame | test_mock_and_coverage.cpp | HTTP frame operations | 13 |
| RestResponse | test_mock_and_coverage.cpp | Response parsing | 13 |
| MockClient | test_mock_and_coverage.cpp | Mock client functionality | 20 |
| SendAsyncWithRetry_Scenarios | test_auto_retry_scenarios.cpp | Auto-retry scenarios | 18 |
| **TOTAL** | | | **87** |

### Test Types

#### Unit Tests
- Test individual components in isolation
- Use mock objects to eliminate dependencies
- Fast execution (< 1ms per test)
- Examples: ContentType, enum conversions, request encoding

#### Integration Tests
- Test multiple components together
- May use external services (httpbin.org, reqbin.com)
- Slower execution (1-5s per test)
- Examples: test_postbin.cpp, test_restcl.cpp

#### Scenario Tests
- Test real-world use cases
- Comprehensive validation
- Examples: test_auto_retry_scenarios.cpp

## Running Tests

### Build Tests

```bash
# Configure with tests enabled
cmake --preset Apple-Debug -DRESTCL_BUILD_TESTS=ON

# Build all tests
cmake --build --preset Apple-Debug
```

### Run All Tests

```bash
# Run all tests
ctest --preset Apple-Debug

# Run with verbose output
ctest --preset Apple-Debug -V

# Run with output on failure
ctest --preset Apple-Debug --output-on-failure
```

### Run Specific Test Suite

```bash
# Run only auto-retry tests
ctest --preset Apple-Debug -R "SendAsyncWithRetry_Scenarios"

# Run only mock client tests
ctest --preset Apple-Debug -R "MockClient"

# Run only content type tests
ctest --preset Apple-Debug -R "ContentType_Tests"
```

### Run Specific Test

```bash
# Run single test
ctest --preset Apple-Debug -R "TransientNetworkFailure_EventualSuccess"

# Run with verbose output
ctest --preset Apple-Debug -R "TransientNetworkFailure_EventualSuccess" -V
```

### Platform-Specific Presets

```bash
# macOS
cmake --build --preset Apple-Debug
ctest --preset Apple-Debug

# Linux with GCC
cmake --build --preset Linux-GCC-Debug
ctest --preset Linux-GCC-Debug

# Linux with Clang
cmake --build --preset Linux-Clang-Debug
ctest --preset Linux-Clang-Debug

# Windows
cmake --build --preset Windows-Debug
ctest --preset Windows-Debug
```

## Test Categories

### 1. ContentType Tests (10 tests)

**Purpose:** Validate ContentType struct functionality

**Tests:**
- Default construction
- Assignment from JSON
- Assignment from string
- Copy from shared pointer
- Parse from serialized JSON
- JSON serialization
- Formatter support

**Example:**
```cpp
TEST(ContentType_Tests, AssignFromJson) {
    ContentType ct;
    nlohmann::json j = {{"key", "value"}};
    ct = j;
    EXPECT_EQ(CONTENT_APPLICATION_JSON, ct.type);
    EXPECT_FALSE(ct.body.empty());
}
```

### 2. Enum Conversion Tests (6 tests)

**Purpose:** Validate enum to_string conversions and formatters

**Tests:**
- HttpMethodType to_string
- HttpProtocolVersionType to_string
- Formatter support for enums
- JSON roundtrip serialization

**Example:**
```cpp
TEST(EnumConversions, HttpMethodType_to_string) {
    EXPECT_EQ("GET", to_string(HttpMethodType::METHOD_GET));
    EXPECT_EQ("POST", to_string(HttpMethodType::METHOD_POST));
}
```

### 3. Request Literal Tests (1 test)

**Purpose:** Validate user-defined literal operators

**Tests:**
- All HTTP method literals (_GET, _POST, _PUT, _DELETE, etc.)

**Example:**
```cpp
TEST(RequestLiterals, AllVerbs) {
    auto get = "https://example.com/"_GET;
    auto post = "https://example.com/"_POST;
    EXPECT_EQ(HttpMethodType::METHOD_GET, get.getMethod());
}
```

### 4. REST Request Tests (6 tests)

**Purpose:** Validate request construction and encoding

**Tests:**
- Two-argument constructor
- Three-argument constructor
- Four-argument constructor
- Encoding with content
- JSON serialization
- Formatter support

**Example:**
```cpp
TEST(RestRequest, TwoArgConstructor) {
    rest_request req(HttpMethodType::METHOD_GET, 
                     "https://example.com/path?q=1"_Uri);
    EXPECT_EQ(HttpMethodType::METHOD_GET, req.getMethod());
}
```

### 5. HTTP Frame Tests (13 tests)

**Purpose:** Validate HTTP frame operations (headers, content, protocol)

**Tests:**
- setContent error paths
- setContent with various types
- getContentBodyJSON
- encodeContent
- setProtocol
- setMethod
- setHeader
- encodeHeaders
- JSON serialization

**Example:**
```cpp
TEST(HttpFrame, SetContent_EmptyTypeNonEmptyBody) {
    auto req = "https://example.com/"_POST;
    EXPECT_THROW(req.setContent("", "some body"), 
                 std::invalid_argument);
}
```

### 6. REST Response Tests (13 tests)

**Purpose:** Validate response parsing and handling

**Tests:**
- Parse simple OK response
- Parse JSON body
- Parse 404 Not Found
- Parse 301 Redirect
- Parse 500 Server Error
- Parse no body (204)
- Parse no headers (error case)
- Parse multiple headers
- Parse invalid start line
- Parse HTTP/1.0
- Success boundary values
- Status pair extraction
- Encoding and formatting

**Example:**
```cpp
TEST(RestResponse, Parse_SimpleOK) {
    std::string raw = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: text/plain\r\n"
                      "Content-Length: 5\r\n"
                      "\r\n"
                      "hello";
    auto resp = rest_response<>::parse(raw);
    EXPECT_EQ(200u, resp.statusCode());
    EXPECT_EQ("hello", resp.getContentBody());
}
```

### 7. Mock Client Tests (20 tests)

**Purpose:** Validate mock client and basic async operations

**Tests:**
- Configure
- Send success/error
- SendAsync with callbacks
- SendAsyncWithRetry scenarios
- Method chaining
- Multiple sends
- Callback precedence
- Retry count tracking
- X-restcl-Retry header

**Example:**
```cpp
TEST(MockClient, Send_Success) {
    MockRESTClient client;
    client.cannedResponse.setStatus(200, "OK");
    auto req = "https://example.com/"_GET;
    auto result = client.send(req);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(200u, result->statusCode());
}
```

### 8. Auto-Retry Scenario Tests (18 tests)

**Purpose:** Comprehensive testing of auto-retry functionality

**Tests:**
- Transient network failures
- Persistent timeouts
- JSON payload preservation
- Concurrent retries
- Global callback usage
- Inline callback precedence
- Server error handling
- Custom headers preservation
- Different HTTP methods
- Method chaining
- Query parameter preservation
- Large request bodies
- Response header inspection
- Response body parsing
- Error code inspection
- Retry count header tracking
- Zero retries configuration
- Maximum retries configuration

**Example:**
```cpp
TEST(SendAsyncWithRetry_Scenarios, TransientNetworkFailure_EventualSuccess) {
    MockRESTClientForRetry client;
    client.cannedResponse.setStatus(200, "OK");
    client.shouldReturnError = false;
    
    bool callbackInvoked = false;
    client.sendAsyncWithRetry("https://api.example.com/data"_GET,
        [&](auto& req, std::expected<rest_response<>, int> resp) {
            callbackInvoked = true;
            EXPECT_EQ(200u, resp->statusCode());
        });
    
    EXPECT_TRUE(callbackInvoked);
}
```

## Mock Testing

### MockRESTClient

The `MockRESTClient` class provides deterministic testing without network I/O:

```cpp
class MockRESTClient : public basic_restclient<char> {
public:
    rest_response<char> cannedResponse;
    int cannedErrorCode {0};
    bool shouldReturnError {false};
    int sendCallCount {0};
    int sendAsyncCallCount {0};
    int sendAsyncWithRetryCallCount {0};
    // ... implementation
};
```

### Using Mock Client

```cpp
TEST(MyTest, Example) {
    MockRESTClient client;
    
    // Configure mock response
    client.cannedResponse.setStatus(200, "OK");
    client.cannedResponse.setContent({{"key", "value"}});
    client.shouldReturnError = false;
    
    // Execute test
    auto req = "https://api.example.com/data"_GET;
    auto result = client.send(req);
    
    // Verify
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(200u, result->statusCode());
    EXPECT_EQ(1, client.sendCallCount);
}
```

### Mock Features

- **Configurable Responses** - Set status, headers, body
- **Error Simulation** - Simulate network errors
- **Call Tracking** - Count method invocations
- **Callback Support** - Test callback invocation
- **Retry Simulation** - Simulate retry behavior

## Auto-Retry Testing

### Retry Scenarios

The test suite covers comprehensive retry scenarios:

#### 1. Success Scenarios
- Success on first attempt
- Success after retries
- Concurrent retries

#### 2. Failure Scenarios
- Persistent timeouts
- Connection errors
- Max retries exhausted

#### 3. Data Preservation
- JSON payload preservation
- Custom headers preservation
- Query parameters preservation
- Large payloads

#### 4. Callback Behavior
- Global callback usage
- Inline callback precedence
- Response inspection
- Error code capture

#### 5. Configuration
- Zero retries
- High retry counts
- Method chaining

### Example Retry Test

```cpp
TEST(SendAsyncWithRetry_Scenarios, PersistentConnectionTimeout) {
    MockRESTClientForRetry client;
    client.shouldReturnError = true;
    client.cannedErrorCode = 28;  // CURLE_OPERATION_TIMEDOUT
    client.maxRetries = 3;
    
    int attemptCount = 0;
    int finalError = 0;
    
    client.sendAsyncWithRetry("https://slow-server.example.com/api"_GET,
        [&](auto& req, std::expected<rest_response<>, int> resp) {
            attemptCount = client.retryAttempts;
            if (!resp.has_value()) {
                finalError = resp.error();
            }
        });
    
    EXPECT_EQ(28, finalError);
    EXPECT_EQ(4, attemptCount);  // 3 retries + 1 initial
}
```

## Integration Testing

### External Service Tests

Some tests use external services for integration validation:

**Services Used:**
- httpbin.org - HTTP testing service
- reqbin.com - Request/response testing
- Google API - Real-world API testing
- DuckDuckGo - Search API testing

### Running Integration Tests

```bash
# Run all tests including integration tests
ctest --preset Apple-Debug

# Run only integration tests
ctest --preset Apple-Debug -R "postbin"

# Run with timeout (for slow network)
ctest --preset Apple-Debug --timeout 30
```

### Integration Test Example

```cpp
TEST(Integration, FetchFromHttpBin) {
    auto client = GetRESTClient();
    auto req = "https://httpbin.org/get"_GET;
    auto result = client->send(req);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(200u, result->statusCode());
    
    auto json = result->getContentBodyJSON();
    EXPECT_TRUE(json.contains("url"));
}
```

## Writing New Tests

### Test Structure

```cpp
#include "gtest/gtest.h"
#include "../include/siddiqsoft/restcl.hpp"

namespace siddiqsoft {
    using namespace restcl_literals;
    
    // Test Suite
    TEST(MySuite, MyTest) {
        // Arrange
        MockRESTClient client;
        client.cannedResponse.setStatus(200, "OK");
        
        // Act
        auto req = "https://api.example.com/data"_GET;
        auto result = client.send(req);
        
        // Assert
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(200u, result->statusCode());
    }
}
```

### Best Practices

1. **Use Descriptive Names**
   ```cpp
   TEST(SendAsyncWithRetry_Scenarios, TransientNetworkFailure_EventualSuccess)
   // Good - clearly describes what is being tested
   ```

2. **Follow AAA Pattern** - Arrange, Act, Assert
   ```cpp
   // Arrange
   MockRESTClient client;
   client.cannedResponse.setStatus(200, "OK");
   
   // Act
   auto result = client.send(req);
   
   // Assert
   EXPECT_TRUE(result.has_value());
   ```

3. **Use Meaningful Assertions**
   ```cpp
   EXPECT_EQ(200u, result->statusCode());  // Good
   EXPECT_TRUE(result.has_value());        // Good
   // vs
   EXPECT_TRUE(result);  // Unclear
   ```

4. **Test One Thing**
   ```cpp
   // Good - tests one scenario
   TEST(SendAsyncWithRetry_Scenarios, PersistentConnectionTimeout) { ... }
   
   // Bad - tests multiple scenarios
   TEST(SendAsyncWithRetry_Scenarios, AllScenarios) { ... }
   ```

5. **Use Fixtures for Setup**
   ```cpp
   class RetryTestFixture : public ::testing::Test {
   protected:
       MockRESTClientForRetry client;
       
       void SetUp() override {
           client.maxRetries = 3;
       }
   };
   
   TEST_F(RetryTestFixture, MyTest) {
       // Use client fixture
   }
   ```

### Common Assertions

```cpp
// Equality
EXPECT_EQ(expected, actual);
EXPECT_NE(expected, actual);

// Boolean
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// Comparison
EXPECT_LT(a, b);
EXPECT_LE(a, b);
EXPECT_GT(a, b);
EXPECT_GE(a, b);

// String
EXPECT_STREQ("expected", actual);
EXPECT_STRNE("expected", actual);

// Exceptions
EXPECT_THROW(statement, exception_type);
EXPECT_NO_THROW(statement);

// Floating point
EXPECT_FLOAT_EQ(expected, actual);
EXPECT_DOUBLE_EQ(expected, actual);
```

## Debugging Tests

### Enable Verbose Output

```bash
# Show all test output
ctest --preset Apple-Debug -V

# Show output on failure
ctest --preset Apple-Debug --output-on-failure
```

### Run Single Test with Debugging

```bash
# Run specific test
ctest --preset Apple-Debug -R "TestName" -V

# Run with GDB (Unix/Linux)
gdb ./build/Apple-Debug/tests/restcl_tests
(gdb) run --gtest_filter="TestName"
```

### Add Debug Output

```cpp
TEST(MyTest, Debug) {
    MockRESTClient client;
    
    // Add debug output
    std::cout << "Before send" << std::endl;
    auto result = client.send(req);
    std::cout << "After send, has_value: " << result.has_value() << std::endl;
    
    EXPECT_TRUE(result.has_value());
}
```

### Use Breakpoints

```cpp
TEST(MyTest, Breakpoint) {
    MockRESTClient client;
    
    // Set breakpoint here in debugger
    auto result = client.send(req);
    
    EXPECT_TRUE(result.has_value());
}
```

## Coverage Analysis

### Enable Coverage

Coverage is automatically enabled for Debug builds on Linux/Clang:

```bash
cmake --preset Linux-Clang-Debug
cmake --build --preset Linux-Clang-Debug
ctest --preset Linux-Clang-Debug
```

### Generate Coverage Report

```bash
# Generate coverage data
lcov --capture --directory . --output-file coverage.info

# Generate HTML report
genhtml coverage.info --output-directory coverage_report

# View report
open coverage_report/index.html
```

### Coverage Targets

- **Line Coverage:** > 80%
- **Branch Coverage:** > 75%
- **Function Coverage:** > 90%

## Test Results

### Output Format

Tests produce XML output in `tests/results/`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<testsuites tests="87" failures="0" disabled="0" errors="0">
  <testsuite name="ContentType_Tests" tests="10">
    <testcase name="DefaultConstruction" status="run" time="0.001"/>
    <testcase name="AssignFromJson" status="run" time="0.002"/>
  </testsuite>
</testsuites>
```

### CI/CD Integration

Tests are automatically run in Azure Pipelines:

```yaml
- task: CTest@2
  inputs:
    projectDir: '$(Build.SourcesDirectory)'
    testDir: '$(Build.BinariesDirectory)'
    configuration: 'Debug'
    publishTestResults: true
```

## Troubleshooting

### Test Failures

**Issue:** Tests fail with timeout
```bash
# Increase timeout
ctest --preset Apple-Debug --timeout 60
```

**Issue:** Tests fail with network errors
```bash
# Skip integration tests
ctest --preset Apple-Debug -E "postbin"
```

**Issue:** Memory leaks detected
```bash
# Run with ASAN
ASAN_OPTIONS=verbosity=1 ctest --preset Apple-Debug -V
```

### Build Issues

**Issue:** Tests don't compile
```bash
# Clean and rebuild
rm -rf build/
cmake --preset Apple-Debug
cmake --build --preset Apple-Debug
```

**Issue:** Missing dependencies
```bash
# Ensure all dependencies are installed
cmake --preset Apple-Debug
```

## Performance

### Test Execution Time

- **Unit Tests:** < 1ms per test
- **Mock Tests:** 1-10ms per test
- **Integration Tests:** 1-5s per test
- **Total Suite:** ~30-60s

### Optimization

```bash
# Run tests in parallel
ctest --preset Apple-Debug -j 4

# Run only fast tests
ctest --preset Apple-Debug -E "postbin"
```

## See Also

- [API Documentation](API.md) - Complete API reference
- [Best Practices](../best_practices.md) - Project best practices
- [Google Test Documentation](https://google.github.io/googletest/) - gtest reference
