/**
 * @file test_mock_and_coverage.cpp
 * @brief Mock-based tests and coverage improvements for restcl.
 *        These tests exercise the HTTP frame, request, response parsing,
 *        ContentType, enum conversions, serializers, and a mock rest client
 *        without requiring any network access.
 *
 * @copyright Copyright (c) 2024 Siddiq Software
 */

#include "gtest/gtest.h"
#include <expected>
#include <format>
#include <iostream>
#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "siddiqsoft/SplitUri.hpp"
#include "../include/siddiqsoft/restcl.hpp"


namespace siddiqsoft
{
    using namespace restcl_literals;

    // =========================================================================
    // Mock REST Client
    // =========================================================================

    /// @brief A mock implementation of basic_restclient for testing without network IO.
    class MockRESTClient : public basic_restclient<char>
    {
    public:
        // Configurable canned response
        rest_response<char>    cannedResponse {};
        int                    cannedErrorCode {0};
        bool                   shouldReturnError {false};
        int                    sendCallCount {0};
        int                    sendAsyncCallCount {0};
        int                    configureCallCount {0};
        basic_callbacktype     storedCallback {};
        nlohmann::json         lastConfig {};

        basic_restclient& configure(const nlohmann::json& cfg = {}, basic_callbacktype&& cb = {}) override
        {
            configureCallCount++;
            lastConfig = cfg;
            if (cb) storedCallback = std::move(cb);
            return *this;
        }

        [[nodiscard]] std::expected<rest_response<char>, int> send(rest_request<>& req) override
        {
            sendCallCount++;
            if (shouldReturnError) {
                return std::unexpected(cannedErrorCode);
            }
            return cannedResponse;
        }

        basic_restclient& sendAsync(rest_request<>&& req, basic_callbacktype&& cb = {}) override
        {
            sendAsyncCallCount++;
            auto result = shouldReturnError ? std::expected<rest_response<char>, int>(std::unexpected(cannedErrorCode))
                                            : std::expected<rest_response<char>, int>(cannedResponse);
            if (cb) {
                cb(req, result);
            }
            else if (storedCallback) {
                storedCallback(req, result);
            }
            return *this;
        }
    };


    // =========================================================================
    // ContentType Tests
    // =========================================================================

    TEST(ContentType_Tests, DefaultConstruction)
    {
        ContentType ct;
        EXPECT_TRUE(ct.type.empty());
        EXPECT_TRUE(ct.body.empty());
        EXPECT_EQ(0u, ct.length);
        EXPECT_EQ(0u, ct.offset);
        EXPECT_EQ(0u, ct.remainingSize);
        EXPECT_FALSE(static_cast<bool>(ct));
        EXPECT_EQ("", static_cast<std::string>(ct));
    }

    TEST(ContentType_Tests, AssignFromJson)
    {
        ContentType ct;
        nlohmann::json j = {{"key", "value"}};
        ct = j;
        EXPECT_EQ(CONTENT_APPLICATION_JSON, ct.type);
        EXPECT_FALSE(ct.body.empty());
        EXPECT_EQ(ct.body.length(), ct.length);
        EXPECT_EQ(ct.body.length(), ct.remainingSize);
        EXPECT_TRUE(static_cast<bool>(ct));
        // Verify the body is valid JSON
        auto parsed = nlohmann::json::parse(ct.body);
        EXPECT_EQ("value", parsed.at("key"));
    }

    TEST(ContentType_Tests, AssignFromString)
    {
        ContentType ct;
        std::string s = "Hello World";
        ct = s;
        EXPECT_EQ(CONTENT_APPLICATION_TEXT, ct.type);
        EXPECT_EQ("Hello World", ct.body);
        EXPECT_EQ(11u, ct.length);
        EXPECT_EQ(11u, ct.remainingSize);
        EXPECT_TRUE(static_cast<bool>(ct));
        EXPECT_EQ("Hello World", static_cast<std::string>(ct));
    }

