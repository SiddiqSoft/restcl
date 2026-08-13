# restcl: A Focused REST Client for Modern C++

<!-- badges -->
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.restcl?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=13&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.restcl)
![](https://img.shields.io/github/v/tag/SiddiqSoft/restcl)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/13)
<!-- end badges -->

**`restcl`** is a header-only Modern C++23 REST client library designed with `nlohmann::json` as a first-class API metaphor for interacting with RESTful servers.

* **Cross-Platform Native IO**: Uses native `WinHTTP` on Windows (`WinHttpRESTClient`) and `libcurl` on Linux/macOS (`HttpRESTClient`).
* **Factory Function**: `siddiqsoft::GetRESTClient()` automatically returns the optimal client instance for the target platform.
* **Expressive Request Syntax**: Native user-defined literals like `"https://api.example.com/endpoint"_GET`.
* **JSON-First API**: Request headers, parameters, and bodies are represented as `nlohmann::json` objects.

---

## Documentation Site

Full guides, tutorials, API specifications, and interactive dependency graphs are hosted on our documentation site:

* 🚀 [**Features & Usage**](docs/features/index.md): User-defined literals, JSON API metaphor, async callbacks.
* 📦 [**Integration & CMake**](docs/integration/cmake.md): `add_subdirectory`, CPM, FetchContent, build options, testing.
* 📊 [**Dependency Graph**](docs/integration/dependencies.md): Automated visual dependency diagram and version matrix.
* 📖 [**API Reference**](docs/api/index.md): Specifications for `GetRESTClient`, `rest_request`, `rest_response`, and client classes.
* 💡 [**Examples**](docs/examples/index.md): Standalone applications including the [Cosmos Probes](docs/examples/cosmosprobes.md) service health probe.

---

## Quick Start

```cpp
#include "siddiqsoft/restcl.hpp"

using namespace siddiqsoft;
using namespace siddiqsoft::restcl_literals;

int main()
{
    // Instantiates WinHttpRESTClient on Windows or HttpRESTClient on Linux/macOS
    auto client = GetRESTClient({
        {"userAgent", "my-app/1.0"},
        {"timeout", 5000}
    });

    // 1. Simple GET request using literal operator
    auto response = client->send("https://httpbin.org/get"_GET);
    if (response && response->success()) {
        std::cout << "Response body: " << response->content->body << std::endl;
    }

    // 2. POST request with custom headers and JSON body
    auto req = "https://httpbin.org/post"_POST;
    req.headers["X-Custom-Header"] = "my-header-value";
    req.setContent({ {"name", "Modern C++"}, {"version", 23} });

    auto postResponse = client->send(req);
    if (postResponse && postResponse->success()) {
        std::cout << "Status: " << postResponse->statusCode() << std::endl;
    }

    return 0;
}
```

---

## Integration

### Using CPM / FetchContent

```cmake
CPMAddPackage("gh:SiddiqSoft/restcl#2.3.12")
target_link_libraries(${PROJECT_NAME} INTERFACE siddiqsoft::restcl)
```

### Git Submodule

```bash
git submodule add https://github.com/SiddiqSoft/restcl.git vendor/restcl
```

```cmake
add_subdirectory(vendor/restcl)
target_link_libraries(your_target PRIVATE siddiqsoft::restcl)
```

For full setup guides and NuGet usage, view the [Integration Guide](docs/integration/index.md).

---

## Requirements & Building

| Requirement | Details |
| :--- | :--- |
| **Language Standard** | C++23 (`/std:c++latest` on MSVC, `-std=c++23` on Clang/GCC) |
| **Platforms** | Windows (MSVC 2022+), Linux (GCC 11+, Clang 13+), macOS (Apple Clang 13+) |
| **Dependencies** | [`nlohmann/json`](https://github.com/nlohmann/json), [`SplitUri`](https://github.com/SiddiqSoft/SplitUri), [`AzureCppUtils`](https://github.com/SiddiqSoft/AzureCppUtils), [`ctre`](https://github.com/hanickadot/compile-time-regular-expressions) |

### Building with CMake Presets

```bash
# Configure with a preset matching your OS/compiler (e.g. Darwin, Linux-GCC, Windows-x64)
cmake --preset Darwin

# Build target
cmake --build --preset Darwin

# Run test suite
ctest --preset Darwin
```

---

## License

Licensed under the [MIT License](LICENSE).
