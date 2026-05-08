# restcl Documentation Index

## Overview

This directory contains comprehensive documentation for the **restcl** library - a modern C++23 header-only REST client library.

## Documentation Files

### 1. [API.md](API.md) - Complete API Reference
**Size:** 707 lines | **Focus:** API usage and reference

**Contents:**
- Quick start guide
- Core concepts (requests, responses, UDLs)
- Complete API reference
- Advanced usage patterns
- Error handling
- Configuration options
- 4 comprehensive examples
- Best practices

**Best For:**
- Learning how to use the library
- API reference lookup
- Understanding core concepts
- Finding usage examples

### 2. [TESTING.md](TESTING.md) - Testing Guide
**Size:** 800 lines | **Focus:** Testing and quality assurance

**Contents:**
- Test architecture and framework
- Test organization (8 test suites, 87 tests)
- Running tests (all platforms)
- Test categories and examples
- Mock testing guide
- Auto-retry testing
- Integration testing
- Writing new tests
- Debugging tests
- Coverage analysis
- Troubleshooting

**Best For:**
- Understanding the test suite
- Running tests
- Writing new tests
- Debugging test failures
- Coverage analysis

### 3. [AUTO_RETRY.md](AUTO_RETRY.md) - Auto-Retry Feature
**Size:** 595 lines | **Focus:** Auto-retry functionality

**Contents:**
- Quick start with retry
- Feature overview
- Complete API reference
- Configuration options
- Retry behavior and logic
- 3 real-world use cases
- Best practices
- 4 detailed examples
- Testing guide
- Limitations and future enhancements

**Best For:**
- Learning about auto-retry
- Configuring retry behavior
- Real-world retry scenarios
- Best practices for resilience
- Testing retry functionality

## Quick Navigation

### By Task

**I want to...**