    TEST(ContentType_Tests, CopyFromSharedPtr)
    {
        auto src = std::make_shared<ContentType>();
        src->type = "text/html";
        src->body = "<html></html>";
        src->length = src->body.length();
        src->offset = 5;
        src->remainingSize = 8;

        ContentType ct(src);
        EXPECT_EQ("text/html", ct.type);
        EXPECT_EQ("<html></html>", ct.body);
        EXPECT_EQ(src->length, ct.length);
        EXPECT_EQ(5u, ct.offset);
        EXPECT_EQ(8u, ct.remainingSize);
    }

    TEST(ContentType_Tests, CopyFromNullSharedPtr)
    {
        std::shared_ptr<ContentType> src;
        ContentType ct(src);
        EXPECT_TRUE(ct.body.empty());
        EXPECT_EQ(0u, ct.length);
    }

    TEST(ContentType_Tests, AssignFromSharedPtr)
    {
        auto src = std::make_shared<ContentType>();
        src->type = "application/xml";
        src->body = "<root/>";
        src->length = 7;

        ContentType ct;
        ct = src;
        EXPECT_EQ("application/xml", ct.type);
        EXPECT_EQ("<root/>", ct.body);
    }

    TEST(ContentType_Tests, AssignFromNullSharedPtr)
    {
        std::shared_ptr<ContentType> src;
        ContentType ct;
        ct.body = "should remain";
        ct = src;
        // Null src should not modify ct
        EXPECT_EQ("should remain", ct.body);
    }

    TEST(ContentType_Tests, ParseFromSerializedJson_Valid)
    {
        ContentType ct;
        ct.parseFromSerializedJson("{\"hello\":\"world\"}");
        EXPECT_EQ(CONTENT_APPLICATION_JSON, ct.type);
        EXPECT_FALSE(ct.body.empty());
        EXPECT_EQ(0u, ct.offset);
        auto parsed = nlohmann::json::parse(ct.body);
        EXPECT_EQ("world", parsed.at("hello"));
    }

    TEST(ContentType_Tests, ParseFromSerializedJson_Invalid)
    {
        ContentType ct;
        ct.parseFromSerializedJson("not valid json {{{");
        // Should silently fail
        EXPECT_TRUE(ct.body.empty());
        EXPECT_EQ(0u, ct.length);
    }

    TEST(ContentType_Tests, ToJson)
    {
        ContentType ct;
        ct.type = "application/json";
        ct.body = "{\"a\":1}";
        ct.length = 7;
        ct.offset = 2;
        ct.remainingSize = 5;

        nlohmann::json j;
        to_json(j, ct);
        EXPECT_EQ("application/json", j.at("type"));
        EXPECT_EQ("{\"a\":1}", j.at("body"));
        EXPECT_EQ(7u, j.at("length"));
        EXPECT_EQ(2u, j.at("offset"));
        EXPECT_EQ(5u, j.at("remainingSize"));
    }

    TEST(ContentType_Tests, Formatter)
    {
        ContentType ct;
        ct.type = "text/plain";
        ct.body = "hello";
        ct.length = 5;
        auto formatted = std::format("{}", ct);
        EXPECT_TRUE(formatted.find("text/plain") != std::string::npos);
        EXPECT_TRUE(formatted.find("hello") != std::string::npos);
        EXPECT_TRUE(formatted.find("5") != std::string::npos);
    }


    // =========================================================================
    // Enum to_string / Formatter Tests
    // =========================================================================

    TEST(EnumConversions, HttpMethodType_to_string)
    {
        EXPECT_EQ("GET", to_string(HttpMethodType::METHOD_GET));
        EXPECT_EQ("HEAD", to_string(HttpMethodType::METHOD_HEAD));
        EXPECT_EQ("POST", to_string(HttpMethodType::METHOD_POST));
        EXPECT_EQ("PUT", to_string(HttpMethodType::METHOD_PUT));
        EXPECT_EQ("DELETE", to_string(HttpMethodType::METHOD_DELETE));
        EXPECT_EQ("CONNECT", to_string(HttpMethodType::METHOD_CONNECT));
        EXPECT_EQ("OPTIONS", to_string(HttpMethodType::METHOD_OPTIONS));
        EXPECT_EQ("TRACE", to_string(HttpMethodType::METHOD_TRACE));
        EXPECT_EQ("PATCH", to_string(HttpMethodType::METHOD_PATCH));
        EXPECT_EQ("UNKNOWN", to_string(HttpMethodType::METHOD_UNKNOWN));
    }

