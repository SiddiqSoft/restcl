# First-Class JSON API Metaphor

In `restcl`, `nlohmann::json` is the primary metadata and payload interface. Instead of complex C-style structures, headers, configuration options, and payloads are represented and manipulated using standard JSON objects.

---

## Configuration via JSON

Client settings are initialized using JSON objects:

```cpp
nlohmann::json config = {
    {"userAgent", "my-service/2.0"},
    {"connectTimeout", 3000},
    {"timeout", 10000},
    {"verifyPeer", 1},
    {"trace", false}
};

auto client = siddiqsoft::GetRESTClient(config);
```

---

## Headers & Content Payload

`rest_request` inherits from `http_frame`, which maintains headers and body content as JSON-compatible entities.

### Setting Headers

```cpp
auto req = "https://httpbin.org/post"_POST;

// Direct JSON key assignment for HTTP headers
req.headers["Authorization"] = "Bearer secret-token";
req.headers["X-Request-ID"] = "req-123456";
```

### Setting JSON Content Body

`setContent()` serializes any `nlohmann::json` object into the request body and automatically sets the `Content-Type: application/json` header and `Content-Length`.

```cpp
nlohmann::json payload = {
    {"username", "alice"},
    {"roles", {"admin", "developer"}},
    {"metadata", {
        {"loginCount", 42},
        {"enabled", true}
    }}
};

req.setContent(payload);
```

---

## Response Parsing

Received responses automatically parse JSON content bodies when the `Content-Type` header is `application/json`:

```cpp
auto resp = client->send(req);
if (resp && resp->success()) {
    // Access response content
    std::string rawBody = resp->content->body;
    
    // Parse response body to nlohmann::json
    nlohmann::json jsonResp = resp->content->toJson();
    std::cout << "User ID: " << jsonResp["id"] << std::endl;
}
```

!!! success "Simplicity First"
    By using `nlohmann::json`, request construction feels like JavaScript / TypeScript `fetch()` while retaining full C++23 type safety and performance.
