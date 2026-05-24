# restcl API Documentation

## Overview

**restcl** is a modern C++23 header-only REST client library that provides a clean, JSON-first API for interacting with RESTful servers. The library uses native platform implementations (WinHTTP on Windows, libcurl on Unix/Linux/macOS) to abstract away low-level HTTP details while maintaining a modern C++ interface.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Core Concepts](#core-concepts)
3. [API Reference](#api-reference)
4. [Advanced Usage](#advanced-usage)
5. [Error Handling](#error-handling)
6. [Configuration](#configuration)
7. [Examples](#examples)

## Quick Start

[Checkout the class diagram](./RESTCL_CLASS_DIAGRAM.md)

### Basic GET Request

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

// Create a GET request using user-defined literal
auto req = "https://api.example.com/users"_GET;

// Get a REST client
auto client = siddiqsoft::GetRESTClient();

// Send synchronous request
auto result = client->send(req);

if (result.has_value()) {
    auto& response = result.value();
    std::cout << "Status: " << response.statusCode() << std::endl;
    std::cout << "Body: " << response.getContentBody() << std::endl;
} else {
    std::cerr << "Error: " << result.error() << std::endl;
}
```

### Basic POST Request with JSON

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

// Create a POST request with JSON body
auto req = "https://api.example.com/users"_POST;
req.setContent({
    {"name", "John Doe"},
    {"email", "john@example.com"},
    {"role", "admin"}
});

// Send request
auto client = siddiqsoft::GetRESTClient();
auto result = client->send(req);

if (result.has_value()) {
    auto json = result->getContentBodyJSON();
    std::cout << "Created user ID: " << json.at("id") << std::endl;
}
```

### Asynchronous Request with Callback

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

auto client = siddiqsoft::GetRESTClient();

client->sendAsync("https://api.example.com/data"_GET,
    [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
        if (resp.has_value()) {
            std::cout << "Response received: " << resp->statusCode() << std::endl;
        } else {
            std::cerr << "Request failed with error: " << resp.error() << std::endl;
        }
    });
```

## Core Concepts

### REST Request Model

The `rest_request<CharT>` class represents an HTTP request with:
- **HTTP Method** - GET, POST, PUT, DELETE, HEAD, PATCH, OPTIONS, TRACE, CONNECT
- **URI** - Parsed into components (scheme, authority, path, query)
- **Headers** - HTTP headers as key-value pairs
- **Content** - Request body with content type

```cpp
rest_request<char> req(
    HttpMethodType::METHOD_POST,
    "https://api.example.com/users",
    {{"Authorization", "Bearer token123"}},
    {{"name", "John"}}
);
```

### REST Response Model

The `rest_response<CharT>` class represents an HTTP response with:
- **Status Code** - HTTP status (200, 404, 500, etc.)
- **Reason Code** - HTTP reason phrase ("OK", "Not Found", etc.)
- **Headers** - Response headers
- **Content** - Response body with automatic JSON parsing

```cpp
auto response = rest_response<>::parse(rawHttpResponse);
std::cout << "Status: " << response.statusCode() << std::endl;
std::cout << "Reason: " << response.reasonCode() << std::endl;

// Automatic JSON parsing for application/json content type
auto json = response.getContentBodyJSON();
```

### User-Defined Literals (UDLs)

The library provides convenient UDLs for creating requests:

```cpp
using namespace siddiqsoft::restcl_literals;

auto get_req = "https://api.example.com/users"_GET;
auto post_req = "https://api.example.com/users"_POST;
auto put_req = "https://api.example.com/users/1"_PUT;
auto delete_req = "https://api.example.com/users/1"_DELETE;
auto patch_req = "https://api.example.com/users/1"_PATCH;
auto head_req = "https://api.example.com/users"_HEAD;
auto options_req = "https://api.example.com/users"_OPTIONS;
auto trace_req = "https://api.example.com/users"_TRACE;
auto connect_req = "https://api.example.com/users"_CONNECT;
```

### Content Type Handling

The `ContentType` struct manages request/response bodies:

```cpp
// Automatic JSON content type
req.setContent({{"key", "value"}});  // Sets Content-Type: application/json

// Custom content type
req.setContent("text/plain", "raw text body");

// Shared pointer for advanced scenarios
auto ct = std::make_shared<ContentType>();
ct->type = "application/xml";
ct->body = "<root/>";
req.setContent(ct);
```

### Error Handling with std::expected

The library uses `std::expected<T, E>` for error handling instead of exceptions:

```cpp
auto result = client->send(req);

if (result.has_value()) {
    // Success path
    auto& response = result.value();
    std::cout << "Status: " << response.statusCode() << std::endl;
} else {
    // Error path
    int error_code = result.error();
    std::cerr << "Error code: " << error_code << std::endl;
}
```

## API Reference

### GetRESTClient()

Factory function to obtain a REST client instance.

```cpp
std::shared_ptr<basic_restclient<char>> client = GetRESTClient();
```

**Returns:** Shared pointer to platform-specific REST client implementation
- Windows: `WinHttpRESTClient`
- Unix/Linux/macOS: `HttpRESTClient`

### basic_restclient Interface

#### configure()

Configure global settings and callback.

```cpp
client->configure(
    {
        {"connectTimeout", 3000},
        {"timeout", 5000},
        {"userAgent", "MyApp/1.0"},
        {RESTCL_CONFIG_TRACE, false}
    },
    [](auto& req, std::expected<rest_response<>, int> resp) {
        // Global callback
    }
);
```

**Parameters:**
- `cfg` - JSON configuration object
- `cb` - Optional global callback function

**Returns:** Reference to self for method chaining

#### send()

Synchronous HTTP request execution.

```cpp
auto result = client->send(req);
```

**Parameters:**
- `req` - Reference to rest_request object

**Returns:** `std::expected<rest_response<>, int>` - Response or error code

#### sendAsync()

Asynchronous HTTP request execution.

```cpp
client->sendAsync(std::move(req), [](auto& req, auto resp) {
    // Handle response
});
```

**Parameters:**
- `req` - Rvalue reference to rest_request object
- `cb` - Optional callback function

**Returns:** Reference to self for method chaining

#### sendAsync()

Asynchronous HTTP request execution with optional automatic retry logic.

```cpp
// Simple async without retry
client->sendAsync(std::move(req), [](auto& req, auto resp) {
    // Handle response
});

// Async with retry (retryCount > 1)
client->sendAsync(std::move(req), [](auto& req, auto resp) {
    // Handle response after retries
}, 4);  // 3 retries + 1 initial attempt
```

**Parameters:**
- `req` - Rvalue reference to rest_request object
- `cb` - Optional callback function
- `retryCount` - Number of total attempts (default: 1, no retry)

**Returns:** Reference to self for method chaining

**Features:**
- Non-blocking async operations
- Optional automatic retry on failure
- Configurable retry attempts via retryCount parameter
- X-restcl-Retry header tracking (when retryCount > 1)
- Transparent retry without intermediate callbacks
- Request/response preservation across retries

### rest_request Methods

#### setMethod()

Set HTTP method.

```cpp
req.setMethod("POST");
req.setMethod(HttpMethodType::METHOD_POST);
```

#### setHeader()

Set or remove HTTP header.

```cpp
req.setHeader("Authorization", "Bearer token123");
req.setHeader("X-Custom", "value");
req.setHeader("X-Custom", "");  // Remove header
```

#### setContent()

Set request body with content type.

```cpp
// JSON content
req.setContent({{"key", "value"}});

// Text content
req.setContent("text/plain", "raw text");

// Shared pointer
req.setContent(std::make_shared<ContentType>(...));
```

#### getContentBody()

Get request body as string.

```cpp
std::string body = req.getContentBody();
```

#### getContentBodyJSON()

Get request body as parsed JSON.

```cpp
nlohmann::json json = req.getContentBodyJSON();
```

#### encode()

Encode request as HTTP frame.

```cpp
std::string httpFrame = req.encode();
```

### rest_response Methods

#### parse()

Parse HTTP response from raw string.

```cpp
auto response = rest_response<>::parse(rawHttpResponse);
```

#### statusCode()

Get HTTP status code.

```cpp
unsigned code = response.statusCode();
```

#### reasonCode()

Get HTTP reason phrase.

```cpp
std::string reason = response.reasonCode();
```

#### success()

Check if response indicates success (100-399).

```cpp
if (response.success()) {
    // Handle success
}
```

#### getContentBody()

Get response body as string.

```cpp
std::string body = response.getContentBody();
```

#### getContentBodyJSON()

Get response body as parsed JSON.

```cpp
nlohmann::json json = response.getContentBodyJSON();
```

#### getHeaders()

Get response headers.

```cpp
auto headers = response.getHeaders();
std::string contentType = headers.value("Content-Type", "");
```

## Advanced Usage

### Method Chaining

All methods return references for fluent API:

```cpp
client->configure({{"timeout", 5000}})
      .sendAsync("https://api.example.com/data"_GET,
                 [](auto& req, auto resp) {
                     // Handle response
                 });
```

### Custom Headers

```cpp
auto req = "https://api.example.com/secure"_GET;
req.setHeader("Authorization", "Bearer eyJhbGc...");
req.setHeader("X-Request-ID", "12345");
req.setHeader("Accept", "application/json");
```

### Query Parameters

```cpp
auto req = "https://api.example.com/search?q=test&limit=10&offset=0"_GET;
```

### Large Payloads

```cpp
nlohmann::json largePayload = nlohmann::json::array();
for (int i = 0; i < 1000; ++i) {
    largePayload.push_back({
        {"id", i},
        {"name", std::format("item_{}", i)}
    });
}

auto req = "https://api.example.com/batch"_POST;
req.setContent(largePayload);
```

### Response Header Inspection

```cpp
client->sendAsync("https://api.example.com/data"_GET,
    [](auto& req, std::expected<rest_response<>, int> resp) {
        if (resp.has_value()) {
            auto headers = resp->getHeaders();
            std::string requestId = headers.value("X-Request-Id", "");
            std::string rateLimit = headers.value("X-RateLimit-Remaining", "");
            std::cout << "Request ID: " << requestId << std::endl;
            std::cout << "Rate Limit: " << rateLimit << std::endl;
        }
    });
```

### Retry with Configuration

```cpp
client->configure({
    {"connectTimeout", 3000},
    {"timeout", 5000},
    {"maxRetries", 3}
});

client->sendAsyncWithRetry("https://api.example.com/data"_GET,
    [](auto& req, std::expected<rest_response<>, int> resp) {
        if (resp.has_value()) {
            std::cout << "Success after retries" << std::endl;
        } else {
            std::cout << "Failed after all retries" << std::endl;
        }
    });
```

## Error Handling

### Error Codes

**Windows (WinHTTP):**
- 12001: ERROR_INTERNET_OUT_OF_HANDLES
- 12002: ERROR_INTERNET_TIMEOUT
- 12029: ERROR_INTERNET_CANNOT_CONNECT
- 12030: ERROR_INTERNET_CONNECTION_ABORTED

**Unix/Linux/macOS (libcurl):**
- 7: CURLE_COULDNT_CONNECT
- 28: CURLE_OPERATION_TIMEDOUT
- 35: CURLE_SSL_CONNECT_ERROR

### Handling Errors

```cpp
auto result = client->send(req);

if (!result.has_value()) {
    int error_code = result.error();
    
    switch (error_code) {
        case 28:  // Timeout
            std::cerr << "Request timed out" << std::endl;
            break;
        case 7:   // Connection error
            std::cerr << "Connection failed" << std::endl;
            break;
        default:
            std::cerr << "Error: " << error_code << std::endl;
    }
}
```

### Validation Errors

Validation errors throw `std::invalid_argument`:

```cpp
try {
    auto req = "https://api.example.com/data"_POST;
    req.setContent("application/json", "");  // Throws: empty body with content type
} catch (const std::invalid_argument& e) {
    std::cerr << "Validation error: " << e.what() << std::endl;
}
```

## Configuration

### Global Configuration

```cpp
client->configure({
    {"connectTimeout", 3000},      // Connection timeout in ms
    {"timeout", 5000},             // Overall timeout in ms
    {"userAgent", "MyApp/1.0"},    // User-Agent header
    {RESTCL_CONFIG_TRACE, false},              // Enable trace logging
    {"freshConnect", false},       // Force new connection
    {"maxRetries", 3}              // Max retry attempts
});
```

### Per-Request Configuration

```cpp
auto req = "https://api.example.com/data"_GET;
req.setHeader("User-Agent", "MyApp/1.0");
req.setHeader("Accept", "application/json");
```

## Examples

### Example 1: Fetch User Data

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    
    auto req = "https://api.example.com/users/123"_GET;
    auto result = client->send(req);
    
    if (result.has_value()) {
        auto json = result->getContentBodyJSON();
        std::cout << "User: " << json.at("name") << std::endl;
        std::cout << "Email: " << json.at("email") << std::endl;
    } else {
        std::cerr << "Failed to fetch user" << std::endl;
    }
    
    return 0;
}
```

### Example 2: Create Resource with Retry

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    
    client->configure({{"maxRetries", 3}})
          .sendAsyncWithRetry("https://api.example.com/users"_POST,
              [](auto& req, std::expected<siddiqsoft::rest_response<>, int> resp) {
                  if (resp.has_value()) {
                      auto json = resp->getContentBodyJSON();
                      std::cout << "Created user ID: " << json.at("id") << std::endl;
                  } else {
                      std::cerr << "Failed to create user" << std::endl;
                  }
              });
    
    // Keep application alive for async callback
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    return 0;
}
```

### Example 3: Batch Operations

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    
    // Create batch payload
    nlohmann::json batch = nlohmann::json::array();
    for (int i = 0; i < 100; ++i) {
        batch.push_back({
            {"id", i},
            {"name", std::format("item_{}", i)},
            {"value", i * 10}
        });
    }
    
    auto req = "https://api.example.com/batch"_POST;
    req.setContent(batch);
    req.setHeader("Authorization", "Bearer token123");
    
    auto result = client->send(req);
    
    if (result.has_value()) {
        std::cout << "Batch processed successfully" << std::endl;
    }
    
    return 0;
}
```

### Example 4: Error Handling

```cpp
#include "siddiqsoft/restcl.hpp"
using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    
    auto req = "https://api.example.com/data"_GET;
    auto result = client->send(req);
    
    if (result.has_value()) {
        auto& response = result.value();
        
        if (response.success()) {
            std::cout << "Success: " << response.statusCode() << std::endl;
        } else {
            std::cerr << "HTTP Error: " << response.statusCode() 
                      << " " << response.reasonCode() << std::endl;
        }
    } else {
        int error = result.error();
        std::cerr << "Network Error: " << error << std::endl;
    }
    
    return 0;
}
```

## Best Practices

1. **Use User-Defined Literals** - Cleaner and more readable
   ```cpp
   auto req = "https://api.example.com/data"_GET;  // Good
   // vs
   rest_request<> req(HttpMethodType::METHOD_GET, "https://api.example.com/data");  // Verbose
   ```

2. **Handle Errors Properly** - Always check `std::expected` result
   ```cpp
   if (result.has_value()) {
       // Handle success
   } else {
       // Handle error
   }
   ```

3. **Use Async for I/O-Bound Operations** - Avoid blocking
   ```cpp
   client->sendAsync(std::move(req), callback);  // Non-blocking
   // vs
   client->send(req);  // Blocking
   ```

4. **Preserve Request Data** - Use move semantics
   ```cpp
   client->sendAsync(std::move(req), callback);  // Efficient
   ```

5. **Configure Once** - Set global configuration at startup
   ```cpp
   client->configure({{"timeout", 5000}});
   ```

6. **Use Method Chaining** - More fluent API
   ```cpp
   client->configure({...})
         .sendAsync(std::move(req), callback);
   ```

## Limitations and Notes

- **No Automatic Redirects** - 3xx status codes are returned to caller
- **No Connection Pooling** - Each client manages its own connections
- **No Custom Certificate Validation** - Uses platform defaults
- **Header Folding Supported** - HTTP header continuation lines are handled
- **JSON-First** - Other content types supported but secondary
- **No Exceptions for IO Errors** - Use `std::expected` for error handling

## See Also

- [Testing Documentation](TESTING.md) - Comprehensive testing guide
- [Best Practices](../best_practices.md) - Project best practices
- [API Examples](../API.md) - Additional API examples
