# User-Defined Literals

`restcl` introduces C++20 user-defined literal operators in the `siddiqsoft::restcl_literals` (or `siddiqsoft::literals`) namespace. These literals allow you to construct fully parsed `rest_request` objects directly from string literals.

---

## Supported Operators

The following string literal suffixes are available:

| Suffix | HTTP Method | Example |
| :--- | :--- | :--- |
| `_GET` | `GET` | `"https://httpbin.org/get"_GET` |
| `_POST` | `POST` | `"https://httpbin.org/post"_POST` |
| `_PUT` | `PUT` | `"https://httpbin.org/put"_PUT` |
| `_DELETE` | `DELETE` | `"https://httpbin.org/delete"_DELETE` |
| `_HEAD` | `HEAD` | `"https://httpbin.org/get"_HEAD` |
| `_OPTIONS` | `OPTIONS` | `"https://httpbin.org/get"_OPTIONS` |
| `_PATCH` | `PATCH` | `"https://httpbin.org/patch"_PATCH` |

---

## How It Works

Under the hood, the literal operator parses the provided URL string using `siddiqsoft::SplitUri` and constructs a `rest_request` object pre-configured with the corresponding HTTP method and target URI.

```cpp
#include "siddiqsoft/restcl.hpp"

using namespace siddiqsoft::restcl_literals;

void example()
{
    // Create a GET request
    auto getReq = "https://api.github.com/orgs/SiddiqSoft/repos"_GET;
    getReq.headers["User-Agent"] = "restcl-docs-example";

    // Create a POST request with query parameters in the URL
    auto postReq = "https://httpbin.org/post?source=cpp20"_POST;
    postReq.setContent({{"message", "Hello from restcl!"}});
}
```

!!! note "Namespace Usage"
    You can bring literal operators into scope with:
    ```cpp
    using namespace siddiqsoft::restcl_literals;
    ```
    or the alias namespace:
    ```cpp
    using namespace siddiqsoft::literals;
    ```
