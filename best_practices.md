# 📘 Project Best Practices

## 1. Project Purpose

**restcl** is a focused, header-only REST client library for modern C++ (C++23). It provides a clean, JSON-first API for interacting with RESTful servers. The library prioritizes simplicity and instructional clarity over performance, using native platform implementations (WinHTTP on Windows, libcurl on Unix/Linux/macOS) to abstract away low-level HTTP details while maintaining a modern C++ interface.

## 2. Project Structure

```
restcl/
├── include/siddiqsoft/
│   ├── restcl.hpp                    # Main public header (platform dispatcher)
│   └── private/
│       ├── basic_restclient.hpp      # Abstract base class for REST clients
│       ├── http_frame.hpp            # Base class for HTTP requests/responses
│       ├── rest_request.hpp          # REST request model with literals support
│       ├── rest_response.hpp         # REST response model with parsing
│       ├── restcl_unix.hpp           # libcurl-based implementation
│       └── restcl_win.hpp            # WinHTTP-based implementation
├── tests/                            # Google Test suite
│       ├── test_validation.cpp       # Validation and error handling tests
│       ├── test_restcl.cpp           # Core functionality tests
│       ├── test_serializers.cpp      # JSON serialization tests
│       ├── test_postbin.cpp          # Integration tests with external services
│       ├── test_libcurl_helpers.cpp  # Unix/Linux-specific tests
│       └── test_mock_and_coverage.cpp # Mock and coverage tests
├── docs/                             # Doxygen documentation
├── pack/                             # NuGet packaging and build helpers
├── CMakeLists.txt                    # Main CMake configuration
├── CMakePresets.json                 # CMake presets for builds
├── .clang-format                     # Code formatting rules
├── .clang-tidy                       # Static analysis configuration
└── azure-pipelines.yml               # CI/CD pipeline
```

### Key Directories

