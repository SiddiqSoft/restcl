restcl : A focused REST Client for Modern C++
-------------------------------------------
<!-- badges -->
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.restcl?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=13&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.restcl)
![](https://img.shields.io/github/v/tag/SiddiqSoft/restcl)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/13)
<!--![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/13)-->
<!-- end badges -->

## Overview

**restcl** is a header-only REST client library for modern C++23 that provides a clean, JSON-first API for interacting with RESTful servers. It abstracts platform-specific HTTP implementations (WinHTTP on Windows, libcurl on Unix/Linux/macOS) behind a unified, modern C++ interface. The library prioritizes simplicity and clarity over performance, making it ideal for applications that need straightforward REST communication without the complexity of lower-level HTTP libraries.

### Key Features

- **JSON-First API**: JSON is a first-class citizen in the API design
- **Modern C++23**: Leverages C++23 features including `std::expected`, `std::format`, and user-defined literals
- **Header-Only**: Easy integration with no compilation overhead
- **Cross-Platform**: Native implementations for Windows (WinHTTP) and Unix/Linux/macOS (libcurl)
- **User-Defined Literals**: Convenient syntax like `"https://api.example.com"_GET`
- **Async Support**: Non-blocking async operations with callback-based responses
- **Error Handling**: Uses `std::expected<T, E>` for robust error handling without exceptions
- **Comprehensive Testing**: Extensive test suite with unit, integration, and stress tests

# Motivation

Design a library where JSON is a first-class API metaphor for interacting with RESTful servers.
- Focused on REST interactions with JSON. Limiting allows us to simplify the usage and make it feel very C++ instead of the C-like API of Win32 or LibCURL.
- Modern C++ features: C++23 is required!
    - Visual Studio 2022
    - Windows version uses WinHTTP library and HTTP/2
- Header only
- Use native implementations for the actual IO: Windows support uses WinHttp library.
  - Initial implementation is for Windows using WinHTTP.
  - Alternate implementation using LibCURL.
- Support for literals to allow `_GET`, `_DELETE`, etc. Using the `siddiqsoft::restcl_literals` namespace.
- Support for std::format and concepts.
- Be instructional and use as little code as necessary.
- The focus is on the interface to the end user.
- Performance is not the objective.
- Simplicity (hide the underlying implementation)

# Documentation

For comprehensive guidance on code style, testing strategies, common patterns, and best practices when working with or extending this library, see [best_practices.md](best_practices.md).

# Roadmap

- Optimize
- Add more tests