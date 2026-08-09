# Class `HttpRESTClient`

Header: `<siddiqsoft/restcl.hpp>` or `<siddiqsoft/private/restcl_unix.hpp>`  
Namespace: `siddiqsoft`  
Inherits: [`basic_restclient<char>`](basic_restclient.md)  
Platform: **Unix / Linux / macOS** (uses libcurl)

---

## Class Signature

```cpp
class HttpRESTClient : public basic_restclient<char>
{
public:
    HttpRESTClient() = default;
    HttpRESTClient(const nlohmann::json& cfg, basic_callbacktype&& cb = {});

    static std::shared_ptr<HttpRESTClient> CreateInstance(
        const nlohmann::json& cfg = {},
        basic_callbacktype&& cb = {}
    );

    basic_restclient& configure(
        const nlohmann::json& cfg = {},
        basic_callbacktype&& cb = {}
    ) override;

    [[nodiscard]] std::expected<rest_response<char>, int> send(
        rest_request<char>& req
    ) override;

    basic_restclient& sendAsync(
        rest_request<char>&& req,
        basic_callbacktype&& cb = {}
    ) override;
};
```

---

## Overview

`HttpRESTClient` is the Unix/Linux/macOS implementation of `basic_restclient`. It uses `libcurl` under the hood to perform non-blocking and synchronous multi-threaded HTTP operations with support for SSL/TLS, custom headers, timeouts, and JSON payloads.