- **Get started quickly** → [API.md - Quick Start](API.md#quick-start)
- **Understand the API** → [API.md - API Reference](API.md#api-reference)
- **Use auto-retry** → [AUTO_RETRY.md - Quick Start](AUTO_RETRY.md#quick-start)
- **Run tests** → [TESTING.md - Running Tests](TESTING.md#running-tests)
- **Write tests** → [TESTING.md - Writing New Tests](TESTING.md#writing-new-tests)
- **Debug issues** → [TESTING.md - Debugging Tests](TESTING.md#debugging-tests)
- **See examples** → [API.md - Examples](API.md#examples)
- **Understand retry** → [AUTO_RETRY.md - Feature Overview](AUTO_RETRY.md#feature-overview)

### By Topic

**Core Concepts**
- [REST Request Model](API.md#rest-request-model)
- [REST Response Model](API.md#rest-response-model)
- [User-Defined Literals](API.md#user-defined-literals-udls)
- [Content Type Handling](API.md#content-type-handling)
- [Error Handling](API.md#error-handling-with-stdexpected)

**API Methods**
- [GetRESTClient()](API.md#getrestclient)
- [configure()](API.md#configure)
- [send()](API.md#send)
- [sendAsync()](API.md#sendasync)
- [sendAsyncWithRetry()](API.md#sendasyncwithretry)

**Testing**
- [Test Architecture](TESTING.md#test-architecture)
- [Test Organization](TESTING.md#test-organization)
- [Running Tests](TESTING.md#running-tests)
- [Mock Testing](TESTING.md#mock-testing)
- [Auto-Retry Testing](TESTING.md#auto-retry-testing)

**Auto-Retry**
- [Retry Behavior](AUTO_RETRY.md#retry-behavior)
- [Configuration](AUTO_RETRY.md#configuration)
- [Use Cases](AUTO_RETRY.md#use-cases)
- [Best Practices](AUTO_RETRY.md#best-practices)

## Documentation Statistics

| Document | Lines | Sections | Examples | Code Blocks |
|----------|-------|----------|----------|------------|
| API.md | 707 | 12 | 4 | 50+ |
| TESTING.md | 800 | 14 | 8 | 40+ |
| AUTO_RETRY.md | 595 | 11 | 4 | 35+ |
| **TOTAL** | **2,102** | **37** | **16** | **125+** |

## Key Features Documented

### API Features
✅ Synchronous requests (send)
✅ Asynchronous requests (sendAsync)
✅ Auto-retry with configuration (sendAsyncWithRetry)
✅ User-defined literals for clean syntax
✅ JSON-first design
✅ Method chaining
✅ Error handling with std::expected
✅ Custom headers and content types

### Testing Features
✅ 87 comprehensive tests
✅ 8 test suites
✅ Mock client for deterministic testing
✅ Integration tests with external services
✅ Auto-retry scenario tests (18 tests)
✅ Code coverage analysis
✅ Platform-specific testing (Windows, Linux, macOS)

### Auto-Retry Features
✅ Configurable retry attempts
✅ Automatic request preservation
✅ X-restcl-Retry header tracking
✅ Global and inline callback support
✅ Transparent retry logic
✅ Comprehensive error handling

## Getting Started

### For New Users

1. Start with [API.md - Quick Start](API.md#quick-start)
2. Read [API.md - Core Concepts](API.md#core-concepts)
3. Try the [API.md - Examples](API.md#examples)
4. Explore [AUTO_RETRY.md](AUTO_RETRY.md) for resilience

### For Developers

1. Review [TESTING.md - Test Architecture](TESTING.md#test-architecture)
2. Learn [TESTING.md - Running Tests](TESTING.md#running-tests)
3. Study [TESTING.md - Writing New Tests](TESTING.md#writing-new-tests)
4. Check [TESTING.md - Mock Testing](TESTING.md#mock-testing)

### For Contributors

1. Read [API.md - Best Practices](API.md#best-practices)
2. Review [AUTO_RETRY.md - Best Practices](AUTO_RETRY.md#best-practices)
3. Study [TESTING.md - Writing New Tests](TESTING.md#writing-new-tests)
4. Check [TESTING.md - Coverage Analysis](TESTING.md#coverage-analysis)

## Code Examples

### Basic GET Request
```cpp
auto client = siddiqsoft::GetRESTClient();
auto req = "https://api.example.com/users"_GET;
auto result = client->send(req);
if (result.has_value()) {
    std::cout << "Status: " << result->statusCode() << std::endl;
}
```
→ See [API.md - Quick Start](API.md#quick-start)

### POST with JSON
```cpp
auto req = "https://api.example.com/users"_POST;
req.setContent({{"name", "John"}, {"email", "john@example.com"}});
auto result = client->send(req);
```
→ See [API.md - Basic POST Request with JSON](API.md#basic-post-request-with-json)

### Async with Retry
```cpp
client->configure({{"maxRetries", 3}})
      .sendAsyncWithRetry("https://api.example.com/data"_GET,
          [](auto& req, auto resp) {
              if (resp.has_value()) {
                  std::cout << "Success" << std::endl;
              }
          });
```
→ See [AUTO_RETRY.md - Quick Start](AUTO_RETRY.md#quick-start)

### Running Tests
```bash
# Run all tests
ctest --preset Apple-Debug

# Run auto-retry tests
ctest --preset Apple-Debug -R "SendAsyncWithRetry_Scenarios"

# Run specific test
ctest --preset Apple-Debug -R "TransientNetworkFailure_EventualSuccess"
```
→ See [TESTING.md - Running Tests](TESTING.md#running-tests)

## Related Documentation

- **[best_practices.md](../best_practices.md)** - Project-wide best practices
- **[API.md](../API.md)** - Additional API examples
- **[README.md](../README.md)** - Project overview
- **[CMakeLists.txt](../CMakeLists.txt)** - Build configuration

## Documentation Maintenance

### Last Updated
- API.md: May 7, 2024
- TESTING.md: May 7, 2024
- AUTO_RETRY.md: May 7, 2024

### Coverage
- ✅ All public APIs documented
- ✅ All test suites documented
- ✅ All features documented
- ✅ Real-world examples provided
- ✅ Best practices included

## Contributing to Documentation

When adding new features:

1. **Update API.md** - Add API reference and examples
2. **Update TESTING.md** - Add test documentation
3. **Update AUTO_RETRY.md** - If retry-related
4. **Update this index** - Add new sections/links
5. **Add code examples** - Include practical usage

## Support

For questions or issues:

1. Check the relevant documentation file
2. Search for examples in the code
3. Review test cases for usage patterns
4. Check [best_practices.md](../best_practices.md)

## License

All documentation is part of the restcl project and follows the same license.

---

**Last Updated:** May 7, 2024
**Documentation Version:** 1.0
**Library Version:** 1.0+
