# Auto-Retry Feature Documentation

## Overview

The **auto-retry** feature in restcl provides automatic retry logic for failed HTTP requests. This is essential for building resilient applications that can handle transient network failures, timeouts, and temporary service unavailability.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Feature Overview](#feature-overview)
3. [API Reference](#api-reference)
4. [Configuration](#configuration)
5. [Retry Behavior](#retry-behavior)
6. [Use Cases](#use-cases)
7. [Best Practices](#best-practices)
8. [Examples](#examples)
9. [Testing](#testing)

## Quick Start

### Basic Retry

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

auto client = siddiqsoft::GetRESTClient();

// Send request with automatic retry
client->sendAsyncWithRetry("https://api.example.com/data"_GET,
    [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
        if (resp.has_value()) {
            std::cout << "Success: " << resp->statusCode() << std::endl;
        } else {
            std::cout << "Failed after retries: " << resp.error() << std::endl;
        }
    });
```

### Retry with Configuration

```cpp
auto client = siddiqsoft::GetRESTClient();

client->configure({
    {"maxRetries", 3},
    {"connectTimeout", 3000},
    {"timeout", 5000}
})
.sendAsyncWithRetry("https://api.example.com/data"_GET,
    [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
        // Handle response after retries
    });
```

## Feature Overview

### What is Auto-Retry?

Auto-retry automatically resends failed HTTP requests up to a configured maximum number of times. This helps applications recover from transient failures without manual intervention.

### When to Use Auto-Retry

✅ **Good Use Cases:**
- Network timeouts
- Temporary connection failures
- Service temporarily unavailable (5xx errors)
- Rate limiting (429 errors)
- Transient DNS failures

❌ **Bad Use Cases:**
- Permanent errors (4xx client errors)
- Invalid requests
- Authentication failures
- Malformed data

### Key Features

1. **Automatic Retry** - Transparent retry without intermediate callbacks
2. **Configurable Max Retries** - Control retry attempts (0-10+)
3. **Retry Tracking** - X-restcl-Retry header tracks attempt count
4. **Request Preservation** - Headers, body, and URI preserved across retries
5. **Callback Support** - Global and inline callback support
6. **Method Chaining** - Fluent API for configuration

## API Reference

### sendAsyncWithRetry()

Asynchronous HTTP request execution with automatic retry logic.

```cpp
basic_restclient& sendAsyncWithRetry(
    rest_request<>&& req,
    basic_callbacktype&& cb = {}
);
```

**Parameters:**
- `req` - Rvalue reference to rest_request object
- `cb` - Optional callback function (overrides global callback)

**Returns:** Reference to self for method chaining

**Callback Signature:**
```cpp
void callback(rest_request<>& req, 
              std::expected<rest_response<>, int> resp);
```

**Callback Parameters:**
- `req` - Reference to the request (with X-restcl-Retry header)
- `resp` - Response or error code

### Configuration Options

```cpp
client->configure({
    {"maxRetries", 3},           // Max retry attempts (default: 3)
    {"connectTimeout", 3000},    // Connection timeout in ms
    {"timeout", 5000},           // Overall timeout in ms
    {"userAgent", "MyApp/1.0"},  // User-Agent header
    {"trace", false}             // Enable trace logging
});
```

## Retry Behavior

### Retry Logic

1. **Initial Attempt** - Send request
2. **Check Result** - Evaluate response/error
3. **Retry Decision** - Decide whether to retry
4. **Retry Attempt** - Resend request (if retrying)
5. **Final Callback** - Invoke callback with final result

### Retry Conditions

Retries occur when:
- Network error occurs (connection timeout, connection refused, etc.)
- Request fails and retry attempts remain

Retries do NOT occur when:
- Request succeeds (status 100-399)
- Max retries exhausted
- Callback is invoked only after final result

### X-restcl-Retry Header

The library automatically adds the `X-restcl-Retry` header to track retry attempts:

```
X-restcl-Retry: 1  // First attempt
X-restcl-Retry: 2  // First retry
X-restcl-Retry: 3  // Second retry
X-restcl-Retry: 4  // Third retry (if maxRetries=3)
```

### Retry Attempt Count

With `maxRetries=3`:
- Initial attempt: 1
- Retry 1: 2
- Retry 2: 3
- Retry 3: 4
- **Total attempts: 4**

## Configuration

### Global Configuration

```cpp
auto client = siddiqsoft::GetRESTClient();

client->configure({
    {"maxRetries", 3},
    {"connectTimeout", 3000},
    {"timeout", 5000}
});

// All subsequent requests use this configuration
client->sendAsyncWithRetry(std::move(req), callback);
```

### Per-Request Override

```cpp
// Global configuration
client->configure({{"maxRetries", 3}});

// Per-request callback (overrides global callback)
client->sendAsyncWithRetry(std::move(req),
    [](auto& req, auto resp) {
        // This callback is used instead of global callback
    });
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| maxRetries | int | 3 | Maximum retry attempts |
| connectTimeout | int | 3000 | Connection timeout (ms) |
| timeout | int | 5000 | Overall timeout (ms) |
| userAgent | string | "restcl/1.0" | User-Agent header |
| trace | bool | false | Enable trace logging |
| freshConnect | bool | false | Force new connection |

## Use Cases

### Use Case 1: Resilient API Client

```cpp
class ApiClient {
private:
    std::shared_ptr<siddiqsoft::basic_restclient<char>> client;
    
public:
    ApiClient() {
        client = siddiqsoft::GetRESTClient();
        client->configure({
            {"maxRetries", 3},
            {"connectTimeout", 3000},
            {"timeout", 5000}
        });
    }
    
    void fetchData(const std::string& url) {
        using namespace siddiqsoft::restcl_literals;
        
        auto req = (url + "?format=json")_GET;
        req.setHeader("Authorization", "Bearer token123");
        
        client->sendAsyncWithRetry(std::move(req),
            [this](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
                if (resp.has_value()) {
                    handleSuccess(resp.value());
                } else {
                    handleError(resp.error());
                }
            });
    }
    
private:
    void handleSuccess(const siddiqsoft::rest_response<>& resp) {
        std::cout << "Success: " << resp.statusCode() << std::endl;
    }
    
    void handleError(int error) {
        std::cerr << "Error: " << error << std::endl;
    }
};
```

### Use Case 2: Batch Processing with Retry

```cpp
void processBatch(const std::vector<nlohmann::json>& items) {
    auto client = siddiqsoft::GetRESTClient();
    
    client->configure({{"maxRetries", 5}});
    
    for (const auto& item : items) {
        using namespace siddiqsoft::restcl_literals;
        
        auto req = "https://api.example.com/process"_POST;
        req.setContent(item);
        
        client->sendAsyncWithRetry(std::move(req),
            [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
                if (resp.has_value()) {
                    std::cout << "Item processed" << std::endl;
                } else {
                    std::cerr << "Failed to process item" << std::endl;
                }
            });
    }
}
```

### Use Case 3: Conditional Retry Logic

```cpp
void smartRetry(const std::string& url) {
    auto client = siddiqsoft::GetRESTClient();
    
    using namespace siddiqsoft::restcl_literals;
    auto req = (url)_GET;
    
    client->sendAsyncWithRetry(std::move(req),
        [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
            if (resp.has_value()) {
                auto status = resp->statusCode();
                
                if (status >= 200 && status < 300) {
                    std::cout << "Success" << std::endl;
                } else if (status >= 500) {
                    std::cout << "Server error (may retry)" << std::endl;
                } else if (status >= 400) {
                    std::cout << "Client error (no retry)" << std::endl;
                }
            } else {
                std::cout << "Network error: " << resp.error() << std::endl;
            }
        });
}
```

## Best Practices

### 1. Set Appropriate Retry Count

```cpp
// Good - reasonable retry count
client->configure({{"maxRetries", 3}});

// Bad - too many retries
client->configure({{"maxRetries", 100}});

// Bad - no retries
client->configure({{"maxRetries", 0}});
```

### 2. Use Timeouts

```cpp
// Good - set both connection and overall timeout
client->configure({
    {"connectTimeout", 3000},
    {"timeout", 5000}
});

// Bad - no timeout (may hang indefinitely)
client->configure({});
```

### 3. Handle Errors Properly

```cpp
// Good - check for errors
client->sendAsyncWithRetry(std::move(req),
    [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
        if (resp.has_value()) {
            // Handle success
        } else {
            // Handle error
        }
    });

// Bad - ignore errors
client->sendAsyncWithRetry(std::move(req),
    [](auto& req, auto resp) {
        // No error handling
    });
```

### 4. Use Move Semantics

```cpp
// Good - move request
client->sendAsyncWithRetry(std::move(req), callback);

// Bad - copy request
auto req_copy = req;
client->sendAsyncWithRetry(std::move(req_copy), callback);
```

### 5. Keep Callbacks Lightweight

```cpp
// Good - lightweight callback
client->sendAsyncWithRetry(std::move(req),
    [](auto& req, auto resp) {
        if (resp.has_value()) {
            std::cout << "Success" << std::endl;
        }
    });

// Bad - heavy processing in callback
client->sendAsyncWithRetry(std::move(req),
    [](auto& req, auto resp) {
        // Heavy computation
        for (int i = 0; i < 1000000; ++i) {
            // ...
        }
    });
```

### 6. Configure Once

```cpp
// Good - configure once at startup
auto client = siddiqsoft::GetRESTClient();
client->configure({{"maxRetries", 3}});

// Use client for multiple requests
client->sendAsyncWithRetry(std::move(req1), callback1);
client->sendAsyncWithRetry(std::move(req2), callback2);

// Bad - reconfigure for each request
for (auto& req : requests) {
    auto client = siddiqsoft::GetRESTClient();
    client->configure({{"maxRetries", 3}});
    client->sendAsyncWithRetry(std::move(req), callback);
}
```

## Examples

### Example 1: Simple Retry

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    
    client->sendAsyncWithRetry("https://api.example.com/data"_GET,
        [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
            if (resp.has_value()) {
                std::cout << "Status: " << resp->statusCode() << std::endl;
            } else {
                std::cerr << "Error: " << resp.error() << std::endl;
            }
        });
    
    // Keep application alive for async callback
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    return 0;
}
```

### Example 2: Retry with Configuration

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    
    client->configure({
        {"maxRetries", 5},
        {"connectTimeout", 3000},
        {"timeout", 10000}
    })
    .sendAsyncWithRetry("https://api.example.com/data"_GET,
        [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
            if (resp.has_value()) {
                auto json = resp->getContentBodyJSON();
                std::cout << "Data: " << json.dump() << std::endl;
            }
        });
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}
```

### Example 3: Retry with POST

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    
    auto req = "https://api.example.com/users"_POST;
    req.setContent({
        {"name", "John Doe"},
        {"email", "john@example.com"}
    });
    req.setHeader("Authorization", "Bearer token123");
    
    client->configure({{"maxRetries", 3}})
          .sendAsyncWithRetry(std::move(req),
              [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
                  if (resp.has_value()) {
                      auto json = resp->getContentBodyJSON();
                      std::cout << "Created user ID: " << json.at("id") << std::endl;
                  }
              });
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}
```

### Example 4: Retry with Error Handling

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    
    client->configure({{"maxRetries", 3}})
          .sendAsyncWithRetry("https://api.example.com/data"_GET,
              [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
                  if (resp.has_value()) {
                      auto& response = resp.value();
                      
                      if (response.success()) {
                          std::cout << "Success: " << response.statusCode() << std::endl;
                      } else {
                          std::cerr << "HTTP Error: " << response.statusCode() 
                                    << " " << response.reasonCode() << std::endl;
                      }
                  } else {
                      int error = resp.error();
                      std::cerr << "Network Error: " << error << std::endl;
                  }
              });
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}
```

## Testing

### Unit Tests

The library includes comprehensive unit tests for auto-retry functionality:

```bash
# Run auto-retry tests
ctest --preset Apple-Debug -R "SendAsyncWithRetry_Scenarios"

# Run specific test
ctest --preset Apple-Debug -R "TransientNetworkFailure_EventualSuccess"
```

### Test Coverage

The test suite covers:
- ✅ Transient network failures
- ✅ Persistent timeouts
- ✅ Request payload preservation
- ✅ Concurrent retries
- ✅ Callback behavior
- ✅ HTTP methods
- ✅ Query parameters
- ✅ Response inspection
- ✅ Error codes
- ✅ Retry count tracking
- ✅ Configuration options

### Writing Retry Tests

```cpp
TEST(MyRetryTest, Example) {
    MockRESTClientForRetry client;
    client.cannedResponse.setStatus(200, "OK");
    client.shouldReturnError = false;
    client.maxRetries = 3;
    
    bool callbackInvoked = false;
    
    client.sendAsyncWithRetry("https://api.example.com/data"_GET,
        [&](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
            callbackInvoked = true;
            EXPECT_TRUE(resp.has_value());
        });
    
    EXPECT_TRUE(callbackInvoked);
}
```

## Limitations

- **No Exponential Backoff** - Retries happen immediately
- **No Jitter** - No randomization in retry timing
- **No Circuit Breaker** - No automatic failure detection
- **No Retry Budget** - No global retry limit across requests
- **Synchronous Retries** - Retries happen in sequence, not parallel

## Future Enhancements

Potential improvements for future versions:
- Exponential backoff with jitter
- Circuit breaker pattern
- Retry budget management
- Conditional retry logic
- Custom retry strategies

## See Also

- [API Documentation](API.md) - Complete API reference
- [Testing Documentation](TESTING.md) - Testing guide
- [Best Practices](../best_practices.md) - Project best practices
