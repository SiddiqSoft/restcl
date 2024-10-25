restcl : A focused REST Client for Modern C++
-------------------------------------------
<!-- badges -->
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.restcl?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=13&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.restcl)
![](https://img.shields.io/github/v/tag/SiddiqSoft/restcl)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/13)
<!--![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/13)-->
<!-- end badges -->

# Motivation

Design a library where JSON is a first-class API metaphor for interacting with RESTful servers.
- Focused on REST interactions with JSON. Limiting allows us to simplify the usage and make it feel very C++ instead of the C-like API of Win32 or OpenSSL.
- Modern C++ features: C++20 is required!
    - Visual Studio 2022
    - Windows version uses WinHTTP library and HTTP/2
- Header only
- Use native implementations for the actual IO: Windows support uses WinHttp library.
  - Initial implementation is for Windows using WinHTTP.
  - Alternate implementation using OpenSSL tbd.
- Support for literals to allow `_GET`, `_DELETE`, etc. Using the `siddiqsoft::httpqrequest::literals` namespace.
- Support for std::format and concepts.
- Be instructional and use as little code as necessary.
- The focus is on the interface to the end user.
- Performance is not the objective.
- Simplicity (hide the underlying implementation)

# Roadmap

- Switch to CMake build
- Do a UNIX version, of course!