    TEST(EnumConversions, HttpProtocolVersionType_to_string)
    {
        EXPECT_EQ("HTTP/1.0", to_string(HttpProtocolVersionType::Http1));
        EXPECT_EQ("HTTP/1.1", to_string(HttpProtocolVersionType::Http11));
        EXPECT_EQ("HTTP/1.2", to_string(HttpProtocolVersionType::Http12));
        EXPECT_EQ("UNKNOWN", to_string(HttpProtocolVersionType::UNKNOWN));
        EXPECT_EQ("UNKNOWN", to_string(HttpProtocolVersionType::Http2));
        EXPECT_EQ("UNKNOWN", to_string(HttpProtocolVersionType::Http3));
    }

    TEST(EnumConversions, HttpMethodType_Formatter)
    {
        EXPECT_EQ("GET", std::format("{}", HttpMethodType::METHOD_GET));
        EXPECT_EQ("POST", std::format("{}", HttpMethodType::METHOD_POST));
        EXPECT_EQ("PUT", std::format("{}", HttpMethodType::METHOD_PUT));
        EXPECT_EQ("DELETE", std::format("{}", HttpMethodType::METHOD_DELETE));
        EXPECT_EQ("HEAD", std::format("{}", HttpMethodType::METHOD_HEAD));
        EXPECT_EQ("CONNECT", std::format("{}", HttpMethodType::METHOD_CONNECT));
        EXPECT_EQ("OPTIONS", std::format("{}", HttpMethodType::METHOD_OPTIONS));
        EXPECT_EQ("TRACE", std::format("{}", HttpMethodType::METHOD_TRACE));
        EXPECT_EQ("PATCH", std::format("{}", HttpMethodType::METHOD_PATCH));
        EXPECT_EQ("UNKNOWN", std::format("{}", HttpMethodType::METHOD_UNKNOWN));
    }

    TEST(EnumConversions, HttpProtocolVersionType_Formatter)
    {
        EXPECT_EQ("HTTP/1.0", std::format("{}", HttpProtocolVersionType::Http1));
        EXPECT_EQ("HTTP/1.1", std::format("{}", HttpProtocolVersionType::Http11));
        EXPECT_EQ("UNKNOWN", std::format("{}", HttpProtocolVersionType::UNKNOWN));
    }

    TEST(EnumConversions, HttpMethodType_JsonRoundtrip)
    {
        nlohmann::json j = HttpMethodType::METHOD_PATCH;
        EXPECT_EQ("PATCH", j.get<std::string>());
        auto m = j.get<HttpMethodType>();
        EXPECT_EQ(HttpMethodType::METHOD_PATCH, m);
    }

    TEST(EnumConversions, HttpProtocolVersionType_JsonRoundtrip)
    {
        nlohmann::json j = HttpProtocolVersionType::Http2;
        EXPECT_EQ("HTTP/2", j.get<std::string>());
        auto p = j.get<HttpProtocolVersionType>();
        EXPECT_EQ(HttpProtocolVersionType::Http2, p);
    }


    // =========================================================================
    // rest_request Literal Operators
    // =========================================================================

    TEST(RequestLiterals, AllVerbs)
    {
        auto get     = "https://example.com/"_GET;
        auto head    = "https://example.com/"_HEAD;
        auto post    = "https://example.com/"_POST;
        auto put     = "https://example.com/"_PUT;
        auto del     = "https://example.com/"_DELETE;
        auto conn    = "https://example.com/"_CONNECT;
        auto options = "https://example.com/"_OPTIONS;
        auto trace   = "https://example.com/"_TRACE;
        auto patch   = "https://example.com/"_PATCH;

        EXPECT_EQ(HttpMethodType::METHOD_GET, get.getMethod());
        EXPECT_EQ(HttpMethodType::METHOD_HEAD, head.getMethod());
        EXPECT_EQ(HttpMethodType::METHOD_POST, post.getMethod());
        EXPECT_EQ(HttpMethodType::METHOD_PUT, put.getMethod());
        EXPECT_EQ(HttpMethodType::METHOD_DELETE, del.getMethod());
        EXPECT_EQ(HttpMethodType::METHOD_CONNECT, conn.getMethod());
        EXPECT_EQ(HttpMethodType::METHOD_OPTIONS, options.getMethod());
        EXPECT_EQ(HttpMethodType::METHOD_TRACE, trace.getMethod());
        EXPECT_EQ(HttpMethodType::METHOD_PATCH, patch.getMethod());
    }


