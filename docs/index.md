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
> We are going to track VS 2022 and make full use of C++20 facilities and the API is subject to change.


# Dependencies

We use [NuGet](https://nuget.org) for dependencies. Why? Simply put, it is the *easiest* source for obtaining packages. Publishing your own packages is superior to the manual and as-yet-immature [vcpkg](https://vcpkg.io/en/index.html). _They want me to [git clone](https://vcpkg.io/en/getting-started.html?platform=windows) the thing and build it first..._ _NuGet, comparatively, gives you a first-class experience and writing your own packages is a breeze! Sure, it does not work for non-Windows and I'll have to eventually tackle CMake._

Package     | Comments
-----------:|:----------
[nlohmann.json](https://github.com/nlohmann/json)<br/>![](https://img.shields.io/nuget/v/nlohmann.json)| This is one of the simplest JSON libraries for C++.<br/>We have to make choices and this is our choice: clean, simple and elegant over efficiency. Our use-case <br/>The library is quite conformant and lends itself to general purpose use since it uses `<vector>` underneath it all.<br/>We leave time and experience (plus manpower) to optimize this library for us. So long as it works and we do not have to worry about some JAVA-esque or C-style interface!
[azure-cpp-utils](https://github.com/SiddiqSoft/azure-cpp-utils)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.AzureCppUtils) | This is library with helpers for encryption, base64 encode/decode and conversion of utf8<-->wide strings.
[SplitUri](https://github.com/SiddiqSoft/SplitUri)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.SplitUri) | This is library provides parsing of the url.
[string2map](https://github.com/SiddiqSoft/string2map)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.string2map) | This library provides for parsing of HTTP headers into a std::map
[acw32h](https://github.com/SiddiqSoft/acw32h)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.acw32h) | This library provides for an auto-closing wrapper for HINTERNET, HSESSION and HINSTANCE objects.
[RunOnEnd](https://github.com/SiddiqSoft/RunOnEnd)<br/>![](https://img.shields.io/nuget/v/SiddiqSoft.RunOnEnd) | This library provides for arbitrary lambda call on scope exit.

_Unless otherwise noted, use the latest. We're quite aggressive updating dependencies._

<hr/>


# API

Namespace: `siddiqsoft`<br/>
File: `restcl_winhttp.hpp`

> **NOTE** Internal helpers have been omitted for clarity and when not used by the client code.

## class `siddiqsoft::WinHttpRESTClient`

This is the starting point for your client. We make extensive use of initializer list and json to make REST calls more JavaScript-like. Just take a look at the [example](#examples).

### Signature

```cpp
    class WinHttpRESTClient : public basic_restclient
    {
        WinHttpRESTClient(const WinHttpRESTClient&) = delete;
        WinHttpRESTClient& operator=(const WinHttpRESTClient&) = delete;

        WinHttpRESTClient(const std::string& ua = {});
        void send(basic_restrequest&& req, basic_callbacktype&& callback);
    };
```

### Member Variables

_Private members are implementation-specific and detailed in the source file._

### Member Functions

#### `WinHttpRESTClient::WinHttpRESTClient`
```cpp
⎔    WinHttpRESTClient::WinHttpRESTClient( const std::string& );
```

Creates the Windows REST Client with given UserAgent string and creates a reusable `HSESSION` object.

Sets the HTTP/2 option and the decompression options

##### Parameters

- `ua` User agent string; defaults to `siddiqsoft.restcl_winhttp/0.7.4 (Windows NT; x64)`


#### `WinHttpRESTClient::send`
```cpp
⎔    void send(basic_restrequest&& req, basic_callbacktype&& callback);
```

Uses the existing hSession to connect, send, receive data from the remote server and fire the callback.

> _Why no "return object"?_
>
> Invoking a lambda and minimize the data-copy as well as lifetime of the underlying objects.
> The callback has the original request as well as the response as `const` to minimize data race.

##### Parameters

Parameter | Type | Description
---------:|------|:-----------
`req` | [`basic_restrequest`](#class-basic_restrequest) | The Request to be sent to the remote server.
`callback` | [`basic_callbacktype`](#alias-basic_callbacktype) | The callback is invoked on completion or an error.

See the [examples](#examples) section.


## alias `basic_callbacktype`

### Signature
```cpp
    using basic_callbacktype = std::function<void(const basic_restrequest&  req,
                                                  const basic_restresponse& resp)>;
```

Callback invoked by the library on error / success. The request and response are valid for the lifespan of the call but may not be modified.

##### Parameters

Parameter | Type | Description
---------:|------|:-----------
`req` | [`const basic_restrequest`](#class-basic_restrequest) | The Request to be sent to the remote server.
`resp` | [`const basic_restresponse`](#class-basic_restresponse) | The Response from the remote server.

<hr/>


## class `basic_restrequest`

### Signature
```cpp
class basic_restrequest
{
protected:
    basic_restrequest();
    explicit basic_restrequest(const std::string& endpoint);
    explicit basic_restrequest(const Uri<char>& s);
public:
    const auto&        operator[](const auto& key) const;
    auto&              operator[](const auto& key);
    basic_restrequest& setContent(const std::string& contentType, const std::string& content);
    basic_restrequest& setContent(const nlohmann::json& c);
    std::string        getContent() const;
    void               encodeHeaders_to(std::string& rs) const;
    std::string        encode() const;

    Uri<char, AuthorityHttp<char>> uri;

protected:
    nlohmann::json rrd;
```

#### Member Variables

Parameter | Type | Description
---------:|------|:-----------
`uri` | [`Uri<char,AuthorityHttp<char>>`](https://siddiqsoft.github.io/SplitUri/#class-siddiqsofturi) | The Uri for this client
`rrd` | [`nlohmann::json`](https://json.nlohmann.me/) | The json contains the request data: `{"request": {"method": "GET", "uri": {}, "version": ""}, "headers": {}, "content": nullptr}`

The underlying json object has the following structure

Field | Type | Description
------|------|------------
`request` | json | Contains the request line decomposed into the key-values<br/>- `method` - `GET`, `POST`, etc.<br/>- `url` - The path to the document<br/>- `version` - `HTTP/2` or `HTTP/1.1`


#### Member Functions

##### `basic_restrequest::operator[] const`
```cpp
⎔    const auto&        operator[](const auto& key) const;
```

The `key` can be either `std::string` or `json_pointer` type.

Accessor for `request`, `headers` and `content` (returns the key within the underlying json object).

To access the request path: `req["request"]["url"]` and to access the `Content-Type`, you'd use `req["headers"]["Content-Type"]`. You can also use `json_pointer` notation to access the elements: `req["/headers/Content-Type"_json_pointer]`

> API simplicity. Use an access model that is simple and flexible without the need for adding specific methods

##### `basic_restrequest::operator[]`
```cpp
⎔    auto&              operator[](const auto& key);
```

The `key` can be either `std::string` or `json_pointer` type.

Mutator for `request`, `headers` and `content` (returns the key within the underlying json object).

To access the request path: `req["request"]["url"]` and to access the `Content-Type`, you'd use `req["headers"]["Content-Type"]`. You can also use `json_pointer` notation to access the elements: `req["/headers/Content-Type"_json_pointer]`

##### `basic_restrequest::setContent`

```cpp
⎔    basic_restrequest& setContent(const std::string& contentType, const std::string& content);
```

- contentType - Set the content type header
- content - Set the content body

Convenience method to set non-JSON content along with the headers `Content-Type` and `Content-Length`.

Functionally equivalent to the following:
```cpp
    req["content"]= content;
    req["headers"]["Content-Type"]= contentType;
```
> If the header `Content-Length` is not set then the value is calculated during the `encode()` invocation.

##### `basic_restrequest::setContent`

```cpp
⎔    basic_restrequest& setContent(const nlohmann::json& c);
```

- contentType - Set the content type header
- content - Set the content body

Convenience method to set non-JSON content along with the headers `Content-Type` and `Content-Length`.

Functionally equivalent to the following: `req["content"]= content; // where content is json`

##### `basic_restrequest::getContent const`

```cpp
⎔    std::string        getContent() const;
```

Returns a serialized representation of the content.

If the content is json then the method `.dump()` is invoked to serialized the json.

##### `basic_restrequest::encodeHeaders_to`

```cpp
⎔    void               encodeHeaders_to(std::string& rs) const;
```

Helper to encode the headers to a given string using `std::format` and `std::back_inserter`.

##### `basic_restrequest::encode`

```cpp
⎔    std::string        encode() const;
```

Helper encodes the HTTP request with request line, header section and the content.

<hr/>


## class `basic_restresponse`

### Signature
```cpp
class basic_restresponse
{
protected:
    basic_restresponse();
public:
    basic_restresponse(const basic_restresponse& src);
    basic_restresponse(basic_restresponse&& src);

    basic_restresponse&              operator=(basic_restresponse&&);
    basic_restresponse&              operator=(const basic_restresponse&);
    basic_restresponse&              setContent(const std::string& content);
    bool                             success() const;
    const auto&                      operator[](const auto& key) const;
    auto&                            operator[](const auto& key);
    std::string                      encode() const;
    std::tuple<uint32_t,std::string> status() const;

protected:
    uint32_t       ioErrorCode {0};
    std::string    ioError {};
    nlohmann::json rrd {{"response", {{"version", HTTPProtocolVersion::Http2},
                                      {"status", 0},
                                      {"reason", ""}}},
                        {"headers", nullptr},
                        {"content", nullptr}};
```

#### Member Variables

Parameter | Type | Description
---------:|------|:-----------
`ioErrorCode` | `uint32_t` | The status code response from the server
`ioError` | `std::string` | The reason phrase from the server
`rrd` | [`nlohmann::json`]() | The json contains the response data: `{"response": {"reason": "OK", "status": 200, "version": ""}, "headers": {}, "content": nullptr}`

#### Member Functions

> Omit eplanations for constructors. They're pretty standard default/empty, move constructors and move assignment operator.


##### `basic_restresponse::setContent`

```cpp
    basic_restresponse&              setContent(const std::string& content);
```
- content - Set the content body as read by the server.

This method should not be used by the client.

The client must use the headers to figure out the type of the content and its length.


##### `basic_restresponse::success`

```cpp
    bool                             success() const;
```

Returns true if the HTTP status >=99 and <400. False if there is any IO error or status >400 from the remote server.


##### `basic_restresponse::operator[] const`
```cpp
⎔    const auto&        operator[](const auto& key) const;
```

The `key` can be either `std::string` or `json_pointer` type.

Accessor for `response`, `headers` and `content` (returns the key within the underlying json object).

To access the request path: `req["response"]["url"]` and to access the `Content-Type`, you'd use `resp["headers"]["Content-Type"]`. You can also use `json_pointer` notation to access the elements: `resp["/headers/Content-Type"_json_pointer]`

> API simplicity. Use an access model that is simple and flexible without the need for adding specific methods

##### `basic_restresponse::operator[]`
```cpp
⎔    auto&              operator[](const auto& key);
```

The `key` can be either `std::string` or `json_pointer` type.

Mutator for `response`, `headers` and `content` (returns the key within the underlying json object).

To access the request path: `resp["request"]["url"]` and to access the `Content-Type`, you'd use `resp["headers"]["Content-Type"]`. You can also use `json_pointer` notation to access the elements: `req["/headers/Content-Type"_json_pointer]`

##### `basic_restresponse::encode`

```cpp
    std::string                      encode() const;
```

##### `basic_restresponse::status`

```cpp
    std::tuple<uint32_t,std::string> status() const;
```

Returns a tuple of status/ioerror and the reason-phrase or ioerror message.

Equivalent to `resp["response"]["status"]` and `resp["response"]["reason"]` or the WinHTTP error code and the corresponding WinHTTP message.

<hr/>


## Examples

### GET

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
```

### POST

```cpp
    #include "siddiqsoft/restcl.hpp"
    #include "siddiqsoft/restcl_winhttp.hpp"
    ...
    using namespace siddiqsoft;
    using namespace siddiqsoft::literals;

    WinHttpRESTClient wrc("my-user-agent-string");
        ...
    // Create a POST request by parsing out the string
    auto myPost= "https://server:999/path?q=hello-world"_POST;
    // Add custom header
    myPost["headers"]["X-MyHeader"]= "my-header-value";
    // Adds the content with supplied json object and sets the 
    // headers Content-Length and Content-Type
    myPost.setContent( { {"foo", "bar"}, {"goto", 99} } );
    // Send the request and invoke the callback
    wrc.send( std::move(myReq), [](auto& req, auto& resp){
                                    if(resp.success())
                                        doSomething();
                                    else
                                        logError(resp.error());
                                });
```

This is the *actual* [implemenation](https://github.com/SiddiqSoft/CosmosClient/blob/main/src/azure-cosmos-restcl.hpp) of the Azure Cosmos REST create-document call:

Our design is data descriptive and makes use of the initializer list, overloads to make the task of creating a REST call simple. Our goal is to allow you to focus on your task and not worry about the underlying IO call (one of the few things that I like about JavaScript's model).

The code here focusses on the REST API and its structure as required by the [Cosmos REST API]( https://docs.microsoft.com/en-us/rest/api/documentdb/create-a-document). You're not struggling with the library or dealing with yet-another-string class or some convoluted JSON library or a talkative API or legacy C-like APIs.

```cpp
    /// @brief Create an entity in documentdb using the json object as the payload.
    /// @param dbName Database name
    /// @param collName Collection name
    /// @param doc The document must include the `id` and the partition key.
    /// @return status code and the created document as returned by Cosmos
    CosmosResponseType create(const std::string& dbName, const std::string& collName, const nlohmann::json& doc)
    {
        ...
        CosmosResponseType ret {0xFA17, {}};
        auto               ts   = DateUtils::RFC7231();
        auto               pkId = "siddiqsoft.com";
        auto               auth = EncryptionUtils::CosmosToken<char>(
                                                cnxn.current().Key,
                                                "POST",
                                                "docs",
                                                std::format("dbs/{}/colls/{}", dbName, collName),
                                                ts);

        restClient.send( siddiqsoft::ReqPost {
                            std::format("{}dbs/{}/colls/{}/docs",
                                        cnxn.current().currentWriteUri(),
                                        dbName,
                                        collName),
                            { {"Authorization", auth},
                             {"x-ms-date", ts},
                             {"x-ms-documentdb-partitionkey", nlohmann::json {pkId}},
                             {"x-ms-version", config["apiVersion"]},
                             {"x-ms-cosmos-allow-tentative-writes", "true"} },
                            doc},
                         [&ret](const auto& req, const auto& resp) {
                            ret = {std::get<0>(resp.status()), resp.success() ? std::move(resp["content"])
                                                                              : nlohmann::json {}};
                       });

        return ret;
    }
```

## Notes

