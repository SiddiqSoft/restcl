# Factory Function `GetRESTClient`

Header: `<siddiqsoft/restcl.hpp>`  
Namespace: `siddiqsoft`

---

## Signature

```cpp
[[nodiscard]] inline auto GetRESTClient(
    const nlohmann::json& cfg = {},
    basic_callbacktype&& cb = {}
);
```

---

## Description

Creates and returns a shared pointer to a platform-specific REST client instance (`std::shared_ptr<WinHttpRESTClient>` on Windows, `std::shared_ptr<HttpRESTClient>` on Linux/macOS).

---

## Parameters

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `cfg` | `const nlohmann::json&` | `{}` | Optional configuration object (see supported options below). |
| `cb` | `basic_callbacktype&&` | `{}` | Optional global default callback for asynchronous operations. |

### Configuration Options (`cfg`)

| Key | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `"userAgent"` | `std::string` | `"siddiqsoft.restcl/2"` | User-Agent request header string. |
| `"trace"` | `bool` | `false` | Enable verbose trace debugging output. |
| `"connectTimeout"` | `int` | `0` | Connection timeout in milliseconds. |
| `"timeout"` | `int` | `0` | Total request timeout in milliseconds. |
| `"verifyPeer"` | `int` | `1` | SSL peer verification enabled (1) or disabled (0). |
| `"freshConnect"` | `bool` | `false` | Force fresh connection for each request. |
| `"useTLSv1_3"` | `bool` | `false` | Force TLS 1.3 protocol version (Unix/Linux/macOS). |
| `"useTLSv1_2"` | `bool` | `false` | Force TLS 1.2 protocol version (Unix/Linux/macOS). |
| `"useTLSv1_1"` | `bool` | `false` | Force TLS 1.1 protocol version (Unix/Linux/macOS). |

---

## Return Value

Returns a shared pointer to the concrete implementation:
- **Windows**: `std::shared_ptr<WinHttpRESTClient>`
- **Linux / macOS**: `std::shared_ptr<HttpRESTClient>`

---

## Code Example

```cpp
#include "siddiqsoft/restcl.hpp"

using namespace siddiqsoft;
using namespace siddiqsoft::restcl_literals;

int main()
{
    nlohmann::json config = {
        {"userAgent", "my-service/1.0"},
        {"timeout", 5000}
    };

    auto client = GetRESTClient(config);

    auto response = client->send("https://httpbin.org/get"_GET);
    if (response && response->success()) {
        std::cout << "Success: " << response->content->body << std::endl;
    }

    return 0;
}
```