    // =========================================================================
    // rest_request Construction and Encoding
    // =========================================================================

    TEST(RestRequest, TwoArgConstructor)
    {
        using namespace siddiqsoft::splituri_literals;
        rest_request req(HttpMethodType::METHOD_GET, "https://example.com/path?q=1"_Uri);
        EXPECT_EQ(HttpMethodType::METHOD_GET, req.getMethod());
        EXPECT_EQ("example.com", req.getUri().authority.host);
        auto encoded = req.encode();
        EXPECT_TRUE(encoded.find("GET") != std::string::npos);
        EXPECT_TRUE(encoded.find("/path?q=1") != std::string::npos);
    }

    TEST(RestRequest, ThreeArgConstructor)
    {
        using namespace siddiqsoft::splituri_literals;
        rest_request req(HttpMethodType::METHOD_POST,
                         "https://api.example.com/data"_Uri,
                         {{"Authorization", "Bearer token123"}});
        EXPECT_EQ(HttpMethodType::METHOD_POST, req.getMethod());
        EXPECT_EQ("Bearer token123", req.getHeaders().value("Authorization", ""));
    }

    TEST(RestRequest, FourArgConstructor)
    {
        using namespace siddiqsoft::splituri_literals;
        rest_request req(HttpMethodType::METHOD_PUT,
                         "https://api.example.com/item/1"_Uri,
                         {{"Accept", "application/json"}},
                         {{"name", "updated"}});
        EXPECT_EQ(HttpMethodType::METHOD_PUT, req.getMethod());
        EXPECT_EQ("application/json", req.getHeaders().value("Accept", ""));
        EXPECT_FALSE(req.getContentBody().empty());
    }

    TEST(RestRequest, EncodeWithContent)
    {
        auto req = "https://httpbin.org/post"_POST;
        req.setContent({{"key", "value"}});
        auto encoded = req.encode();
        EXPECT_TRUE(encoded.find("POST") != std::string::npos);
        EXPECT_TRUE(encoded.find("Content-Type") != std::string::npos);
        EXPECT_TRUE(encoded.find("Content-Length") != std::string::npos);
        EXPECT_TRUE(encoded.find("key") != std::string::npos);
    }

    TEST(RestRequest, ToJson)
    {
        auto req = "https://example.com/api"_GET;
        nlohmann::json j = req;
        EXPECT_TRUE(j.contains("request"));
        EXPECT_TRUE(j.contains("headers"));
        EXPECT_TRUE(j.contains("content"));
        EXPECT_EQ("GET", j.at("request").at("method").get<std::string>());
    }

    TEST(RestRequest, OstreamOperator)
    {
        auto req = "https://example.com/"_GET;
        std::ostringstream oss;
        oss << req;
        EXPECT_TRUE(oss.str().find("GET") != std::string::npos);
    }

    TEST(RestRequest, StdFormatter)
    {
        auto req = "https://example.com/"_GET;
        auto formatted = std::format("{}", req);
        EXPECT_TRUE(formatted.find("GET") != std::string::npos);
        EXPECT_TRUE(formatted.find("HTTP/1.1") != std::string::npos);
    }


    // =========================================================================
    // http_frame setContent Error Paths
    // =========================================================================

    TEST(HttpFrame, SetContent_EmptyTypeNonEmptyBody)
    {
        auto req = "https://example.com/"_POST;
        EXPECT_THROW(req.setContent("", "some body"), std::invalid_argument);
    }

    TEST(HttpFrame, SetContent_NonEmptyTypeEmptyBody)
    {
        auto req = "https://example.com/"_POST;
        EXPECT_THROW(req.setContent("application/json", ""), std::invalid_argument);
    }

    TEST(HttpFrame, SetContent_BothEmpty)
    {
        auto req = "https://example.com/"_POST;
        EXPECT_NO_THROW(req.setContent("", ""));
    }

