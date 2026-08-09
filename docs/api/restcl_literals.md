# Namespace `siddiqsoft::restcl_literals`

Header: `<siddiqsoft/restcl.hpp>`  
Namespace: `siddiqsoft::restcl_literals` (also aliased as `siddiqsoft::literals`)

---

## User-Defined Literals

```cpp
namespace siddiqsoft::restcl_literals {
    rest_request<char> operator""_GET(const char* str, std::size_t len);
    rest_request<char> operator""_POST(const char* str, std::size_t len);
    rest_request<char> operator""_PUT(const char* str, std::size_t len);
    rest_request<char> operator""_DELETE(const char* str, std::size_t len);
    rest_request<char> operator""_HEAD(const char* str, std::size_t len);
    rest_request<char> operator""_OPTIONS(const char* str, std::size_t len);
    rest_request<char> operator""_PATCH(const char* str, std::size_t len);
}
```

---

## Usage

```cpp
#include "siddiqsoft/restcl.hpp"

using namespace siddiqsoft::restcl_literals;

void demoLiterals()
{
    auto reqGet    = "https://httpbin.org/get"_GET;
    auto reqPost   = "https://httpbin.org/post"_POST;
    auto reqPut    = "https://httpbin.org/put"_PUT;
    auto reqDelete = "https://httpbin.org/delete"_DELETE;
}
```