- **include/siddiqsoft/**: Public API headers. The library is header-only.
- **include/siddiqsoft/private/**: Internal implementation details. Platform-specific implementations are separated.
- **tests/**: Comprehensive test suite using Google Test framework.
- **docs/**: Doxygen-generated documentation with custom styling.
- **pack/**: NuGet package specification and build helpers.

## 3. Test Strategy

### Framework
- **Google Test (gtest)** v1.17.0 for unit and integration testing
- Tests are organized by functionality (validation, serialization, integration)

### Test Organization
- **test_validation.cpp**: Tests for request/response validation, error handling, and edge cases
- **test_restcl.cpp**: Core functionality tests including async operations, multiple HTTP verbs, and stress tests
- **test_serializers.cpp**: JSON serialization and deserialization tests
- **test_postbin.cpp**: Integration tests against external services (httpbin.org, reqbin.com, etc.)
- **test_libcurl_helpers.cpp**: Unix/Linux-specific libcurl helper tests
- **test_mock_and_coverage.cpp**: Mock object tests and code coverage validation

### Mocking Guidelines
- Use `std::atomic<bool>` and `std::atomic<int>` for synchronization in async tests
- Callbacks use `passTest.wait()` and `passTest.notify_all()` for test synchronization
- External service tests (Google, DuckDuckGo, httpbin.org) are used for integration validation

### Unit vs Integration Tests
- **Unit Tests**: Validation of request/response encoding, header parsing, URI parsing
- **Integration Tests**: Real HTTP calls to external services to validate end-to-end functionality
- **Stress Tests**: Multiple concurrent async requests to validate thread safety and callback handling

### Coverage Expectations
- ASAN (Address Sanitizer) and leak detection enabled for Debug builds on Linux/Clang
- Code coverage instrumentation enabled for Debug builds
- Tests must pass on both Windows (MSVC) and Unix/Linux (Clang/GCC)

## 4. Code Style

### Language-Specific Rules (C++23)

- **Modern C++ Features**: Leverage C++23 features including:
  - `std::expected<T, E>` for error handling (no exceptions for IO errors)
  - `std::format` for string formatting (not `printf` or `std::stringstream`)
  - `std::atomic<T>` for thread-safe synchronization
  - Concepts and constraints where applicable
  - `std::jthread` and `std::stop_token` for thread management

- **Async/Await Pattern**: 
  - Use callbacks (`std::function<void(rest_request<>&, std::expected<rest_response<>, int>)>`) for async operations
  - Callbacks receive both the request and response (or error code)
  - Global callbacks can be registered via `configure()` and overridden per-request

- **Type Safety**:
  - Use template parameters for character types (`CharT = char`)
  - Leverage `std::expected` for error handling instead of exceptions for IO operations
  - Use `[[nodiscard]]` on functions that return important values

- **Memory Management**:
  - Prefer `std::shared_ptr` for shared ownership (e.g., `ContentType`)
  - Use move semantics extensively (`std::move`, rvalue references)
  - Header-only library means no dynamic allocation overhead for the library itself

### Naming Conventions

- **Classes**: PascalCase (e.g., `rest_request`, `HttpRESTClient`, `WinHttpRESTClient`)
- **Functions**: camelCase (e.g., `setMethod()`, `getUri()`, `sendAsync()`)
- **Constants**: UPPER_SNAKE_CASE (e.g., `HTTP_NEWLINE`, `CONTENT_APPLICATION_JSON`)
- **Enums**: PascalCase with `Type` suffix (e.g., `HttpMethodType`, `HttpProtocolVersionType`)
- **Template Parameters**: Single uppercase letter or descriptive name (e.g., `CharT`, `T`, `E`)
- **Private Members**: Prefix with underscore (e.g., `_statusCode`, `_reasonCode`)
- **Namespaces**: lowercase (e.g., `siddiqsoft`, `restcl_literals`)

### Commenting and Documentation

- **Doxygen Comments**: Use `///` or `/**` for public API documentation
- **Inline Comments**: Use `//` for implementation details
- **Function Documentation**: Include `@brief`, `@param`, `@return`, `@throw` tags
- **Complex Logic**: Comment non-obvious algorithms or platform-specific code
- **Debug Output**: Use `std::print(std::cerr, ...)` for debug logging (guarded by `#if defined(DEBUG)`)

### Error and Exception Handling

- **IO Errors**: Return `std::expected<rest_response<>, int>` with error codes
  - Windows: Use WinHTTP error codes (e.g., 12001, 12002, 12029)
  - Unix/Linux: Use POSIX error codes (e.g., `ECONNRESET`)
- **Validation Errors**: Throw `std::invalid_argument` for invalid input (e.g., missing content body when content type is set)
- **Parsing Errors**: Throw `std::invalid_argument` for malformed HTTP frames
- **No Exception Guarantees**: IO operations use `std::expected` to avoid exceptions for expected failures

## 5. Common Patterns

### REST Request/Response Model

- **http_frame<CharT>**: Base class providing common HTTP functionality (headers, content, protocol version)
- **rest_request<CharT>**: Extends `http_frame` with request-specific encoding
- **rest_response<CharT>**: Extends `http_frame` with response parsing and status codes

### User-Defined Literals (UDLs)

The library provides convenient UDLs in the `restcl_literals` namespace:
```cpp
using namespace siddiqsoft::restcl_literals;
auto req = "https://api.example.com/users"_GET;
auto req = "https://api.example.com/users"_POST;
auto req = "https://api.example.com/users/123"_DELETE;
```

Supported literals: `_GET`, `_HEAD`, `_POST`, `_PUT`, `_DELETE`, `_CONNECT`, `_OPTIONS`, `_TRACE`, `_PATCH`

### Platform Abstraction

- **basic_restclient<CharT>**: Abstract interface defining `send()` and `sendAsync()` methods
- **HttpRESTClient**: Unix/Linux implementation using libcurl
- **WinHttpRESTClient**: Windows implementation using WinHTTP
- **GetRESTClient()**: Factory function that returns the appropriate implementation based on platform

### Configuration Pattern

```cpp
auto client = GetRESTClient({
    {"connectTimeout", 3000},
    {"timeout", 5000},
    {"userAgent", "MyApp/1.0"},
    {"trace", false}
});

client->configure({...}, [](const auto& req, std::expected<rest_response<>, int> resp) {
    // Handle response
});
```

### JSON Integration

- **nlohmann/json**: First-class JSON support via `nlohmann::json`
- **Serialization**: `to_json()` functions for converting requests/responses to JSON
- **std::formatter**: Custom formatters for `rest_request`, `rest_response`, `HttpMethodType`, etc.
- **Content Handling**: Automatic JSON parsing for `application/json` content types

### Content Type Handling

- **ContentType**: Struct holding type, body, length, offset, and remaining size
- **Automatic Detection**: Response parser detects JSON content and parses accordingly
- **Custom Types**: Support for `application/json+custom` and other custom content types

## 6. Do's and Don'ts

### ✅ Do's

- **Use `std::expected<T, E>`** for IO operations instead of exceptions
- **Use `std::format`** for all string formatting
- **Use `std::atomic<T>`** for thread-safe synchronization in tests
- **Use move semantics** when passing requests to `sendAsync()`
- **Register callbacks** via `configure()` for global async handling
- **Use UDLs** for convenient request creation (e.g., `"https://..."_GET`)
- **Leverage templates** for character type flexibility (`CharT`)
- **Document public APIs** with Doxygen comments
- **Test on both platforms** (Windows MSVC and Unix/Linux Clang/GCC)
- **Use `[[nodiscard]]`** on functions returning important values
- **Guard debug output** with `#if defined(DEBUG)` or `#if defined(_DEBUG)`

### ❌ Don'ts

- **Don't use exceptions** for expected IO failures (use `std::expected`)
- **Don't use `printf`** or `std::stringstream` (use `std::format`)
- **Don't use raw pointers** for ownership (use `std::shared_ptr` or stack allocation)
- **Don't hardcode platform-specific code** in public headers (use conditional includes)
- **Don't ignore move semantics** (use rvalue references and `std::move`)
- **Don't create unnecessary copies** of requests/responses
- **Don't block in callbacks** (keep them lightweight)
- **Don't assume synchronous behavior** in async operations (use proper synchronization)
- **Don't forget to wait for async operations** in tests (use `passTest.wait()`)
- **Don't mix HTTP/1.0 and HTTP/2** protocol versions without explicit handling
- **Don't set Content-Type without content body** (validation will throw)

## 7. Tools & Dependencies

### Core Dependencies

- **nlohmann/json** (v3.12.0): JSON parsing and serialization
- **SplitUri** (v3.0.0): URI parsing and manipulation
- **AzureCppUtils** (v3.1.0): Azure-specific utilities
- **string2map** (v2.5.0): String-to-map conversion utilities
- **RunOnEnd** (v1.4.2): RAII-based cleanup utilities
- **asynchrony** (v2.1.1): Asynchronous operation helpers
- **acw32h** (v2.7.4): Windows-specific C++ wrapper for WinHTTP (Windows only)
- **CURL** (v8.7+): libcurl for Unix/Linux/macOS

### Build Tools

- **CMake** (v3.29+): Build system
- **Clang-Format**: Code formatting (configuration in `.clang-format`)
- **Clang-Tidy**: Static analysis (configuration in `.clang-tidy`)
- **Google Test** (v1.17.0): Testing framework
- **Doxygen**: Documentation generation

### Compiler Requirements

- **C++23 Standard**: Required
- **Visual Studio 2022**: For Windows builds (MSVC)
- **Clang 18+** or **GCC 13+**: For Unix/Linux builds
- **Clang on macOS**: With `-fexperimental-library` flag for `std::stop_token` and `std::jthread`

### Project Setup

```bash
# Clone the repository
git clone https://github.com/SiddiqSoft/restcl.git
cd restcl

# Configure with tests enabled
cmake --preset default -DRESTCL_BUILD_TESTS=ON

# Build
cmake --build --preset default

# Run tests
ctest --preset default
```

## 8. Other Notes

### Important for LLM Code Generation

- **Header-Only Library**: All implementation is in headers under `include/siddiqsoft/`. No `.cpp` files for the library itself.
- **Platform Abstraction**: Always check for platform-specific code paths using preprocessor directives:
  - `#if defined(__linux__) || defined(__APPLE__)` for Unix/Linux/macOS
  - `#if (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64))` for Windows
- **Callback Synchronization**: In tests, use `std::atomic<bool>` with `.wait()` and `.notify_all()` for proper async test synchronization
- **Error Codes**: Different platforms return different error codes:
  - Windows: WinHTTP error codes (12001, 12002, 12029, etc.)
  - Unix/Linux: POSIX error codes (ECONNRESET, etc.)
- **Content Handling**: Always validate that if `Content-Type` is set, a body must be provided, and vice versa
- **URI Parsing**: Uses `SplitUri` library; URIs are parsed into components (scheme, authority, path, query)
- **JSON First**: The library assumes JSON is the primary content type; other types are supported but secondary
- **No Connection Pooling**: Each client instance manages its own connections; use `GetRESTClient()` to obtain instances
- **Async Callbacks**: Callbacks are invoked from worker threads; ensure thread safety in callback implementations
- **Test Timeouts**: External service tests may fail due to network issues; use reasonable timeout values (3-5 seconds)

### Special Edge Cases

- **Folded Headers**: HTTP header folding (continuation lines) is supported in response parsing
- **Custom Content Types**: Support for `application/json+custom` and other variants
- **Empty Responses**: Responses without content bodies are valid (e.g., 204 No Content)
- **Redirect Handling**: The library does not automatically follow redirects; status codes like 302 are returned to the caller
- **SSL/TLS**: Handled by underlying platform libraries (WinHTTP or libcurl); no custom certificate validation
- **User-Agent**: Can be configured globally or per-request via headers
- **Timeouts**: Separate `connectTimeout` and overall `timeout` configuration options
- **Fresh Connections**: `freshConnect` option forces new connections instead of reusing existing ones

### Performance Considerations

- **Not Performance-Focused**: The library prioritizes clarity and correctness over performance
- **Header-Only**: No compilation overhead; templates are instantiated at consumer's compile time
- **Move Semantics**: Extensive use of move semantics to minimize copies
- **Async Operations**: Use async for I/O-bound operations to avoid blocking
- **Callback Overhead**: Keep callbacks lightweight; heavy processing should be deferred

### Documentation

- **Doxygen**: Run `rebuild-docs.ps1` (Windows) or equivalent to generate HTML documentation
- **Custom Styling**: Uses doxygen-awesome CSS for modern documentation appearance
- **API Reference**: All public classes and functions are documented with Doxygen comments
