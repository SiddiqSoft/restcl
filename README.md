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

# Requirements
- C++20 with support for `concepts`, `format`
- The library `nlohmann.json` is used as the primary interface
- The build and tests are for Visual Studio 2019 under x64.

# Usage
- Use the nuget [SiddiqSoft.restcl](https://www.nuget.org/packages/SiddiqSoft.restcl/)
- Copy paste..whatever works.

```cpp
TEST(TSendRequest, test2a)
{
    bool passTest = false;

    SendRequest(RESTRequestType {RESTMethodType::Options, "https://reqbin.com/echo/post/json"},
                [&passTest](const auto& req, auto& resp) {
                    // Checks the implementation of the encode() implementation
                    std::cerr << "From callback Wire serialize              : " << req.encode() << std::endl;
                    if (resp.isSuccessful())
                    {
                        passTest = true;
                        std::cerr << "Response\n" << resp << std::endl;
                    }
                    else
                    {
                        auto [ec, emsg] = resp.getIOError();
                        std::cerr << emsg << std::endl;
                    }
                });

    EXPECT_TRUE(passTest);
}
```




<p align="right">
&copy; 2021 Siddiq Software LLC. All rights reserved.
</p>
