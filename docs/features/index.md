# Features Overview

The `restcl` library provides a modern, clean, header-only C++ interface for executing HTTP requests against RESTful endpoints.

---

## Key Features

=== "Literal Operators"

    Construct HTTP requests directly from string literals using expressive suffixes like `_GET`, `_POST`, `_PUT`, and `_DELETE`.

    ```cpp
    auto req = "https://api.example.com/items"_GET;
    ```

    [Read more about User-Defined Literals :octicons-arrow-right-24:](literals.md)

=== "JSON API Metaphor"

    First-class integration with `nlohmann::json`. Requests and responses structure headers, parameters, and content body seamlessly as JSON objects.

    ```cpp
    req.setContent({ {"key", "value"}, {"count", 42} });
    ```

    [Read more about JSON Metaphor :octicons-arrow-right-24:](json_metaphor.md)

=== "Asynchronous & Callback Support"

    Execute requests synchronously or asynchronously using non-blocking I/O with callback functions.

    ```cpp
    client->sendAsync(std::move(req), [](auto& req, auto resp) {
        // Handle response
    });
    ```

    [Read more about Asynchronous Operations :octicons-arrow-right-24:](async.md)

=== "Native Platform Performance"

    Zero overhead wrapper over native system HTTP engines:
    - **Windows**: WinHTTP (`WinHttpRESTClient`)
    - **Unix/Linux/macOS**: libcurl (`HttpRESTClient`)

---

## Summary Matrix

| Feature | Description | Reference |
| :--- | :--- | :--- |
| **User-Defined Literals** | Suffix syntax (`"url"_GET`) for concise request creation | [Literals](literals.md) |
| **JSON Payload Handling** | Built-in serialization with `nlohmann::json` | [JSON Metaphor](json_metaphor.md) |
| **Async Callbacks** | Non-blocking execution using `sendAsync` | [Async Operations](async.md) |
| **Cross-Platform Factory** | Unified `GetRESTClient()` creation | [GetRESTClient](../api/get_rest_client.md) |
