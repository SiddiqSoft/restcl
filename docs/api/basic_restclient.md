# Class Template `basic_restclient<CharT>`

Header: `<siddiqsoft/private/basic_restclient.hpp>`  
Namespace: `siddiqsoft`

---

## Class Signature

```cpp
template <typename CharT = char>
class basic_restclient
{
public:
    virtual ~basic_restclient() = default;

    virtual basic_restclient& configure(
        const nlohmann::json& config = {},
        basic_callbacktype&& callback = {}
    ) = 0;

    [[nodiscard]] virtual std::expected<rest_response<CharT>, int> send(
        rest_request<CharT>& req
    ) = 0;

    virtual basic_restclient& sendAsync(
        rest_request<CharT>&& req,
        basic_callbacktype&& callback = {}
    ) = 0;
};
```

---

## Callback Signature

```cpp
using basic_callbacktype = std::function<void(
    rest_request<>& request,
    std::expected<rest_response<>, int> responseOrError
)>;
```

The callback function takes:
1. `rest_request<>&`: Reference to the initiating request.
2. `std::expected<rest_response<>, int>`: Result object holding either the received `rest_response` or an integer transport error code.
