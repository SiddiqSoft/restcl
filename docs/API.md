# restcl API Reference

This document describes the current public API surface in the codebase.

## Table of Contents

- [Factory Function](#factory-function)
- [Core Client Interface](#core-client-interface)
- [Request API](#request-api)
- [Response API](#response-api)
- [Shared HTTP Frame API](#shared-http-frame-api)
- [Platform Implementations](#platform-implementations)
- [Error Handling](#error-handling)

## Factory Function

### GetRESTClient

**Source:** `include/siddiqsoft/restcl.hpp`

**Signature:**

```cpp
[[nodiscard]] static auto GetRESTClient(
    const nlohmann::json& cfg = {},
    basic_callbacktype&& cb = {}
);
```

Returns a platform-specific client instance:

- Unix and macOS: `std::shared_ptr<HttpRESTClient>`
- Windows: `std::shared_ptr<WinHttpRESTClient>`

The callback type is:

```cpp
using basic_callbacktype = std::function<
    void(rest_request<>&, std::expected<rest_response<>, int>)
>;
```

### Configuration keys

Common keys accepted by platform clients:

- `userAgent` (string, default `"siddiqsoft.restcl/2"`)
- `trace` (bool, default `false`)
- `connectTimeout` (integer milliseconds, default `0`)
- `timeout` (integer milliseconds, default `0`)
- `headers` (json object, optional)
- `downloadDirectory` (implementation-reserved)

Unix and macOS only:

- `verifyPeer` (integer, default `1`; set to `0` to disable SSL peer verification)
- `freshConnect` (bool, default `false`)

## Core Client Interface

**Source:** `include/siddiqsoft/private/basic_restclient.hpp`

Template:

```cpp
template <typename CharT = char>
class basic_restclient
```

### configure

```cpp
virtual basic_restclient& configure(
    const nlohmann::json& = {},
    basic_callbacktype&& = {}
) = 0;
```

Configures provider settings and optionally registers a default async callback.

### send

```cpp
[[nodiscard]] virtual std::expected<rest_response<CharT>, int>
send(rest_request<>&) = 0;
```

Synchronous request execution.

### sendAsync

```cpp
virtual basic_restclient& sendAsync(
    rest_request<>&&,
    basic_callbacktype&& = {}
) = 0;
```

Asynchronous request execution. A callback must be provided either here or through `configure`.

Behavior notes:

- Unix and macOS may throw `std::runtime_error` if client initialization is incomplete.
- All platforms throw `std::invalid_argument` if no callback is available.

## Request API

**Source:** `include/siddiqsoft/private/rest_request.hpp`

Class:

```cpp
template <typename CharT = char>
class rest_request : public http_frame<CharT>
```

### Constructors

```cpp
rest_request() = default;

rest_request(const HttpMethodType&,
             const Uri<CharT, AuthorityHttp<CharT>>);

rest_request(const HttpMethodType&,
             const Uri<CharT, AuthorityHttp<CharT>>,
             const nlohmann::json& headers);

rest_request(const HttpMethodType&,
             const Uri<CharT, AuthorityHttp<CharT>>,
             const nlohmann::json& headers,
             const nlohmann::json& content);
```

### User-defined literals

Namespace: `siddiqsoft::restcl_literals`

```cpp
operator""_GET(const char*, size_t)
operator""_HEAD(const char*, size_t)
operator""_POST(const char*, size_t)
operator""_PUT(const char*, size_t)
operator""_DELETE(const char*, size_t)
operator""_CONNECT(const char*, size_t)
operator""_OPTIONS(const char*, size_t)
operator""_TRACE(const char*, size_t)
operator""_PATCH(const char*, size_t)
```

All return `rest_request<char>`.

### encode

```cpp
std::string encode() const override;
```

Builds the HTTP wire representation (start line, headers, optional body).

Throws `std::invalid_argument` if content type exists but body is empty.

## Response API

**Source:** `include/siddiqsoft/private/rest_response.hpp`

Class:

```cpp
template <typename CharT = char>
class rest_response : public http_frame<CharT>
```

### Status helpers

```cpp
bool success() const;
auto statusCode() const;
auto reasonCode() const;
auto status() const; // std::pair<unsigned, std::string>
rest_response<>& setStatus(const int code, const std::string& message);
```

`success()` is true for status codes in the range `100..399`.

### parse

```cpp
[[nodiscard]] static auto parse(std::string& srcBuffer)
    -> siddiqsoft::rest_response<char>;
```

Parses start line, headers, and body from a raw HTTP response buffer.

### encode

```cpp
std::string encode() const override;
```

Encodes the response as HTTP wire format.

## Shared HTTP Frame API

**Source:** `include/siddiqsoft/private/http_frame.hpp`

Base class:

```cpp
template <typename CharT = char>
class http_frame
```

`rest_request` and `rest_response` inherit these APIs.

### Protocol

```cpp
auto& setProtocol(const HttpProtocolVersionType&);
auto& setProtocol(const std::string&);
auto getProtocol();
```

### Method

```cpp
auto& setMethod(const HttpMethodType&);
auto& setMethod(const std::string&);
[[nodiscard]] HttpMethodType getMethod() const;
```

### URI

```cpp
auto& setUri(const Uri<char, AuthorityHttp<char>>&);
auto& getUri() const;
auto getHost() const;
```

Note: setting URI also updates the `Host` header as `host:port`.

### Headers

```cpp
auto& setHeaders(const nlohmann::json&);
auto& setHeader(const std::string& key, const std::string& value);
auto& getHeader(const std::string& key) const;
nlohmann::json& getHeaders();
[[nodiscard]] std::string encodeHeaders() const;
```

Header removal behavior: passing an empty value to `setHeader` erases that header.

### Content

```cpp
auto& setContent(const std::string& ctype, const std::string& c);
auto& setContent(const std::string& src);
auto& setContent(std::shared_ptr<ContentType> src);
auto& setContent(const nlohmann::json& c);

auto& getContent() const;
auto& getContentBody() const;
[[nodiscard]] auto getContentBodyJSON() const -> nlohmann::json;
[[nodiscard]] auto encodeContent() const;
```

Behavior details:

- `setContent(const nlohmann::json&)` applies only for non-empty JSON objects.
- `setContent(const std::string&)` uses current `Content-Type` header if present, otherwise defaults content type metadata to `application/text`.
- `getContentBodyJSON()` returns parsed JSON when content type contains `json`; otherwise returns `nullptr` JSON.

## Platform Implementations

### Unix and macOS

**Sources:**

- `include/siddiqsoft/private/restcl_unix.hpp`
- `include/siddiqsoft/private/libcurl_singleton.hpp`

Client class:

```cpp
class HttpRESTClient : public basic_restclient<char>
```

Factory:

```cpp
[[nodiscard]] static auto CreateInstance(
    const nlohmann::json& cfg = {},
    basic_callbacktype&& cb = {}
);
```

Implementation notes:

- Uses `LibCurlSingleton` for libcurl lifecycle and handle pooling.
- Supports sync and async operations.
- Async callback dispatch is guarded by a mutex to safely access the configured callback.

Additional Unix type:

```cpp
struct rest_result_error
```

Wraps several libcurl and POSIX error families and provides `to_string()`.

### Windows

**Source:** `include/siddiqsoft/private/restcl_win.hpp`

Client class:

```cpp
class WinHttpRESTClient : public basic_restclient<char>
```

Factory:

```cpp
[[nodiscard]] static auto CreateInstance(
    const nlohmann::json& cfg = {},
    basic_callbacktype&& cb = {}
) -> std::shared_ptr<WinHttpRESTClient>;
```

Implementation notes:

- Uses WinHTTP session APIs for request execution.
- Enables HTTP/2 and decompression during configuration when possible.
- Supports sync and async operations with callback dispatch through a work pool.

Additional Windows type:

```cpp
struct rest_result_error
```

Wraps WinInet/WinHTTP-style error codes and exposes `to_string()`.

## Error Handling

Synchronous send returns:

```cpp
std::expected<rest_response<>, int>
```

Usage pattern:

```cpp
auto req  = "https://api.example.com/users"_GET;
auto resp = client->send(req);

if (resp) {
    std::cout << resp->statusCode() << "\n";
} else {
    std::cerr << "error=" << resp.error() << "\n";
}
```

Async send does not return a response directly. Handle both success and failure in the callback.
