# Class `WinHttpRESTClient`

Header: `<siddiqsoft/restcl.hpp>` or `<siddiqsoft/private/restcl_win.hpp>`  
Namespace: `siddiqsoft`  
Inherits: [`basic_restclient<char>`](basic_restclient.md)  
Platform: **Windows** (uses WinHTTP API)

---

## Class Signature

```cpp
class WinHttpRESTClient : public basic_restclient<char>
{
public:
    WinHttpRESTClient() = default;
    WinHttpRESTClient(const nlohmann::json& cfg, basic_callbacktype&& cb = {});

    static std::shared_ptr<WinHttpRESTClient> CreateInstance(
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

## Member Functions

### `CreateInstance`

```cpp
static std::shared_ptr<WinHttpRESTClient> CreateInstance(
    const nlohmann::json& cfg = {},
    basic_callbacktype&& cb = {}
);
```
Creates a shared pointer to a new `WinHttpRESTClient` instance configured with `cfg`.

---

### `configure`

```cpp
basic_restclient& configure(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {});
```
Updates client configuration settings and optional default callback.

---

### `send` (Synchronous)

```cpp
[[nodiscard]] std::expected<rest_response<char>, int> send(rest_request<char>& req);
```
Performs a synchronous HTTP request and returns an `std::expected` containing the `rest_response` or an error code integer.

---

### `sendAsync` (Asynchronous)

```cpp
basic_restclient& sendAsync(rest_request<char>&& req, basic_callbacktype&& cb = {});
```
Dispatches the request asynchronously using background thread pools. Executes `cb` (or the default callback) upon completion.
