# Cosmos Probes Example

The **Cosmos Probes** example (`examples/cosmosprobes`) demonstrates how to build a lightweight health-check / readiness probe executable in C++23 using `restcl`.

---

## Key Features Demonstrated

* **Platform-Independent Client Factory**: Uses `siddiqsoft::GetRESTClient()` to acquire the native HTTP engine (`HttpRESTClient` on Linux/macOS or `WinHttpRESTClient` on Windows).
* **Curl Singleton Lifecycle**: Calls `LibCurlSingleton::GetInstance()` to safely initialize global cURL resources.
* **Client Configuration**: Configures connection timeout (`connectTimeout`), request timeout (`timeout`), and tracing (`trace`) dynamically using a JSON object.
* **User-Defined Literals**: Constructs GET requests cleanly using `"http://localhost:8080/ready"_GET`.
* **Safe Error Handling**: Evaluates `std::expected` response objects and extracts status codes, response content, or error messages cleanly.

---

## Source Code Walkthrough

Below is the complete implementation from `examples/cosmosprobes/src/probes.cpp`:

```cpp
#include <print>
#include <format>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "nlohmann/json.hpp"
#include "siddiqsoft/restcl.hpp"


int main(int argc, char** argv)
{
    using namespace siddiqsoft::restcl_literals;

    std::atomic_bool done = false;
    std::println(std::cerr, "{} - Init the CurlLib singleton.\n", __func__);
    auto myCurlInstance = siddiqsoft::LibCurlSingleton::GetInstance();
    if (myCurlInstance) {
        auto wrc = siddiqsoft::GetRESTClient();

        wrc->configure({{"connectTimeout", 3000}, // timeout for the connect phase
                        {"timeout", 5000},        // timeout for the overall IO phase
                        {"trace", false}});

        // The port 8080 is for checking the health of the service.
        auto req  = siddiqsoft::rest_request("http://localhost:8080/ready"_GET);
        auto resp = wrc->send(req);
        if (resp && resp->success()) {
            std::println(std::cerr, "  - Got Valid Response ------ \n{}", *resp);
        }
        else if (resp) {
            auto [ec, emsg] = resp->status();
            std::println(std::cerr, "  - Got response error: {} - {}", ec, emsg);
        }
        else {
            std::println(std::cerr, "  - Got error: `{}` -- `{}`", resp.error(), curl_easy_strerror(static_cast<CURLcode>(resp.error())));
        }

        return 0;
    }
    else {
        std::println(std::cerr, "{} - Failed to get CurlLib singleton instance!", __func__);
        return 1;
    }
}
```

---

## Detailed Code Breakdown

### 1. Singleton Initialization
```cpp
auto myCurlInstance = siddiqsoft::LibCurlSingleton::GetInstance();
```
Ensures underlying `libcurl` system resources are initialized once before any network calls take place.

### 2. Client Creation & Configuration
```cpp
auto wrc = siddiqsoft::GetRESTClient();

wrc->configure({{"connectTimeout", 3000},
                {"timeout", 5000},
                {"trace", false}});
```
`GetRESTClient()` creates a shared client pointer tailored to the platform. Configuration settings are passed via `nlohmann::json`.

### 3. Request Creation with UDL
```cpp
auto req = siddiqsoft::rest_request("http://localhost:8080/ready"_GET);
```
The `_GET` literal parses the URI string and returns a pre-populated `rest_request` object targeting the specified endpoint.

### 4. Sending & Response Parsing
```cpp
auto resp = wrc->send(req);
if (resp && resp->success()) {
    std::println(std::cerr, "  - Got Valid Response ------ \n{}", *resp);
}
```
`wrc->send()` executes the request synchronously and returns a `std::expected<rest_response<>, int>`.

---

## Building and Running

### Prerequisites

* C++23 compliant compiler (MSVC 2022+, GCC 11+, Clang 13+)
* CMake 3.31 or newer

### Build Instructions

```bash
# Navigate to example directory
cd examples/cosmosprobes

# Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build executable
cmake --build build
```

### Running the Example

Start a local HTTP server on port 8080 with a `/ready` endpoint, then execute:

```bash
./build/examples_cosmosprobes
```

#### Example Output (Success)

```text
main - Init the CurlLib singleton.

  - Got Valid Response ------ 
{"statusCode":200,"headers":{"content-type":"application/json"},"body":"{\"status\":\"ready\"}"}
```
