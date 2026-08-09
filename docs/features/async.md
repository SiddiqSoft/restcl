# Asynchronous Operations

`restcl` provides built-in non-blocking asynchronous execution via `sendAsync()`.

---

## Overview

When performing asynchronous HTTP operations, responses are processed via callback functions of type `basic_callbacktype`:

```cpp
using basic_callbacktype = std::function<void(
    rest_request<>& request,
    std::expected<rest_response<>, int> responseOrError
)>;
```

---

## Usage Examples

### Per-Request Callback

You can pass a lambda callback directly to `sendAsync()`:

```cpp
#include "siddiqsoft/restcl.hpp"

using namespace siddiqsoft;
using namespace siddiqsoft::restcl_literals;

void runAsyncRequest()
{
    auto client = GetRESTClient();

    auto req = "https://httpbin.org/get"_GET;

    client->sendAsync(std::move(req), [](auto& request, auto result) {
        if (result) {
            auto& resp = result.value();
            std::cout << "HTTP Status: " << resp.statusCode() << std::endl;
            std::cout << "Body: " << resp.content->body << std::endl;
        } else {
            std::cerr << "Error code: " << result.error() << std::endl;
        }
    });
}
```

### Global Default Callback

You can also specify a default callback when initializing client configuration via `configure()` or `GetRESTClient()`:

```cpp
auto client = GetRESTClient(
    { {"userAgent", "AsyncClient/1.0"} },
    [](auto& req, auto result) {
        if (result) {
            std::cout << "Default callback received status: " << result->statusCode() << std::endl;
        }
    }
);

// Uses global default callback:
client->sendAsync("https://httpbin.org/get"_GET);
```

---

## Error Handling

`sendAsync` uses `std::expected<rest_response<>, int>` to represent either a successfully received HTTP response or a transport/network error code.

!!! tip "Checking Results"
    Always check `if (result)` or `result.has_value()` before dereferencing the response object.
