# restcl: A Focused REST Client for Modern C++

<div class="badge-container">
  <a href="https://github.com/SiddiqSoft/restcl/actions/workflows/codeql-analysis.yml"><img src="https://github.com/SiddiqSoft/restcl/actions/workflows/codeql-analysis.yml/badge.svg" alt="CodeQL"></a>
  <a href="https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=13&branchName=main"><img src="https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.restcl?branchName=main" alt="Build Status"></a>
  <a href="https://www.nuget.org/packages/SiddiqSoft.restcl"><img src="https://img.shields.io/nuget/v/SiddiqSoft.restcl" alt="NuGet Version"></a>
  <a href="https://github.com/SiddiqSoft/restcl/tags"><img src="https://img.shields.io/github/v/tag/SiddiqSoft/restcl" alt="GitHub Tag"></a>
</div>

**`restcl`** is a header-only Modern C++20 REST client library designed with `nlohmann::json` as a first-class API metaphor for interacting with RESTful servers.

---

## Design Objectives

* **JSON as First-Class Metaphor**: Standard JSON objects represent requests, headers, and payloads for an intuitive, JavaScript-like API.
* **Modern C++20**: Requires C++20 with support for concepts, user-defined literals, and `std::format`.
* **Cross-Platform & Native IO**:
    * **Windows**: Uses native `WinHTTP` library (`WinHttpRESTClient`).
    * **Linux / macOS**: Uses `libcurl` (`HttpRESTClient`).
* **Factory Function**: Convenient `siddiqsoft::GetRESTClient()` creates the appropriate client instance for your host platform.
* **User-Defined Literals**: Expressive HTTP request creation with `"https://api.example.com/endpoint"_GET`.

---

## Quick Example

=== "Cross-Platform Factory"

    ```cpp
    #include "siddiqsoft/restcl.hpp"

    using namespace siddiqsoft;
    using namespace siddiqsoft::restcl_literals;

    int main()
    {
        // Automatically instantiates WinHttpRESTClient on Windows
        // or HttpRESTClient on Linux/macOS
        auto client = GetRESTClient({
            {"userAgent", "my-app/1.0"},
            {"timeout", 5000}
        });

        // 1. Simple GET request using literal operator
        auto response = client->send("https://httpbin.org/get"_GET);
        if (response && response->success()) {
            std::cout << "Response body: " << response->content->body << std::endl;
        }

        // 2. POST request with custom header and JSON body
        auto req = "https://httpbin.org/post"_POST;
        req.headers["X-Custom-Header"] = "my-header-value";
        req.setContent({ {"name", "Modern C++"}, {"version", 20} });

        // Synchronous send
        auto postResponse = client->send(req);
        if (postResponse && postResponse->success()) {
            std::cout << "Status: " << postResponse->statusCode() << std::endl;
        }

        return 0;
    }
    ```

=== "Windows (WinHTTP)"

    ```cpp
    #include "siddiqsoft/restcl.hpp"

    using namespace siddiqsoft;
    using namespace siddiqsoft::restcl_literals;

    int main()
    {
        WinHttpRESTClient client("my-user-agent-string");

        // Send GET request asynchronously with callback
        client.sendAsync("https://httpbin.org/get"_GET, [](auto& req, auto resp) {
            if (resp && resp->success()) {
                std::cout << "GET succeeded with status " << resp->statusCode() << std::endl;
            }
        });

        return 0;
    }
    ```

=== "Linux / macOS (libcurl)"

    ```cpp
    #include "siddiqsoft/restcl.hpp"

    using namespace siddiqsoft;
    using namespace siddiqsoft::restcl_literals;

    int main()
    {
        HttpRESTClient client({{"userAgent", "LinuxClient/1.0"}});

        auto req = "https://httpbin.org/put"_PUT;
        req.setContent({{"status", "active"}});

        auto resp = client.send(req);
        if (resp && resp->success()) {
            std::cout << "PUT Response: " << resp->content->body << std::endl;
        }

        return 0;
    }
    ```

---

## Requirements

| Requirement | Details |
| :--- | :--- |
| **Language Standard** | C++20 or higher (`/std:c++latest` on MSVC, `-std=c++20` on Clang/GCC) |
| **Dependencies** | [`nlohmann/json`](https://github.com/nlohmann/json), [`SplitUri`](https://github.com/SiddiqSoft/SplitUri), [`azure-cpp-utils`](https://github.com/SiddiqSoft/azure-cpp-utils) |
| **Platform Support** | Windows (MSVC 2019+), Linux (GCC 11+, Clang 13+), macOS (Apple Clang 13+) |

---

## Navigation

- [**Features**](features/index.md): Discover user-defined literals, async callbacks, and the JSON API metaphor.
- [**Integration**](integration/index.md): Guides for CMake, git submodules, and NuGet package integration.
- [**API Reference**](api/index.md): Detailed API documentation for `GetRESTClient`, request/response models, and client interfaces.