    TEST(HttpFrame, SetContent_JsonWithCustomContentType)
    {
        auto req = "https://example.com/"_POST;
        req.setHeaders({{"Content-Type", "application/json+custom"}});
        req.setContent({{"data", 42}});
        EXPECT_EQ("application/json+custom", req.getHeaders().value("Content-Type", ""));
        EXPECT_FALSE(req.getContentBody().empty());
    }

    TEST(HttpFrame, SetContent_StringSrc)
    {
        auto req = "https://example.com/"_POST;
        req.setHeaders({{"Content-Type", "text/plain"}});
        req.setContent(std::string("raw body data"));
        EXPECT_EQ("raw body data", req.getContentBody());
        EXPECT_EQ("text/plain", req.getContent()->type);
    }

    TEST(HttpFrame, SetContent_SharedPtr)
    {
        auto req = "https://example.com/"_POST;
        auto ct = std::make_shared<ContentType>();
        ct->type = "application/octet-stream";
        ct->body = "binary data";
        ct->length = ct->body.length();
        req.setContent(ct);
        EXPECT_EQ("binary data", req.getContentBody());
    }

    TEST(HttpFrame, GetContentBodyJSON_ValidJson)
    {
        auto req = "https://example.com/"_POST;
        req.setContent({{"hello", "world"}});
        auto j = req.getContentBodyJSON();
        EXPECT_FALSE(j.is_null());
        EXPECT_EQ("world", j.at("hello"));
    }

    TEST(HttpFrame, GetContentBodyJSON_NonJsonContentType)
    {
        auto req = "https://example.com/"_POST;
        req.setContent("text/plain", "not json");
        auto j = req.getContentBodyJSON();
        EXPECT_TRUE(j.is_null());
    }

    TEST(HttpFrame, GetContentBodyJSON_EmptyBody)
    {
        auto req = "https://example.com/"_GET;
        auto j = req.getContentBodyJSON();
        EXPECT_TRUE(j.is_null());
    }

    TEST(HttpFrame, EncodeContent)
    {
        auto req = "https://example.com/"_POST;
        req.setContent("text/plain", "test body");
        auto encoded = req.encodeContent();
        EXPECT_EQ("test body", encoded);
    }

    TEST(HttpFrame, SetProtocol_String_Valid)
    {
        auto req = "https://example.com/"_GET;
        req.setProtocol("HTTP/1.0");
        EXPECT_EQ(HttpProtocolVersionType::Http1, req.getProtocol());
    }

    TEST(HttpFrame, SetProtocol_String_Invalid)
    {
        auto req = "https://example.com/"_GET;
        EXPECT_THROW(req.setProtocol("INVALID/9.9"), std::invalid_argument);
    }

    TEST(HttpFrame, SetMethod_String_Valid)
    {
        auto req = "https://example.com/"_GET;
        req.setMethod("DELETE");
        EXPECT_EQ(HttpMethodType::METHOD_DELETE, req.getMethod());
    }

    TEST(HttpFrame, SetMethod_String_Invalid)
    {
        auto req = "https://example.com/"_GET;
        EXPECT_THROW(req.setMethod("INVALID"), std::invalid_argument);
    }

    TEST(HttpFrame, SetHeader_AddAndRemove)
    {
        auto req = "https://example.com/"_GET;
        req.setHeader("X-Custom", "value1");
        EXPECT_EQ("value1", req.getHeaders().value("X-Custom", ""));
        req.setHeader("X-Custom", "");
        EXPECT_FALSE(req.getHeaders().contains("X-Custom"));
    }

    TEST(HttpFrame, GetHeader_Missing)
    {
        auto req = "https://example.com/"_GET;
        EXPECT_THROW(req.getHeader("NonExistent"), nlohmann::json::out_of_range);
    }

    TEST(HttpFrame, EncodeHeaders)
    {
        auto req = "https://example.com/"_GET;
        req.setHeader("X-Test", "abc");
        auto headers = req.encodeHeaders();
        EXPECT_TRUE(headers.find("X-Test: abc\r\n") != std::string::npos);
        EXPECT_TRUE(headers.ends_with("\r\n\r\n"));
    }

