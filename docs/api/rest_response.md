# Class Template `rest_response<CharT>`

Header: `<siddiqsoft/private/rest_response.hpp>`  
Namespace: `siddiqsoft`  
Inherits: `http_frame<CharT>`

---

## Overview

`rest_response` represents an HTTP response received from a server.

---

## Key Methods

### `success`

```cpp
bool success() const;
```
Returns `true` if the HTTP status code is between 100 and 399 inclusive.

---

### `statusCode`

```cpp
unsigned statusCode() const;
```
Returns the HTTP numerical status code (e.g. `200`, `201`, `404`, `500`).

---

### `reasonCode`

```cpp
std::string reasonCode() const;
```
Returns the HTTP status reason phrase (e.g. `"OK"`, `"Not Found"`, `"Internal Server Error"`).

---

### `status`

```cpp
std::pair<unsigned, std::string> status() const;
```
Returns a pair containing `(statusCode, reasonCode)`.

---

## Code Example

```cpp
#include "siddiqsoft/restcl.hpp"

using namespace siddiqsoft::restcl_literals;

void handleResponse(const siddiqsoft::rest_response<>& resp)
{
    std::cout << "Status: " << resp.statusCode() << " " << resp.reasonCode() << std::endl;

    if (resp.success()) {
        std::cout << "Response Body: " << resp.content->body << std::endl;

        // Parse JSON content if available
        if (resp.headers.contains("content-type") &&
            resp.headers["content-type"].get<std::string>().find("application/json") != std::string::npos) {
            nlohmann::json j = nlohmann::json::parse(resp.content->body);
            std::cout << "Parsed JSON: " << j.dump(2) << std::endl;
        }
    }
}
```
