restcl : A focussed REST Client for Modern C++
-------------------------------------------
<!-- badges -->
<!--[![CodeQL](https://github.com/SiddiqSoft/restcl/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/SiddiqSoft/restcl/actions/workflows/codeql-analysis.yml)-->
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.restcl?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=13&branchName=main)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.restcl?branchName=develop)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=13&branchName=develop)
![](https://img.shields.io/nuget/v/SiddiqSoft.restcl)
![](https://img.shields.io/github/v/tag/SiddiqSoft/restcl)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/13)
<!--![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/13)-->
<!-- end badges -->

# Objective

Design a library where JSON is a first-class API metaphor for interacting with RESTful servers.

Your client should not have to worry about the details of the underlying transport protocol or even the 

- Modern C++ features: C++20 is required!
- Header only.
- Use native implementations for the actual IO: Windows support uses WinHttp library.
  - Initial implementation is for Windows using WinHTTP.
  - Alternate implementation using OpenSSL tbd.
- Support for literals to allow `_GET`, `_DELETE`, etc.
- Support for std::format and concepts.
- Be instructional and use as little code as necessary.
- The focus is on the interface to the end user.
- Performance is not the objective.
- Simplicity (hide the underlying implementation)
  ```cpp
    #include "siddiqsoft/restcl.hpp"
    #include "siddiqsoft/restcl_winhttp.hpp"
    ...
    using namespace siddiqsoft;
    using namespace siddiqsoft::literals;

    WinHttpRESTClient wrc;

    // Create a simple GET request from the endpoint string
    // Send the request and invoke the callback.
    wrc.send( "https://google.com"_GET,
              [](auto& req, auto& resp) {
                 if(resp.success())
                    doSomething();
              });
     ...
     ...
    // Create a POST request by parsing out the string
    auto myPost= "https://server:999/path?q=hello-world"_POST;
    // Add custom header
    myPost["headers"]["X-MyHeader"]= "my-header-value";
    // Adds the content with supplied json object and sets the 
    // headers Content-Length and Content-Type
    myPost.setContent( {{"foo", "bar"}, {"goto", 99}} );
    // Send the request and invoke the callback
    wrc.send( myReq, [](auto& req, auto& resp){
                        if(resp.success())
                           doSomething();
                        else
                           logError(resp.error());
                     });
  ```


# Usage

## Requirements
- C++20 with support for `concepts`, `format`
- The library `nlohmann.json` is used as the primary interface
- The build and tests are for Visual Studio 2019 under x64.

## Examples
- Use the nuget [SiddiqSoft.restcl](https://www.nuget.org/packages/SiddiqSoft.restcl/)
- Git submodule, Copy paste..whatever works.


<p align="right">
&copy; 2021 Siddiq Software LLC. All rights reserved.
</p>