    TEST(HttpFrame, ToJson)
    {
        auto req = "https://example.com/"_GET;
        nlohmann::json j;
        // Use the http_frame to_json via rest_request
        j = static_cast<nlohmann::json>(req);
        EXPECT_TRUE(j.contains("request") || j.contains("protocol"));
    }


    // =========================================================================
    // rest_response Tests
    // =========================================================================

    TEST(RestResponse, Parse_SimpleOK)
    {
        std::string raw = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 5\r\n"
                          "\r\n"
                          "hello";
        auto resp = rest_response<>::parse(raw);
        EXPECT_EQ(200u, resp.statusCode());
        EXPECT_EQ("OK", resp.reasonCode());
        EXPECT_TRUE(resp.success());
        EXPECT_EQ("hello", resp.getContentBody());
    }

    TEST(RestResponse, Parse_JsonBody)
    {
        std::string raw = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "Content-Length: 15\r\n"
                          "\r\n"
                          "{\"key\":\"value\"}";
        auto resp = rest_response<>::parse(raw);
        EXPECT_EQ(200u, resp.statusCode());
        EXPECT_TRUE(resp.success());
        auto j = resp.getContentBodyJSON();
        EXPECT_FALSE(j.is_null());
        EXPECT_EQ("value", j.at("key"));
    }

    TEST(RestResponse, Parse_404NotFound)
    {
        std::string raw = "HTTP/1.1 404 Not Found\r\n"
                          "Content-Type: text/html\r\n"
                          "Content-Length: 9\r\n"
                          "\r\n"
                          "Not Found";
        auto resp = rest_response<>::parse(raw);
        EXPECT_EQ(404u, resp.statusCode());
        EXPECT_EQ("Not Found", resp.reasonCode());
        EXPECT_FALSE(resp.success());
    }

    TEST(RestResponse, Parse_301Redirect)
    {
        std::string raw = "HTTP/1.1 301 Moved Permanently\r\n"
                          "Location: https://example.com/new\r\n"
                          "\r\n";
        auto resp = rest_response<>::parse(raw);
        EXPECT_EQ(301u, resp.statusCode());
        EXPECT_TRUE(resp.success()); // 301 < 400
    }

    TEST(RestResponse, Parse_500ServerError)
    {
        std::string raw = "HTTP/1.1 500 Internal Server Error\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 5\r\n"
                          "\r\n"
                          "error";
        auto resp = rest_response<>::parse(raw);
        EXPECT_EQ(500u, resp.statusCode());
        EXPECT_FALSE(resp.success());
    }

    TEST(RestResponse, Parse_NoBody)
    {
        // The parser requires at least one header before the end-of-headers delimiter
        std::string raw = "HTTP/1.1 204 No Content\r\n"
                          "Server: test\r\n"
                          "\r\n";
        auto resp = rest_response<>::parse(raw);
        EXPECT_EQ(204u, resp.statusCode());
        EXPECT_TRUE(resp.success());
        EXPECT_TRUE(resp.getContentBody().empty());
    }

    TEST(RestResponse, Parse_NoHeaders_Throws)
    {
        // A response with no headers at all throws because the parser
        // cannot find the header section delimiter after the start line.
        std::string raw = "HTTP/1.1 204 No Content\r\n"
                          "\r\n";
        EXPECT_THROW(rest_response<>::parse(raw), std::invalid_argument);
    }

    TEST(RestResponse, Parse_MultipleHeaders)
    {
        std::string raw = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: application/json\r\n"
                          "X-Request-Id: abc-123\r\n"
                          "Server: TestServer/1.0\r\n"
                          "Content-Length: 2\r\n"
                          "\r\n"
                          "{}";
        auto resp = rest_response<>::parse(raw);
        EXPECT_EQ(200u, resp.statusCode());
        EXPECT_EQ("abc-123", resp.getHeaders().value("X-Request-Id", ""));
        EXPECT_EQ("TestServer/1.0", resp.getHeaders().value("Server", ""));
    }

    TEST(RestResponse, Parse_InvalidStartLine)
    {
        std::string raw = "GARBAGE DATA\r\n\r\n";
        EXPECT_THROW(rest_response<>::parse(raw), std::invalid_argument);
    }

