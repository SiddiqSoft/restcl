# RestCL Class Diagram

```mermaid
classDiagram
    %% Enums
    class HttpMethodType {
        <<enumeration>>
        METHOD_GET
        METHOD_HEAD
        METHOD_POST
        METHOD_PUT
        METHOD_DELETE
        METHOD_CONNECT
        METHOD_OPTIONS
        METHOD_TRACE
        METHOD_PATCH
        METHOD_UNKNOWN
    }

    class HttpProtocolVersionType {
        <<enumeration>>
        Http1
        Http11
        Http12
        Http2
        Http3
        UNKNOWN
    }

    %% Core Classes
    class ContentType {
        +string type
        +string body
        +size_t length
        +size_t offset
        +size_t remainingSize
        +operator=(json)
        +operator=(string)
        +parseFromSerializedJson(string)
    }

    class http_frame {
        <<abstract>>
        #HttpProtocolVersionType protocol
        #HttpMethodType method
        #Uri uri
        #json headers
        #shared_ptr~ContentType~ content
        +setProtocol(HttpProtocolVersionType)
        +setMethod(HttpMethodType)
        +getMethod()
        +setUri(Uri)
        +setHeaders(json)
        +setHeader(string, string)
        +setContent(json)
        +getContent()
        +encode()* virtual
    }

    class rest_request {
        +rest_request()
        +rest_request(HttpMethodType, Uri)
        +encode() string
    }

    class rest_response {
        -unsigned _statusCode
        -string _reasonCode
        +success() bool
        +statusCode() unsigned
        +setStatus(int, string)
        +encode() string
        +parse(string)$ static
    }

    class basic_restclient {
        <<abstract>>
        #atomic_bool isInitialized
        #atomic_uint64_t ioAttempt
        #atomic_uint64_t callbackAttempt
        +configure(json, callback)* virtual
        +send(rest_request)* virtual
        +sendAsync(rest_request, callback, uint16_t)* virtual
    }

    class RestPoolArgsType {
        +rest_request~CharT~ request
        +basic_callbacktype callback
        +uint16_t retryCounter
    }

    %% Unix/Linux Implementation
    class LibCurlSingleton {
        -resource_pool~shared_ptr~CURL~~ curlHandlePool
        -bool isInitialized
        +GetInstance()$ static
        +getEasyHandle() CurlContextBundlePtr
    }

    class CurlContextBundle {
        +shared_ptr~CURL~ _hndl
        +shared_ptr~ContentType~ _contents
        +curlHandle() CURL*
        +contents() shared_ptr~ContentType~
        +abandon()
    }

    class HttpRESTClient {
        -shared_ptr~LibCurlSingleton~ singletonInstance
        -basic_callbacktype _callback
        -json _config
        -simple_pool~RestPoolArgsType~ pool
        +configure(json, callback) basic_restclient&
        +send(rest_request) expected~rest_response, int~
        +sendAsync(rest_request, callback, uint16_t) basic_restclient&
        +CreateInstance(json, callback)$ static
    }

    %% Windows Implementation
    class WinHttpRESTClient {
        +string UserAgent
        -ACW32HINTERNET hSession
        -basic_callbacktype _callback
        -json _config
        -simple_pool~RestPoolArgsType~ pool
        +configure(json, callback) basic_restclient&
        +send(rest_request) expected~rest_response, int~
        +sendAsync(rest_request, callback, uint16_t) basic_restclient&
        +CreateInstance(json, callback)$ static
    }

    %% Relationships
    http_frame <|-- rest_request
    http_frame <|-- rest_response
    http_frame --> ContentType : contains
    basic_restclient <|-- HttpRESTClient
    basic_restclient <|-- WinHttpRESTClient
    HttpRESTClient --> LibCurlSingleton : uses
    HttpRESTClient --> CurlContextBundle : uses
    HttpRESTClient --> RestPoolArgsType : queues
    WinHttpRESTClient --> RestPoolArgsType : queues
    rest_request --> HttpMethodType : uses
    rest_response --> HttpProtocolVersionType : uses
    http_frame --> HttpMethodType : uses
    http_frame --> HttpProtocolVersionType : uses
    LibCurlSingleton --> CurlContextBundle : creates
    CurlContextBundle --> ContentType : contains
    RestPoolArgsType --> rest_request : contains
```

## Class Hierarchy Overview

### Core Architecture

1. **http_frame** (Abstract Base)
   - Base class for HTTP protocol handling
   - Manages protocol version, HTTP method, URI, headers, and content
   - Provides fluent API for building HTTP messages

2. **rest_request** (Extends http_frame)
   - Models HTTP requests
   - Supports user-defined literals (_GET, _POST, _PUT, etc.)
   - Encodes to HTTP wire format

3. **rest_response** (Extends http_frame)
   - Models HTTP responses
   - Parses HTTP wire format
   - Validates success status codes (100-399)

4. **basic_restclient** (Abstract Base)
   - Defines REST client interface
   - Declares synchronous send() and asynchronous sendAsync() methods
   - Tracks I/O and callback statistics

### Platform-Specific Implementations

#### Unix/Linux/macOS
- **HttpRESTClient** (Extends basic_restclient)
  - Uses libcurl for HTTP operations
  - **LibCurlSingleton**: Manages global libcurl initialization and handle pooling
  - **CurlContextBundle**: RAII wrapper for checked-out CURL handles
  - Thread pool for async operations

#### Windows
- **WinHttpRESTClient** (Extends basic_restclient)
  - Uses WinHTTP API for HTTP operations
  - Native Windows HTTP implementation
  - Thread pool for async operations

### Supporting Classes

- **ContentType**: Manages HTTP message body with type, length, and streaming state
- **RestPoolArgsType**: Container for async request queue items

## Key Design Patterns

1. **Template Metaprogramming**: Generic CharT template for character type flexibility
2. **RAII**: Resource management for CURL handles and HTTP connections
3. **Factory Pattern**: Platform-specific client creation
4. **Thread Pool**: Async request processing with automatic retry logic
5. **Fluent API**: Method chaining for request building
6. **User-Defined Literals**: Convenient request creation syntax
7. **std::expected**: Error handling without exceptions
