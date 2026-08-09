# Class Template `rest_request<CharT>`

Header: `<siddiqsoft/private/rest_request.hpp>`  
Namespace: `siddiqsoft`  
Inherits: `http_frame<CharT>`

---

## Overview

`rest_request` represents an HTTP request message containing:
- **HTTP Method**: GET, POST, PUT, DELETE, etc.
- **Target URI**: Parsed via `siddiqsoft::SplitUri`.
- **Headers**: Key-value pairs stored in JSON structure.
- **Content Body**: Payload data with automatically managed `Content-Type` and `Content-Length`.

---

## Constructors

```cpp
rest_request() = default;

rest_request(const HttpMethodType& method, const Uri<CharT, AuthorityHttp<CharT>> uri);

rest_request(const HttpMethodType& method, const Uri<CharT, AuthorityHttp<CharT>> uri, const nlohmann::json& headers);

rest_request(const HttpMethodType& method, const Uri<CharT, AuthorityHttp<CharT>> uri, const nlohmann::json& headers, const nlohmann::json& content);
```

---

## Key Methods

### `setContent`

```cpp
void setContent(const nlohmann::json& jsonBody);
```
Serializes the given JSON object into the request body, sets `Content-Type: application/json`, and calculates `Content-Length`.

### `encode`

```cpp
std::string encode() const override;
```
Encodes the entire request (Request Line, Headers, Body) into a raw HTTP string stream ready for wire transmission.

---

## Code Example

```cpp
#include "siddiqsoft/restcl.hpp"

using namespace siddiqsoft::restcl_literals;

void createCustomRequest()
{
    auto req = "https://api.example.com/v1/users"_POST;

    // Headers
    req.headers["Authorization"] = "Bearer token123";
    req.headers["Accept"] = "application/json";

    // Body
    req.setContent({
        {"name", "Jane Doe"},
        {"email", "jane@example.com"}
    });

    std::cout << "Encoded HTTP Request:\n" << req.encode() << std::endl;
}
```