    TEST(RestResponse, Parse_HTTP10)
    {
        std::string raw = "HTTP/1.0 200 OK\r\n"
                          "Content-Length: 0\r\n"
                          "\r\n";
        auto resp = rest_response<>::parse(raw);
        EXPECT_EQ(200u, resp.statusCode());
    }

    TEST(RestResponse, Success_BoundaryValues)
    {
        rest_response<> resp;
        resp.setStatus(99, "");
        EXPECT_FALSE(resp.success()); // <= 99

        resp.setStatus(100, "Continue");
        EXPECT_TRUE(resp.success()); // 100 is success

        resp.setStatus(399, "");
        EXPECT_TRUE(resp.success()); // 399 is success

        resp.setStatus(400, "Bad Request");
        EXPECT_FALSE(resp.success()); // >= 400

        resp.setStatus(0, "");
        EXPECT_FALSE(resp.success()); // 0 is not success
    }

    TEST(RestResponse, Status_Pair)
    {
        rest_response<> resp;
        resp.setStatus(201, "Created");
        auto [code, msg] = resp.status();
        EXPECT_EQ(201u, code);
        EXPECT_EQ("Created", msg);
    }

    TEST(RestResponse, Encode)
    {
        rest_response<> resp;
        resp.setStatus(200, "OK");
        resp.setProtocol(HttpProtocolVersionType::Http11);
        auto encoded = resp.encode();
        EXPECT_TRUE(encoded.find("HTTP/1.1 200 OK\r\n") != std::string::npos);
    }

    TEST(RestResponse, EncodeWithContent)
    {
        rest_response<> resp;
        resp.setStatus(200, "OK");
        resp.setProtocol(HttpProtocolVersionType::Http11);
        resp.setContent("text/plain", "hello");
        auto encoded = resp.encode();
        EXPECT_TRUE(encoded.find("hello") != std::string::npos);
    }

    TEST(RestResponse, ToJson)
    {
        rest_response<> resp;
        resp.setStatus(200, "OK");
        nlohmann::json j = resp;
        EXPECT_TRUE(j.contains("response"));
        EXPECT_EQ(200, j.at("response").at("statusCode"));
        EXPECT_EQ("OK", j.at("response").at("statusMessage"));
    }

    TEST(RestResponse, OstreamOperator)
    {
        rest_response<> resp;
        resp.setStatus(200, "OK");
        std::ostringstream oss;
        oss << resp;
        EXPECT_TRUE(oss.str().find("200") != std::string::npos);
    }

    TEST(RestResponse, StdFormatter)
    {
        rest_response<> resp;
        resp.setStatus(404, "Not Found");
        resp.setProtocol(HttpProtocolVersionType::Http11);
        auto formatted = std::format("{}", resp);
        EXPECT_TRUE(formatted.find("404") != std::string::npos);
        EXPECT_TRUE(formatted.find("Not Found") != std::string::npos);
    }


    // =========================================================================
    // Mock REST Client Tests
    // =========================================================================

    TEST(MockClient, Configure)
    {
        MockRESTClient client;
        client.configure({{"userAgent", "test/1.0"}, {"timeout", 5000}});
        EXPECT_EQ(1, client.configureCallCount);
        EXPECT_EQ("test/1.0", client.lastConfig.value("userAgent", ""));
    }

    TEST(MockClient, Send_Success)
    {
        MockRESTClient client;
        client.cannedResponse.setStatus(200, "OK");
        client.cannedResponse.setProtocol(HttpProtocolVersionType::Http11);

        auto req = "https://example.com/"_GET;
        auto result = client.send(req);

        EXPECT_EQ(1, client.sendCallCount);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(200u, result->statusCode());
        EXPECT_TRUE(result->success());
    }

    TEST(MockClient, Send_Error)
    {
        MockRESTClient client;
        client.shouldReturnError = true;
        client.cannedErrorCode = 7; // CURLE_COULDNT_CONNECT

        auto req = "https://example.com/"_GET;
        auto result = client.send(req);

        EXPECT_EQ(1, client.sendCallCount);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(7, result.error());
    }

