# restcl API Reference

Complete documentation of all public API calls in the restcl library.

## Table of Contents

- [Factory Function](#factory-function)
- [REST Client Interface](#rest-client-interface)
- [REST Request API](#rest-request-api)
- [REST Response API](#rest-response-api)
- [HTTP Frame Base Class](#http-frame-base-class)
- [Platform-Specific Implementation](#platform-specific-implementation)
- [Error Handling](#error-handling)

## Factory Function

### `GetRESTClient()`

**Source:** [`include/siddiqsoft/restcl.hpp`](include/siddiqsoft/restcl.hpp)

**Signature:**
```cpp
[[nodiscard]] static auto GetRESTClient(
    const nlohmann::json& cfg = {}, 
    basic_callbacktype&& cb = {}
);
```

**Description:**
Factory function that creates and returns a platform-specific REST client instance. On Unix/Linux/macOS, returns `HttpRESTClient` (libcurl-based). On Windows, returns `WinHttpRESTClient` (WinHTTP-based).

**Parameters:**
- `cfg` (optional): JSON configuration object with the following keys:
  - `"userAgent"` (string): User-Agent header (default: "siddiqsoft.restcl/2")
  - `"trace"` (boolean): Enable verbose logging (default: false)
  - `"connectTimeout"` (integer): Connection timeout in milliseconds (default: 0 = no timeout)
  - `"timeout"` (integer): Overall request timeout in milliseconds (default: 0 = no timeout)
  - `"verifyPeer"` (integer): SSL peer verification (1 = enabled, 0 = disabled, default: 1)
  - `"freshConnect"` (boolean): Force new connections instead of reusing (default: false)

- `cb` (optional): Global callback function for async operations. Can be overridden per-request.
  - Signature: `void(rest_request<>&, std::expected<rest_response<>, int>)`

**Returns:**
- `std::shared_ptr<HttpRESTClient>` on Unix/Linux/macOS
- `std::shared_ptr<WinHttpRESTClient>` on Windows

**Throws:**
- `std::runtime_error` if platform initialization fails

**Example:**
```cpp
auto client = siddiqsoft::GetRESTClient({
    {"userAgent", "MyApp/1.0"},
    {"connectTimeout", 3000},
    {"timeout", 5000}
});
```

---

## REST Client Interface

### `configure()`

**Source:** [`include/siddiqsoft/private/basic_restclient.hpp`](include/siddiqsoft/private/basic_restclient.hpp)

**Signature:**
```cpp
virtual basic_restclient& configure(
    const nlohmann::json& cfg = {}, 
    basic_callbacktype&& func = {}
);
```

**Description:**
Configures the REST client with optional settings and a global callback handler. This is typically called once after client creation.

**Parameters:**
- `cfg` (optional): JSON configuration object (same keys as `GetRESTClient()`)
- `func` (optional): Global callback function for async operations

**Returns:**
- Reference to self for method chaining

**Example:**
```cpp
client->configure({
    {"userAgent", "MyApp/2.0"},
    {"timeout", 10000}
}, [](const auto& req, auto resp) {
    if (resp) {
        std::cout << "Response: " << resp->statusCode() << "\n";
    }
});
```

---

### `send()`

**Signature:**
```cpp
[[nodiscard]] virtual std::expected<rest_response<>, int> send(
    rest_request<>& req
);
```

**Description:**
Performs a synchronous HTTP request and returns the response or error code.

**Parameters:**
- `req`: Reference to the REST request object

**Returns:**
- `std::expected<rest_response<>, int>` containing either:
  - Success: `rest_response<>` with status code, headers, and body
  - Error: Error code (platform-specific)

**Error Codes:**
- Windows: WinHTTP error codes (12001, 12002, 12029, etc.)
- Unix/Linux: POSIX error codes (ECONNRESET, EBUSY, etc.)

**Example:**
```cpp
auto request = "https://api.example.com/users"_GET;
auto response = client->send(request);

if (response) {
    std::cout << "Status: " << response->statusCode() << "\n";
    std::cout << "Body: " << response->getContentBody() << "\n";
} else {
    std::cerr << "Error: " << response.error() << "\n";
}
```

---

### `sendAsync()`

**Signature:**
```cpp
virtual basic_restclient& sendAsync(
    rest_request<>&& req, 
    basic_callbacktype&& callback = {}
);
```

**Description:**
Performs an asynchronous HTTP request. The callback is invoked when the response is received or an error occurs.

**Parameters:**
- `req`: Rvalue reference to the REST request object (moved)
- `callback` (optional): Callback function for this specific request
  - If not provided, uses the global callback from `configure()`
  - Signature: `void(rest_request<>&, std::expected<rest_response<>, int>)`

**Returns:**
- Reference to self for method chaining

**Throws:**
- `std::runtime_error` if client is not initialized
- `std::invalid_argument` if no callback is provided (neither globally nor per-request)

**Example:**
```cpp
auto request = "https://api.example.com/data"_GET;

client->sendAsync(std::move(request), [](const auto& req, auto resp) {
    if (resp) {
        std::cout << "Async response: " << resp->statusCode() << "\n";
    } else {
        std::cerr << "Async error: " << resp.error() << "\n";
    }
});
```

---

## REST Request API

**Source:** [`include/siddiqsoft/private/rest_request.hpp`](include/siddiqsoft/private/rest_request.hpp)

### `rest_request<CharT>` Constructors

**Signature:**
```cpp
rest_request();  // Default constructor

rest_request(const HttpMethodType& method, const Uri<CharT, AuthorityHttp<CharT>>& uri);

rest_request(const HttpMethodType& method, const Uri<CharT, AuthorityHttp<CharT>>& uri, 
             const nlohmann::json& headers);

rest_request(const HttpMethodType& method, const Uri<CharT, AuthorityHttp<CharT>>& uri,
             const nlohmann::json& headers, const nlohmann::json& content);
```

**Description:**
Creates a REST request with optional method, URI, headers, and content.

**Example:**
```cpp
// Using default constructor
rest_request<> req;
req.setMethod(HttpMethodType::METHOD_GET);
req.setUri("https://api.example.com/users");

// Using parameterized constructor
rest_request<> req2(HttpMethodType::METHOD_POST, 
                    Uri<char, AuthorityHttp<char>>("https://api.example.com/users"),
                    nlohmann::json{{"Authorization", "Bearer token"}});
```

---

### User-Defined Literals

**Signature:**
```cpp
rest_request<char> operator""_GET(const char* url, size_t sz);
rest_request<char> operator""_POST(const char* url, size_t sz);
rest_request<char> operator""_PUT(const char* url, size_t sz);
rest_request<char> operator""_DELETE(const char* url, size_t sz);
rest_request<char> operator""_PATCH(const char* url, size_t sz);
rest_request<char> operator""_HEAD(const char* url, size_t sz);
rest_request<char> operator""_OPTIONS(const char* url, size_t sz);
rest_request<char> operator""_TRACE(const char* url, size_t sz);
rest_request<char> operator""_CONNECT(const char* url, size_t sz);
```

**Description:**
Convenient user-defined literals for creating requests with specific HTTP methods.

**Example:**
```cpp
using namespace siddiqsoft::restcl_literals;

auto get_req = "https://api.example.com/users"_GET;
auto post_req = "https://api.example.com/users"_POST;
auto delete_req = "https://api.example.com/users/123"_DELETE;
```

---

### `setMethod()`

**Signature:**
```cpp
auto& setMethod(const HttpMethodType& method);
auto& setMethod(const std::string& method);
```

**Description:**
Sets the HTTP method for the request.

**Parameters:**
- `method`: HTTP method as enum or string ("GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS", "TRACE", "CONNECT")

**Returns:**
- Reference to self for method chaining

**Throws:**
- `std::invalid_argument` if method string is not recognized

**Example:**
```cpp
request.setMethod(HttpMethodType::METHOD_POST);
// or
request.setMethod("POST");
```

---

### `getMethod()`

**Signature:**
```cpp
[[nodiscard]] HttpMethodType getMethod() const;
```

**Description:**
Retrieves the HTTP method of the request.

**Returns:**
- `HttpMethodType` enum value

**Example:**
```cpp
auto method = request.getMethod();
if (method == HttpMethodType::METHOD_GET) {
    std::cout << "This is a GET request\n";
}
```

---

### `setUri()`

**Signature:**
```cpp
auto& setUri(const Uri<char, AuthorityHttp<char>>& uri);
```

**Description:**
Sets the URI for the request. Automatically updates the Host header.

**Parameters:**
- `uri`: Parsed URI object

**Returns:**
- Reference to self for method chaining

**Example:**
```cpp
request.setUri("https://api.example.com/users/123");
```

---

### `getUri()`

**Signature:**
```cpp
auto& getUri() const;
```

**Description:**
Retrieves the URI of the request.

**Returns:**
- Const reference to the URI object

**Example:**
```cpp
auto uri = request.getUri();
std::cout << "Host: " << uri.authority.host << "\n";
```

---

### `setHeader()`

**Signature:**
```cpp
auto& setHeader(const std::string& key, const std::string& value);
```

**Description:**
Sets a single HTTP header. Empty values remove the header.

**Parameters:**
- `key`: Header name
- `value`: Header value (empty string removes the header)

**Returns:**
- Reference to self for method chaining

**Example:**
```cpp
request.setHeader("Authorization", "Bearer token123");
request.setHeader("Content-Type", "application/json");
request.setHeader("X-Custom-Header", "");  // Removes the header
```

---

### `setHeaders()`

**Signature:**
```cpp
auto& setHeaders(const nlohmann::json& headers);
```

**Description:**
Sets multiple HTTP headers from a JSON object.

**Parameters:**
- `headers`: JSON object with header key-value pairs

**Returns:**
- Reference to self for method chaining

**Example:**
```cpp
request.setHeaders({
    {"Authorization", "Bearer token"},
    {"Accept", "application/json"},
    {"User-Agent", "MyApp/1.0"}
});
```

---

### `getHeaders()`

**Signature:**
```cpp
nlohmann::json& getHeaders();
```

**Description:**
Retrieves all HTTP headers as a JSON object.

**Returns:**
- Reference to JSON object containing all headers

**Example:**
```cpp
auto& headers = request.getHeaders();
std::cout << headers.dump(2) << "\n";
```

---

### `setContent()`

**Signature:**
```cpp
auto& setContent(const std::string& contentType, const std::string& body);
auto& setContent(const std::string& body);
auto& setContent(const nlohmann::json& json);
auto& setContent(std::shared_ptr<ContentType> content);
```

**Description:**
Sets the request body content with optional content type.

**Parameters:**
- Overload 1: `contentType` (required), `body` (required)
- Overload 2: `body` (auto-detects content type from headers)
- Overload 3: `json` (sets content type to "application/json")
- Overload 4: `content` (shared pointer to ContentType object)

**Returns:**
- Reference to self for method chaining

**Throws:**
- `std::invalid_argument` if content type is set but body is empty, or vice versa

**Example:**
```cpp
// With explicit content type
request.setContent("application/json", R"({"name":"John"})");

// With JSON object
nlohmann::json payload = {{"name", "John"}};
request.setContent(payload);

// With plain text
request.setContent("Hello, World!");
```

---

### `getContent()`

**Signature:**
```cpp
auto& getContent() const;
```

**Description:**
Retrieves the request content object.

**Returns:**
- Const reference to ContentType object

**Example:**
```cpp
auto& content = request.getContent();
std::cout << "Content-Type: " << content->type << "\n";
std::cout << "Body: " << content->body << "\n";
```

---

### `getContentBody()`

**Signature:**
```cpp
auto& getContentBody() const;
```

**Description:**
Retrieves the request body as a string.

**Returns:**
- Const reference to body string

**Example:**
```cpp
auto& body = request.getContentBody();
std::cout << "Body: " << body << "\n";
```

---

### `encode()`

**Signature:**
```cpp
[[nodiscard]] std::string encode() const override;
```

**Description:**
Encodes the request to an HTTP-formatted byte stream.

**Returns:**
- String containing the complete HTTP request (start line, headers, body)

**Example:**
```cpp
auto encoded = request.encode();
std::cout << encoded << "\n";
```

---

## REST Response API

**Source:** [`include/siddiqsoft/private/rest_response.hpp`](include/siddiqsoft/private/rest_response.hpp)

### `statusCode()`

**Signature:**
```cpp
auto statusCode() const;
```

**Description:**
Retrieves the HTTP status code from the response.

**Returns:**
- Unsigned integer (e.g., 200, 404, 500)

**Example:**
```cpp
auto status = response->statusCode();
if (status == 200) {
    std::cout << "Success!\n";
}
```

---

### `reasonCode()`

**Signature:**
```cpp
auto reasonCode() const;
```

**Description:**
Retrieves the HTTP reason phrase from the response.

**Returns:**
- String (e.g., "OK", "Not Found", "Internal Server Error")

**Example:**
```cpp
std::cout << "Status: " << response->statusCode() << " " 
          << response->reasonCode() << "\n";
```

---

### `status()`

**Signature:**
```cpp
auto status() const;
```

**Description:**
Retrieves both status code and reason phrase as a pair.

**Returns:**
- `std::pair<unsigned, std::string>` with status code and reason phrase

**Example:**
```cpp
auto [code, reason] = response->status();
std::cout << code << ": " << reason << "\n";
```

---

### `success()`

**Signature:**
```cpp
bool success() const;
```

**Description:**
Checks if the response indicates success (status code between 100 and 399).

**Returns:**
- `true` if status code is in range [100, 399], `false` otherwise

**Example:**
```cpp
if (response->success()) {
    std::cout << "Request succeeded!\n";
}
```

---

### `setStatus()`

**Signature:**
```cpp
rest_response<>& setStatus(const int code, const std::string& message);
```

**Description:**
Sets the HTTP status code and reason phrase.

**Parameters:**
- `code`: HTTP status code
- `message`: Reason phrase

**Returns:**
- Reference to self for method chaining

**Example:**
```cpp
response.setStatus(200, "OK");
```

---

### `getHeaders()`, `setHeader()`, `setHeaders()`

Same as `rest_request` - see [REST Request API](#rest-request-api) section above.

---

### `getContent()`, `setContent()`, `getContentBody()`

Same as `rest_request` - see [REST Request API](#rest-request-api) section above.

---

### `parse()`

**Signature:**
```cpp
[[nodiscard]] static auto parse(std::string& srcBuffer) -> rest_response<char>;
```

**Description:**
Parses an HTTP response from a raw byte string.

**Parameters:**
- `srcBuffer`: String containing the raw HTTP response (modified in-place)

**Returns:**
- Parsed `rest_response<char>` object

**Throws:**
- `std::invalid_argument` if the response format is invalid

**Example:**
```cpp
std::string raw_response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{...}";
auto response = rest_response<>::parse(raw_response);
```

---

### `encode()`

**Signature:**
```cpp
[[nodiscard]] std::string encode() const override;
```

**Description:**
Encodes the response to an HTTP-formatted byte stream.

**Returns:**
- String containing the complete HTTP response

**Example:**
```cpp
auto encoded = response->encode();
std::cout << encoded << "\n";
```

---

## HTTP Frame Base Class

**Source:** [`include/siddiqsoft/private/http_frame.hpp`](include/siddiqsoft/private/http_frame.hpp)

The `http_frame<CharT>` class provides common functionality for both requests and responses:

- `setProtocol()` / `getProtocol()`: HTTP protocol version (HTTP/1.0, HTTP/1.1, HTTP/2, HTTP/3)
- `setMethod()` / `getMethod()`: HTTP method
- `setUri()` / `getUri()`: Request URI
- `setHeader()` / `setHeaders()` / `getHeaders()`: Header management
- `setContent()` / `getContent()` / `getContentBody()`: Content management
- `encodeHeaders()`: Encode headers to HTTP format
- `getHost()`: Get the Host header value

---

## Platform-Specific Implementation

### Unix/Linux/macOS Implementation

**Source:** [`include/siddiqsoft/private/restcl_unix.hpp`](include/siddiqsoft/private/restcl_unix.hpp)

#### `HttpRESTClient` Class

The Unix/Linux/macOS implementation uses libcurl for HTTP operations.

**Public Methods:**

- `configure(const nlohmann::json& cfg = {}, basic_callbacktype&& func = {}) -> basic_restclient&`
  - Configures the client with timeout, SSL verification, and other options
  - Registers global callback for async operations

- `send(rest_request<>& req) -> std::expected<rest_response<>, int>`
  - Performs synchronous HTTP request using libcurl
  - Returns response or POSIX error code

- `sendAsync(rest_request<>&& req, basic_callbacktype&& callback = {}) -> basic_restclient&`
  - Performs asynchronous HTTP request
  - Invokes callback when response is received

- `static CreateInstance(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {}) -> std::shared_ptr<HttpRESTClient>`
  - Factory method to create new client instance

**Configuration Options:**

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `userAgent` | string | "siddiqsoft.restcl/2" | User-Agent header |
| `trace` | boolean | false | Enable verbose libcurl output |
| `connectTimeout` | integer | 0 | Connection timeout (ms) |
| `timeout` | integer | 0 | Overall timeout (ms) |
| `verifyPeer` | integer | 1 | SSL peer verification (1=on, 0=off) |
| `freshConnect` | boolean | false | Force new connections |

**Example:**

```cpp
auto client = siddiqsoft::HttpRESTClient::CreateInstance({
    {"userAgent", "MyApp/1.0"},
    {"connectTimeout", 3000},
    {"timeout", 5000},
    {"verifyPeer", 1}
});

auto request = "https://api.example.com/users"_GET;
auto response = client->send(request);
```

#### `LibCurlSingleton` Class

Manages global libcurl initialization and connection pooling.

**Public Methods:**

- `static GetInstance() -> std::shared_ptr<LibCurlSingleton>`
  - Returns singleton instance
  - Initializes libcurl on first call (thread-safe)
  - Throws `std::runtime_error` if initialization fails

- `getEasyHandle() -> CurlContextBundlePtr`
  - Returns a pooled or new libcurl handle
  - Automatically configures debug callbacks
  - Handle is returned to pool on destruction

#### `CurlContextBundle` Class

Manages a libcurl handle with automatic resource cleanup.

**Public Methods:**

- `curlHandle() -> CURL*`
  - Returns the underlying libcurl handle

- `contents() -> std::shared_ptr<ContentType>`
  - Returns the content object for request/response data

- `abandon()`
  - Releases the handle without returning to pool
  - Used for failed operations to prevent reuse

#### `rest_result_error` Struct

Encapsulates libcurl error codes from various APIs.

**Supported Error Types:**
- `CURLcode`: Easy interface errors
- `CURLMcode`: Multi interface errors
- `CURLHcode`: Header API errors
- `CURLSHcode`: Share interface errors
- `CURLUcode`: URL API errors
- `uint32_t`: POSIX error codes

**Public Methods:**

- `to_string() -> std::string`
  - Returns human-readable error message

- `operator std::string()`
  - Implicit conversion to string

**Example:**

```cpp
rest_result_error err(CURLE_COULDNT_RESOLVE_HOST);
std::string msg = err.to_string();  // "Couldn't resolve host name"
```

#### Atomic Counters

The client tracks various operations for monitoring:

- `ioAttempt`: Total I/O attempts
- `ioAttemptFailed`: Failed I/O attempts
- `ioConnect`: Successful connections
- `ioConnectFailed`: Failed connections
- `ioSend`: Successful sends
- `ioSendFailed`: Failed sends
- `ioReadAttempt`: Read attempts
- `ioRead`: Successful reads
- `ioReadFailed`: Failed reads
- `callbackAttempt`: Callback invocations
- `callbackFailed`: Failed callbacks
- `callbackCompleted`: Completed callbacks

---

## Error Handling

All IO operations return `std::expected<T, E>` for error handling:

```cpp
auto response = client->send(request);

if (response) {
    // Success - response contains the result
    std::cout << "Status: " << response->statusCode() << "\n";
} else {
    // Error - response.error() contains the error code
    int error_code = response.error();
    std::cerr << "Error: " << error_code << "\n";
}
```

**Error Codes:**
- **Windows**: WinHTTP error codes (12001-12175)
- **Unix/Linux**: POSIX error codes (ECONNRESET, EBUSY, ENETUNREACH, etc.)
