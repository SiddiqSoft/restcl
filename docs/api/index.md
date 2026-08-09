# API Reference Overview

The `siddiqsoft::restcl` namespace provides the following primary classes, functions, and models:

---

## Primary Entry Point

| Entity | Description | Reference |
| :--- | :--- | :--- |
| `GetRESTClient()` | Factory function producing a platform-specific REST client instance | [`GetRESTClient`](get_rest_client.md) |

---

## Client Classes

| Class | Description | Platform | Reference |
| :--- | :--- | :--- | :--- |
| `WinHttpRESTClient` | Windows REST client using WinHTTP | Windows | [`WinHttpRESTClient`](winhttp_rest_client.md) |
| `HttpRESTClient` | Unix/Linux/macOS REST client using libcurl | Linux / macOS | [`HttpRESTClient`](http_rest_client.md) |
| `basic_restclient<CharT>` | Abstract base class defining sync & async interface | All | [`basic_restclient`](basic_restclient.md) |

---

## Request & Response Models

| Entity | Description | Reference |
| :--- | :--- | :--- |
| `rest_request<CharT>` | HTTP request container (method, URI, headers, content) | [`rest_request`](rest_request.md) |
| `rest_response<CharT>` | HTTP response container (status code, headers, content) | [`rest_response`](rest_response.md) |
| `basic_callbacktype` | Async callback function type signature | [`basic_callbacktype`](basic_restclient.md#callback-signature) |

---

## Literals & Helpers

| Namespace / Literal | Description | Reference |
| :--- | :--- | :--- |
| `siddiqsoft::restcl_literals` | User-defined literals (`_GET`, `_POST`, etc.) | [`restcl_literals`](restcl_literals.md) |
