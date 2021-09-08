restcl : A focused REST Client for Modern C++
-------------------------------------------

<!-- badges -->
[![CodeQL](https://github.com/SiddiqSoft/restcl/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/SiddiqSoft/restcl/actions/workflows/codeql-analysis.yml)
[![Build Status](https://dev.azure.com/siddiqsoft/siddiqsoft/_apis/build/status/SiddiqSoft.restcl?branchName=main)](https://dev.azure.com/siddiqsoft/siddiqsoft/_build/latest?definitionId=13&branchName=main)
![](https://img.shields.io/nuget/v/SiddiqSoft.restcl)
![](https://img.shields.io/github/v/tag/SiddiqSoft/restcl)
![](https://img.shields.io/azure-devops/tests/siddiqsoft/siddiqsoft/13)
<!--![](https://img.shields.io/azure-devops/coverage/siddiqsoft/siddiqsoft/13)-->
<!-- end badges -->

# Getting started

- This library uses Windows code and requires VS 2019 v16.11.2 or better.
- On Windows with VisualStudio, use the Nuget package! 
- Make sure you use `c++latest` as the `<format>` is no longer in the `c++20` option pending ABI resolution.

> **NOTE**
> We are going to track VS 2022 and make full use of C++20 facilities and the API is subjec to change.

```markdown
Syntax highlighted code block

## Dependencies

We use [NuGet](https://nuget.org) for dependencies. Why? Simply put, it is the *easiest* source for obtaining packages. Publishing your own packages is superior to the manual and as-yet-immature [vcpkg](https://vcpkg.io/en/index.html). _They want me to [git clone](https://vcpkg.io/en/getting-started.html?platform=windows) the thing and build it first..._ _NuGet, comparatively, gives you a first-class experience and writing your own packages is a breeze! Sure, it does not work for non-Windows and I'll have to eventually tackle CMake._

Package     | Comments
-----------:|:----------
[nlohmann.json](https://github.com/nlohmann/json)<br/>![](https://img.shields.io/nuget/v/nlohmann.json)| This is one of the simplest JSON libraries for C++.<br/>We have to make choices and this is our choice: clean, simple and elegant over efficiency. Our use-case <br/>The library is quite conformant and lends itself to general purpose use since it uses `<vector>` underneath it all.<br/>We leave time and experience (plus manpower) to optimize this library for us. So long as it works and we do not have to worry about some ugly JAVA-esque or C-style interface!
[azure-cpp-utils](https://github.com/SiddiqSoft/azure-cpp-utils)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.AzureCppUtils) | This is library with helpers for encryption, base64 encode/decode and token generation for Cosmos.
[SplitUri](https://github.com/SiddiqSoft/SplitUri)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.SplitUri) | This is library provides parsing of the url.
[string2map](https://github.com/SiddiqSoft/string2map)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.string2map) | This library provides for parsing of HTTP headers into a std::map
[acw32h](https://github.com/SiddiqSoft/acw32h)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.acw32h) | This library provides for an auto-closing wrapper for HINTERNET, HSESSION and HINSTANCE objects.
[RunOnEnd](https://github.com/SiddiqSoft/RunOnEnd)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.RunOnEnd) | This library provides for arbitrarly lambda call on scope exit.

_Unless otherwise noted, use the latest. We're quite aggressive updating dependencies._

<hr/>


# API

Namespace: `siddiqsoft`<br/>
File: `restcl_winhttp.hpp`

## class `siddiqsoft::WinHttpRESTClient`


### Signature

```cpp
    class WinHttpRESTClient : public basic_restclient
    {
        WinHttpRESTClient(const WinHttpRESTClient&) = delete;
        WinHttpRESTClient& operator=(const WinHttpRESTClient&) = delete;

        WinHttpRESTClient(const std::string& ua = {});
        void send(basic_restrequest&& req,
                  std::function<void(const basic_restrequest&,
                                     const basic_restresponse&)>&& callback);
    };
```

### Member Variables

_Private members are implementation-specific and detailed in the source file._

### Member Functions

#### `WinHttpRESTClient::WinHttpRESTClient`
```cpp
    WinHttpRESTClient::WinHttpRESTClient( const std::string& );
```

Creates the Windows REST Client with given UserAgent string and creates a reusable `HSESSION` object.

Sets the HTTP/2 option and the decompression options

##### Parameters

- `ua` User agent string; defaults to `siddiqsoft.restcl_winhttp/0.7.2 (Windows NT; x64)`


#### `WinHttpRESTClient::send`
```cpp
    void send(basic_restrequest&& req, basic_callbacktype&& callback);
```

Uses the existing hSession to connect, send, receive data from the remote server and fire the callback.

##### Parameters

Parameter | Type | Description
---------:|------|:-----------
`req` | [`basic_request`]() | The Request to be sent to the remote server.
`callback` | [`basic_callbacktype`](#alias-basic_callbacktype) | The callback is invoked on completion or an error.

See the [examples](#examples) section.


#### alias `basic_callbacktype`
```cpp
    using basic_callbacktype = std::function<void(const basic_restrequest&  req,
                                                  const basic_restresponse& resp)>;
```

##### Parameters

Parameter | Type | Description
---------:|------|:-----------
`req` | [`const basic_request`]() | The Request to be sent to the remote server.
`resp` | [`const basic_restresponse`]() | The Response from the remote server.


<hr/>

## Examples

```cpp
#include "siddiqsoft/restcl.hpp"
#include "siddiqsoft/restcl_winhttp.hpp"
...
using namespace siddiqsoft;
using namespace siddiqsoft::literals;

WinHttpRESTClient wrc("my-user-agent-string");

// Create a simple GET request from the endpoint string
// Send the request and invoke the callback.
wrc.send( "https://google.com"_GET,
          [](const auto& req, const auto& resp) {
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
wrc.send( std::move(myReq), [](auto& req, auto& resp){
                                if(resp.success())
                                    doSomething();
                                else
                                    logError(resp.error());
                            });
```

## Notes

