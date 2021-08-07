restcl : PROJECTDESCRIPTION
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

- Modern C++ features.
- Header only.
- Use native implementations for the actual IO: Windows support uses WinHttp library.
- Support for literals to allow `_GET`, `_DELETE`, etc.
- Support for std::format and concepts.
- Be instructional and use as little code as necessary.
- The focus is on the interface to the end user.
- Performance is not the objective.

Simplicity (hide the underlying implementation)

  ```cpp
    #include "siddiqsoft/restcl.hpp"
    #include "siddiqsoft/restcl_winhttp.hpp"

    ...
    ...

    using namespace siddiqsoft;
    
    // Create a simple GET request from the endpoint string
    auto myReq= "https://google.com"_GET;

    // Send the request and invoke the callback.
    SendRequest( myReq, [](auto& req, auto& resp){
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
    // Above is Equivalent to the following code:
    // myPost["content"]= {{"foo", "bar"}, {"goto", 99}};
    // myPost["headers"]["Content-Type"]= "application/json"; // the default so we can skip this
    // myPost["headers"]["Content-Length"]= myPost["content"].dump().length();

    // Send the request and invoke the callback
    SendRequest( myReq, [](auto& req, auto& resp){
                           if(resp.success())
                              doSomething();
                           else
                              logError(resp.error());
                        });
  ```

# Requirements
- C++20 with support for `concepts`, `format`
- The library `nlohmann.json` is used as the primary interface
- The build and tests are for Visual Studio 2019 under x64.

# Usage
- Use the nuget [SiddiqSoft.restcl](https://www.nuget.org/packages/SiddiqSoft.restcl/)
- Copy paste..whatever works.

```cpp
TEST(TSendRequest, test1a)
{
    using namespce siddiqsoft;

    bool passTest = false;

    SendRequest("https://www.siddiqsoft.com/"_GET,
                [&passTest](const auto& req, const auto& resp) {
                    std::cerr << "From callback Serialized json: " << nlohmann::json(req).dump(3) << std::endl;
                    if (resp.success())
                    {
                        passTest = true;
                        std::cerr << "Response\n" << nlohmann::json(resp).dump(3) << std::endl;
                    }
                    else
                    {
                        auto [ec, emsg] = resp.status();
                        std::cerr << "Got error: " << ec << " -- " << emsg << std::endl;
                    }
                });

    EXPECT_TRUE(passTest);
}

```




<p align="right">
&copy; 2021 Siddiq Software LLC. All rights reserved.
</p>