    TEST(MockClient, SendAsync_WithInlineCallback)
    {
        MockRESTClient client;
        client.cannedResponse.setStatus(201, "Created");

        bool callbackInvoked = false;
        unsigned receivedStatus = 0;

        client.sendAsync("https://example.com/resource"_POST,
                         [&](auto& req, std::expected<rest_response<>, int> resp) {
                             callbackInvoked = true;
                             if (resp.has_value()) {
                                 receivedStatus = resp->statusCode();
                             }
                         });

        EXPECT_EQ(1, client.sendAsyncCallCount);
        EXPECT_TRUE(callbackInvoked);
        EXPECT_EQ(201u, receivedStatus);
    }

    TEST(MockClient, SendAsync_WithConfiguredCallback)
    {
        MockRESTClient client;
        client.cannedResponse.setStatus(200, "OK");

        bool callbackInvoked = false;
        client.configure({}, [&](auto& req, std::expected<rest_response<>, int> resp) {
            callbackInvoked = true;
        });

        client.sendAsync("https://example.com/"_GET);

        EXPECT_TRUE(callbackInvoked);
    }

    TEST(MockClient, SendAsync_ErrorPath)
    {
        MockRESTClient client;
        client.shouldReturnError = true;
        client.cannedErrorCode = 28; // CURLE_OPERATION_TIMEDOUT

        int receivedError = 0;
        client.sendAsync("https://example.com/"_GET,
                         [&](auto& req, std::expected<rest_response<>, int> resp) {
                             if (!resp.has_value()) {
                                 receivedError = resp.error();
                             }
                         });

        EXPECT_EQ(28, receivedError);
    }

    TEST(MockClient, Chaining)
    {
        MockRESTClient client;
        client.cannedResponse.setStatus(200, "OK");

        bool called = false;
        client.configure({{"timeout", 3000}})
              .sendAsync("https://example.com/"_GET,
                         [&](auto& req, auto resp) { called = true; });

        EXPECT_EQ(1, client.configureCallCount);
        EXPECT_EQ(1, client.sendAsyncCallCount);
        EXPECT_TRUE(called);
    }

    TEST(MockClient, MultipleSends)
    {
        MockRESTClient client;
        client.cannedResponse.setStatus(200, "OK");

        int callCount = 0;
        for (int i = 0; i < 5; i++) {
            auto req = "https://example.com/"_GET;
            auto result = client.send(req);
            if (result.has_value()) callCount++;
        }

        EXPECT_EQ(5, client.sendCallCount);
        EXPECT_EQ(5, callCount);
    }


    // =========================================================================
    // RestPoolArgsType Tests
    // =========================================================================

    TEST(RestPoolArgsType_Tests, MoveConstruction)
    {
        basic_callbacktype cb = [](auto&, auto) {};
        auto req = "https://example.com/"_GET;
        RestPoolArgsType<char> args(std::move(req), std::move(cb));
        EXPECT_EQ(HttpMethodType::METHOD_GET, args.request.getMethod());
        EXPECT_TRUE(args.callback);
    }

    TEST(RestPoolArgsType_Tests, LvalueCallbackConstruction)
    {
        basic_callbacktype cb = [](auto&, auto) {};
        auto req = "https://example.com/"_POST;
        RestPoolArgsType<char> args(std::move(req), cb);
        EXPECT_EQ(HttpMethodType::METHOD_POST, args.request.getMethod());
        EXPECT_TRUE(args.callback);
    }


    // =========================================================================
    // std::expected<rest_response, int> Formatter
    // =========================================================================

#if defined(__linux__) || defined(__APPLE__)
    TEST(ExpectedFormatter, WithValue)
    {
        rest_response<> resp;
        resp.setStatus(200, "OK");
        std::expected<rest_response<char>, int> result = resp;
        auto formatted = std::format("{}", result);
        EXPECT_TRUE(formatted.find("200") != std::string::npos);
    }

    TEST(ExpectedFormatter, WithError)
    {
        std::expected<rest_response<char>, int> result = std::unexpected(42);
        auto formatted = std::format("{}", result);
        EXPECT_TRUE(formatted.find("42") != std::string::npos);
        EXPECT_TRUE(formatted.find("IO error") != std::string::npos);
    }
#endif

} // namespace siddiqsoft
