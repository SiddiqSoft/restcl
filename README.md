# restcl: A Focused REST Client for Modern C++

<!-- badges -->
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.restcl?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=13&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.restcl)
![](https://img.shields.io/github/v/tag/SiddiqSoft/restcl)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/13)
<!-- end badges -->

## Overview

**restcl** is a header-only REST client library for modern C++23 that provides a clean, JSON-first API for interacting with RESTful servers. It abstracts platform-specific HTTP implementations (WinHTTP on Windows, libcurl on Unix/Linux/macOS) behind a unified, modern C++ interface. The library prioritizes simplicity and instructional clarity over performance, making it ideal for applications that need straightforward REST communication without the complexity of lower-level HTTP libraries.

### Key Features

- **JSON-First API**: JSON is a first-class citizen in the API design
- **Modern C++23**: Leverages C++23 features including `std::expected`, `std::format`, and user-defined literals
- **Header-Only**: Easy integration with no compilation overhead
- **Cross-Platform**: Native implementations for Windows (WinHTTP) and Unix/Linux/macOS (libcurl)
- **User-Defined Literals**: Convenient syntax like `"https://api.example.com"_GET`
- **Async Support**: Non-blocking async operations with callback-based responses
- **Error Handling**: Uses `std::expected<T, E>` for robust error handling without exceptions for IO operations
- **Comprehensive Testing**: Extensive test suite with unit, integration, and stress tests
- **Type-Safe**: Template-based design with support for custom character types

## Table of Contents

- [Quick Start](#quick-start)
- [Installation](#installation)
- [Usage Examples](#usage-examples)
- [Architecture](#architecture)
- [Dependencies](#dependencies)
- [Building](#building)
- [Testing](#testing)
- [Best Practices](#best-practices)
- [Contributing](#contributing)
- [License](#license)

## Quick Start

### Basic GET Request

```cpp
#include <siddiqsoft/restcl.hpp>
using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    
    auto request = "https://api.example.com/users"_GET;
    auto response = client->send(request);
    
    if (response) {
        std::cout << "Status: " << response->statusCode() << "\n";
        std::cout << "Body: " << response->body() << "\n";
    } else {
        std::cerr << "Error: " << response.error() << "\n";
    }
    
    return 0;
}
```

### POST Request with JSON

```cpp
#include <siddiqsoft/restcl.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    
    auto request = "https://api.example.com/users"_POST;
    
    json payload = {
        {"name", "John Doe"},
        {"email", "john@example.com"}
    };
    
    request.setBody(payload.dump());
    request.setHeader("Content-Type", "application/json");
    
    auto response = client->send(request);
    
    if (response) {
        auto result = json::parse(response->body());
        std::cout << "Created user: " << result["id"] << "\n";
    }
    
    return 0;
}
```

### Async Request with Callback

```cpp
#include <siddiqsoft/restcl.hpp>
#include <atomic>

using namespace siddiqsoft::restcl_literals;

int main() {
    auto client = siddiqsoft::GetRESTClient();
    std::atomic<bool> done{false};
    
    auto request = "https://api.example.com/data"_GET;
    
    client->sendAsync(request, [&done](const auto& req, auto response) {
        if (response) {
            std::cout << "Async response: " << response->statusCode() << "\n";
        } else {
            std::cerr << "Async error: " << response.error() << "\n";
        }
        done = true;
    });
    
    // Wait for async operation to complete
    while (!done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return 0;
}
```

## Installation

### Via NuGet (Windows)

```bash
nuget install SiddiqSoft.restcl
```

### Via CMake (Recommended)

Add to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(restcl
    GIT_REPOSITORY https://github.com/SiddiqSoft/restcl.git
    GIT_TAG main
)

FetchContent_MakeAvailable(restcl)

target_link_libraries(your_target PRIVATE restcl::restcl)
```

### Manual Integration

1. Clone the repository
2. Copy the `include/siddiqsoft` directory to your project
3. Ensure your compiler supports C++23
4. Link against platform-specific libraries:
   - **Windows**: WinHTTP (included in Windows SDK)
   - **Unix/Linux/macOS**: libcurl

## Usage Examples

### Supported HTTP Methods

The library provides user-defined literals for all standard HTTP methods:

```cpp
using namespace siddiqsoft::restcl_literals;

auto get_req = "https://api.example.com/resource"_GET;
auto post_req = "https://api.example.com/resource"_POST;
auto put_req = "https://api.example.com/resource/123"_PUT;
auto delete_req = "https://api.example.com/resource/123"_DELETE;
auto patch_req = "https://api.example.com/resource/123"_PATCH;
auto head_req = "https://api.example.com/resource"_HEAD;
auto options_req = "https://api.example.com/resource"_OPTIONS;
```

### Working with Headers

```cpp
auto request = "https://api.example.com/data"_GET;

request.setHeader("Authorization", "Bearer token123");
request.setHeader("Accept", "application/json");
request.setHeader("User-Agent", "MyApp/1.0");

// Access headers
auto auth = request.getHeader("Authorization");
```

### Request Configuration

```cpp
auto client = siddiqsoft::GetRESTClient({
    {"connectTimeout", 3000},      // Connection timeout in ms
    {"timeout", 5000},             // Overall timeout in ms
    {"userAgent", "MyApp/1.0"},    // Custom user agent
    {"trace", false}               // Enable/disable tracing
});
```

### Error Handling

The library uses `std::expected<T, E>` for error handling:

```cpp
auto response = client->send(request);

if (response) {
    // Success path
    std::cout << "Status: " << response->statusCode() << "\n";
} else {
    // Error path
    int error_code = response.error();
    std::cerr << "Request failed with error: " << error_code << "\n";
}
```

### JSON Integration

```cpp
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Parse response as JSON
auto response = client->send(request);
if (response && response->statusCode() == 200) {
    auto data = json::parse(response->body());
    
    // Access JSON data
    std::string name = data["name"];
    int age = data["age"];
}
```

## Architecture

### Project Structure

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
├── tests/                            # Comprehensive test suite
│   ├── test_validation.cpp           # Validation and error handling tests
│   ├── test_restcl.cpp               # Core functionality tests
│   ├── test_serializers.cpp          # JSON serialization tests
│   ├── test_postbin.cpp              # Integration tests with external services
│   ├── test_libcurl_helpers.cpp      # Unix/Linux-specific tests
│   └── test_mock_and_coverage.cpp    # Mock and coverage tests
├── docs/                             # Doxygen documentation
├── pack/                             # NuGet packaging and build helpers
├── CMakeLists.txt                    # Main CMake configuration
├── CMakePresets.json                 # CMake presets for builds
├── .clang-format                     # Code formatting rules
├── .clang-tidy                       # Static analysis configuration
├── best_practices.md                 # Comprehensive development guidelines
└── azure-pipelines.yml               # CI/CD pipeline
```

### Core Components

#### `basic_restclient<CharT>`
Abstract interface defining the REST client contract with `send()` and `sendAsync()` methods.

#### `http_frame<CharT>`
Base class providing common HTTP functionality including headers, content, and protocol version management.

#### `rest_request<CharT>`
Extends `http_frame` with request-specific encoding and HTTP method support.

#### `rest_response<CharT>`
Extends `http_frame` with response parsing, status codes, and reason phrases.

#### Platform-Specific Implementations
- **`HttpRESTClient`**: Unix/Linux/macOS implementation using libcurl
- **`WinHttpRESTClient`**: Windows implementation using WinHTTP with HTTP/2 support

## Dependencies

### Core Dependencies

| Dependency | Version | Purpose |
|-----------|---------|---------|
| nlohmann/json | v3.12.0 | JSON parsing and serialization |
| SplitUri | v3.0.0 | URI parsing and manipulation |
| AzureCppUtils | v3.2.5 | Azure-specific utilities |
| string2map | v2.5.0 | String-to-map conversion utilities |
| RunOnEnd | v1.4.2 | RAII-based cleanup utilities |
| asynchrony | v2.1.1 | Asynchronous operation helpers |
| acw32h | v2.7.4 | Windows-specific C++ wrapper for WinHTTP (Windows only) |
| CURL | v8.7+ | libcurl for Unix/Linux/macOS |

### Build Tools

| Tool | Version | Purpose |
|------|---------|---------|
| CMake | v3.29+ | Build system |
| Clang-Format | Latest | Code formatting |
| Clang-Tidy | Latest | Static analysis |
| Google Test | v1.17.0 | Testing framework |
| Doxygen | Latest | Documentation generation |

### Compiler Requirements

- **C++23 Standard**: Required
- **Visual Studio 2022**: For Windows builds (MSVC)
- **Clang 18+** or **GCC 13+**: For Unix/Linux builds
- **Clang on macOS**: With `-fexperimental-library` flag for `std::stop_token` and `std::jthread`

## Building

### Prerequisites

- CMake 3.29 or later
- C++23 compatible compiler
- Platform-specific dependencies:
  - **Windows**: Windows SDK (includes WinHTTP)
  - **Linux**: libcurl development libraries (`libcurl-devel` or `libcurl4-openssl-dev`)
  - **macOS**: libcurl (usually pre-installed)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/SiddiqSoft/restcl.git
cd restcl

# Configure with CMake
cmake --preset default -DRESTCL_BUILD_TESTS=ON

# Build the project
cmake --build --preset default

# Run tests (optional)
ctest --preset default
```

### CMake Options

- `RESTCL_BUILD_TESTS`: Enable/disable test suite (default: OFF)
- `CMAKE_BUILD_TYPE`: Debug or Release (default: Release)

## Testing

The library includes a comprehensive test suite covering:

### Test Categories

- **Unit Tests** (`test_validation.cpp`): Request/response validation, error handling, edge cases
- **Core Tests** (`test_restcl.cpp`): Core functionality, async operations, multiple HTTP verbs, stress tests
- **Serialization Tests** (`test_serializers.cpp`): JSON serialization and deserialization
- **Integration Tests** (`test_postbin.cpp`): Real HTTP calls to external services
- **Platform-Specific Tests** (`test_libcurl_helpers.cpp`): Unix/Linux-specific libcurl tests
- **Mock Tests** (`test_mock_and_coverage.cpp`): Mock objects and code coverage validation

### Running Tests

```bash
# Build with tests enabled
cmake --preset default -DRESTCL_BUILD_TESTS=ON
cmake --build --preset default

# Run all tests
ctest --preset default

# Run specific test
ctest --preset default -R test_restcl
```

### Test Coverage

- ASAN (Address Sanitizer) and leak detection enabled for Debug builds on Linux/Clang
- Code coverage instrumentation enabled for Debug builds
- Tests pass on both Windows (MSVC) and Unix/Linux (Clang/GCC)

## Best Practices

For comprehensive guidance on code style, testing strategies, common patterns, and best practices when working with or extending this library, see [best_practices.md](best_practices.md).

### Key Principles

#### Error Handling
- Use `std::expected<T, E>` for IO operations instead of exceptions
- Throw `std::invalid_argument` for validation errors
- Different platforms return different error codes:
  - Windows: WinHTTP error codes (12001, 12002, 12029, etc.)
  - Unix/Linux: POSIX error codes (ECONNRESET, etc.)

#### Modern C++ Features
- Leverage C++23 features: `std::expected`, `std::format`, `std::atomic`, concepts
- Use move semantics extensively
- Prefer `std::shared_ptr` for shared ownership
- Use `[[nodiscard]]` on functions returning important values

#### Async Operations
- Use callbacks for async operations: `std::function<void(rest_request<>&, std::expected<rest_response<>, int>)>`
- Callbacks receive both request and response (or error code)
- Register global callbacks via `configure()` and override per-request
- Use `std::atomic<bool>` with `.wait()` and `.notify_all()` for test synchronization

#### Code Style
- Classes: PascalCase (e.g., `rest_request`, `HttpRESTClient`)
- Functions: camelCase (e.g., `setMethod()`, `getUri()`)
- Constants: UPPER_SNAKE_CASE (e.g., `HTTP_NEWLINE`)
- Private members: Prefix with underscore (e.g., `_statusCode`)
- Namespaces: lowercase (e.g., `siddiqsoft`, `restcl_literals`)

#### Documentation
- Use Doxygen comments (`///` or `/**`) for public APIs
- Include `@brief`, `@param`, `@return`, `@throw` tags
- Comment non-obvious algorithms or platform-specific code
- Guard debug output with `#if defined(DEBUG)`

## Design Philosophy

### Motivation

Design a library where JSON is a first-class API metaphor for interacting with RESTful servers.

### Core Principles

1. **Focused Scope**: REST interactions with JSON only. This limitation allows us to simplify usage and make it feel very C++ instead of the C-like API of Win32 or LibCURL.

2. **Modern C++**: C++23 is required, enabling:
   - Visual Studio 2022 on Windows
   - WinHTTP library with HTTP/2 support
   - Modern language features and idioms

3. **Header-Only**: Easy integration with no compilation overhead for the library itself.

4. **Native Implementations**: Use platform-specific libraries for actual IO:
   - Windows: WinHTTP library
   - Unix/Linux/macOS: libcurl

5. **User-Defined Literals**: Support for convenient syntax like `_GET`, `_DELETE`, etc. via the `siddiqsoft::restcl_literals` namespace.

6. **Instructional**: Use as little code as necessary and be clear about intent.

7. **Interface-Focused**: The focus is on the interface to the end user, not internal complexity.

8. **Simplicity Over Performance**: Hide the underlying implementation and prioritize clarity.

## Roadmap

- [ ] Performance optimizations
- [ ] Additional test coverage
- [ ] Extended documentation with more examples
- [ ] Support for additional content types
- [ ] Enhanced error diagnostics

## Contributing

Contributions are welcome! Please ensure:

1. Code follows the style guidelines in [best_practices.md](best_practices.md)
2. All tests pass: `ctest --preset default`
3. New features include appropriate tests
4. Documentation is updated accordingly
5. Code is formatted with clang-format: `clang-format -i <file>`

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Support

For issues, questions, or suggestions:

1. Check the [best_practices.md](best_practices.md) for comprehensive guidance
2. Review existing [GitHub Issues](https://github.com/SiddiqSoft/restcl/issues)
3. Create a new issue with detailed information
4. For security concerns, please email privately

## Acknowledgments

- Built with modern C++23 features
- Uses [nlohmann/json](https://github.com/nlohmann/json) for JSON handling
- Cross-platform support via WinHTTP and libcurl
- Comprehensive testing with Google Test framework
