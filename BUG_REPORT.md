# Bug Report: restcl REST Client Library

## Summary
This document details bugs found in the restcl REST client library implementation.

---

## Bug #1: Integer Overflow in `onSendCallback` - Offset Calculation

**File:** `include/siddiqsoft/private/restcl_unix.hpp`  
**Function:** `onSendCallback` (line ~1050)  
**Severity:** HIGH

### Description
The `onSendCallback` function has a potential integer overflow vulnerability when calculating the offset. The code updates `content->offset` without proper bounds checking before the overflow check.

### Current Code (Lines 1050-1080)
```cpp
if (content->remainingSize) {
    auto dataSizeToCopyToLibCurl = content->remainingSize;
    
    if (dataSizeToCopyToLibCurl > sizeToSendToLibCurlBuffer) {
        dataSizeToCopyToLibCurl = sizeToSendToLibCurlBuffer;
    }

    memcpy(libCurlBuffer, content->body.data() + content->offset, dataSizeToCopyToLibCurl);
    
    content->offset += dataSizeToCopyToLibCurl;  // ← OVERFLOW RISK HERE
    
    // Overflow check happens AFTER the offset is already updated
    if (content->offset >= content->length)
        content->remainingSize = 0;
    else {
        content->remainingSize -= dataSizeToCopyToLibCurl;
    }
}
```

### Issue
1. The offset is incremented **before** checking if it would exceed the content length
2. If `dataSizeToCopyToLibCurl` is large, `content->offset` could overflow or exceed `content->length`
3. The bounds check happens after the damage is done
4. Subsequent calls to `memcpy` could read beyond buffer boundaries

### Recommended Fix
```cpp
if (content->remainingSize) {
    auto dataSizeToCopyToLibCurl = content->remainingSize;
    
    if (dataSizeToCopyToLibCurl > sizeToSendToLibCurlBuffer) {
        dataSizeToCopyToLibCurl = sizeToSendToLibCurlBuffer;
    }

    // Validate offset before memcpy
    if (content->offset > content->body.length()) {
        content->remainingSize = 0;
        return 0;
    }

    memcpy(libCurlBuffer, content->body.data() + content->offset, dataSizeToCopyToLibCurl);
    
    // Check for overflow BEFORE updating offset
    if (content->offset > content->length - dataSizeToCopyToLibCurl) {
        // Would overflow - cap at length
        content->offset = content->length;
        content->remainingSize = 0;
        return dataSizeToCopyToLibCurl;
    }
    
    content->offset += dataSizeToCopyToLibCurl;
    
    if (content->offset >= content->length)
        content->remainingSize = 0;
    else {
        content->remainingSize -= dataSizeToCopyToLibCurl;
    }
}
```

---

## Bug #2: Missing Null/Empty Check Before `memcpy` in `onSendCallback`

**File:** `include/siddiqsoft/private/restcl_unix.hpp`  
**Function:** `onSendCallback` (line ~1070)  
**Severity:** MEDIUM

### Description
The code calls `memcpy` with `content->body.data()` without verifying that the body is not empty. If `content->body` is empty but `content->remainingSize` is non-zero, this could cause undefined behavior.

### Current Code
```cpp
if (content->remainingSize) {
    // ... size calculations ...
    memcpy(libCurlBuffer, content->body.data() + content->offset, dataSizeToCopyToLibCurl);
    // ↑ No check if body is empty!
}
```

### Issue
- `content->body.data()` on an empty string returns a valid pointer to an empty buffer
- However, if `content->offset` is non-zero on an empty body, this is a logic error
- The code should validate that the body is not empty before attempting to copy

### Recommended Fix
```cpp
if (content->remainingSize) {
    auto dataSizeToCopyToLibCurl = content->remainingSize;
    
    // Validate that body is not empty
    if (content->body.empty()) {
        content->remainingSize = 0;
        return 0;
    }
    
    if (dataSizeToCopyToLibCurl > sizeToSendToLibCurlBuffer) {
        dataSizeToCopyToLibCurl = sizeToSendToLibCurlBuffer;
    }

    memcpy(libCurlBuffer, content->body.data() + content->offset, dataSizeToCopyToLibCurl);
    // ... rest of code ...
}
```

---

## Bug #3: Potential Race Condition in `LibCurlSingleton::getEasyHandle`

**File:** `include/siddiqsoft/private/restcl_unix.hpp`  
**Function:** `LibCurlSingleton::getEasyHandle` (line ~850)  
**Severity:** MEDIUM

### Description
The `getEasyHandle` method checks `curlHandlePool.size() > 0` without holding a lock, creating a race condition between the check and the actual checkout.

### Current Code (Lines 850-870)
```cpp
if (isInitialized && (curlHandlePool.size() > 0)) {
    // return an existing handle..
    auto ctxbndl = std::shared_ptr<CurlContextBundle>(
            new CurlContextBundle {curlHandlePool, std::move(curlHandlePool.checkout())});
    // ...
    return ctxbndl;
}
```

### Issue
1. Between checking `size() > 0` and calling `checkout()`, another thread could drain the pool
2. The `checkout()` call might throw an exception or return an invalid handle
3. This violates the TOCTOU (Time-of-Check-Time-of-Use) principle

### Recommended Fix
```cpp
try {
    if (isInitialized) {
        // Attempt checkout directly without pre-checking size
        // The resource_pool should handle empty pool gracefully
        auto ctxbndl = std::shared_ptr<CurlContextBundle>(
                new CurlContextBundle {curlHandlePool, std::move(curlHandlePool.checkout())});
        // ... return ...
        return ctxbndl;
    }
}
catch (std::runtime_error& re) {
    // Pool is empty or checkout failed - fall through to create new handle
    std::print(std::cerr, "{} - Failed to checkout from pool: {}\n", __func__, re.what());
}
// Create new handle...
```

---

## Bug #4: Incorrect Bounds Check in `onSendCallback`

**File:** `include/siddiqsoft/private/restcl_unix.hpp`  
**Function:** `onSendCallback` (line ~1075)  
**Severity:** HIGH

### Description
The bounds check logic is flawed. The condition `if (content->offset > content->length - dataSizeToCopyToLibCurl)` can underflow if `dataSizeToCopyToLibCurl > content->length`.

### Current Code
```cpp
if (content->offset > content->length - dataSizeToCopyToLibCurl) {
    // Prevent overflow - offset would exceed length
    content->remainingSize = 0;
    return dataSizeToCopyToLibCurl;
}
```

### Issue
- If `dataSizeToCopyToLibCurl` (size_t) is greater than `content->length` (size_t), the subtraction underflows
- This creates a very large positive number due to unsigned arithmetic
- The comparison then becomes meaningless

### Example Scenario
```
content->length = 100
dataSizeToCopyToLibCurl = 150
content->offset = 50

content->length - dataSizeToCopyToLibCurl = 100 - 150 = (size_t)-50 = very large number
content->offset (50) > very large number = false (WRONG!)
```

### Recommended Fix
```cpp
// Check if adding dataSizeToCopyToLibCurl to offset would exceed length
if (dataSizeToCopyToLibCurl > content->length || 
    content->offset > content->length - dataSizeToCopyToLibCurl) {
    // Would overflow
    content->remainingSize = 0;
    return dataSizeToCopyToLibCurl;
}
```

---

## Bug #5: Missing Header Validation in `rest_response::parseHeaders`

**File:** `include/siddiqsoft/private/rest_response.hpp`  
**Function:** `parseHeaders` (line ~200)  
**Severity:** LOW

### Description
The header parsing code uses `goto` labels which can make control flow difficult to follow and maintain. While not strictly a bug, it's a code quality issue.

### Current Code (Lines 200-280)
```cpp
label_recummulate_to_unfold_buffer:
    auto hend = useCRLF ? search(hsep, headerEnd, HTTP_NEWLINE.begin(), HTTP_NEWLINE.end())
                        : search(hsep, headerEnd, ELEM_NEWLINE_LF.begin(), ELEM_NEWLINE_LF.end());
    if (hend != headerEnd) {
        // ... folding logic ...
        goto label_recummulate_to_unfold_buffer;  // ← goto used for loop
    }
```

### Issue
- `goto` statements are generally discouraged in modern C++
- The code could be refactored to use a `while` loop instead
- Makes the code harder to understand and maintain

### Recommended Fix
Use a `while` loop instead of `goto`:
```cpp
bool folded = true;
while (folded) {
    auto hend = useCRLF ? search(hsep, headerEnd, HTTP_NEWLINE.begin(), HTTP_NEWLINE.end())
                        : search(hsep, headerEnd, ELEM_NEWLINE_LF.begin(), ELEM_NEWLINE_LF.end());
    if (hend != headerEnd) {
        auto peekPos = hend + lineEndSize;
        if ((peekPos + 1 <= headerEnd) &&
            ((*(peekPos + 1) == ' ') || ((*(peekPos + 1)) == '\t'))) {
            value.append(hsep, hend);
            hsep = peekPos + 1;
            folded = true;
        } else {
            value.append(hsep, hend);
            found = storeHeaderValue(httpm, key, value);
            bufferStart = hend += lineEndSize;
            folded = false;
        }
    } else {
        value.append(hsep, hend);
        found = storeHeaderValue(httpm, key, value);
        bufferStart = headerEnd + headerDelimiterSize;
        folded = false;
    }
}
```

---

## Bug #6: Potential Null Pointer Dereference in `http_frame::setUri`

**File:** `include/siddiqsoft/private/http_frame.hpp`  
**Function:** `setUri` (line ~450)  
**Severity:** MEDIUM

### Description
The `setUri` method accesses `uri.authority.host` and `uri.authority.port` without validating that the URI was parsed correctly.

### Current Code (Lines 450-460)
```cpp
auto& setUri(const Uri<char, AuthorityHttp<char>>& u)
{
    uri              = u;
    headers[HF_HOST] = std::format("{}:{}", uri.authority.host, uri.authority.port);
    return *this;
}
```

### Issue
- If the URI parsing fails or produces invalid authority data, this could create malformed Host headers
- No validation that `uri.authority.host` is not empty
- No validation that `uri.authority.port` is valid

### Recommended Fix
```cpp
auto& setUri(const Uri<char, AuthorityHttp<char>>& u)
{
    uri = u;
    if (!uri.authority.host.empty() && uri.authority.port > 0) {
        headers[HF_HOST] = std::format("{}:{}", uri.authority.host, uri.authority.port);
    } else if (!uri.authority.host.empty()) {
        headers[HF_HOST] = uri.authority.host;
    }
    return *this;
}
```

---

## Bug #7: Missing Validation in `rest_request::encode`

**File:** `include/siddiqsoft/private/rest_request.hpp`  
**Function:** `encode` (line ~120)  
**Severity:** LOW

### Description
The `encode` method throws an exception if content type is set but body is empty, but doesn't validate the URI is valid.

### Current Code (Lines 120-140)
```cpp
std::string encode() const override
{
    std::string rs;

    if (!this->content->type.empty() && this->content->body.empty())
        throw std::invalid_argument("Missing content body when content type is present!");

    // Request Line
    std::format_to(std::back_inserter(rs), "{} {} {}\r\n", this->method, this->uri.urlPart, this->protocol);
    // ...
}
```

### Issue
- If `this->uri.urlPart` is empty, the request line will be malformed
- No validation that the method is not UNKNOWN
- No validation that the protocol is not UNKNOWN

### Recommended Fix
```cpp
std::string encode() const override
{
    std::string rs;

    if (!this->content->type.empty() && this->content->body.empty())
        throw std::invalid_argument("Missing content body when content type is present!");
    
    if (this->uri.urlPart.empty())
        throw std::invalid_argument("URI path cannot be empty!");
    
    if (this->method == HttpMethodType::METHOD_UNKNOWN)
        throw std::invalid_argument("HTTP method cannot be UNKNOWN!");

    // Request Line
    std::format_to(std::back_inserter(rs), "{} {} {}\r\n", this->method, this->uri.urlPart, this->protocol);
    // ...
}
```

---

## Bug #8: Uninitialized Variable in `rest_response::parseStartLine`

**File:** `include/siddiqsoft/private/rest_response.hpp`  
**Function:** `parseStartLine` (line ~160)  
**Severity:** LOW

### Description
The variable `useCRLF` is declared but its initialization logic is unclear.

### Current Code (Lines 160-165)
```cpp
static bool parseStartLine(rest_response<CharT>&        httpm,
                           std::string::iterator&       bufferStart,
                           const std::string::iterator& bufferEnd) noexcept(false)
{
    using namespace std;
    auto useCRLF              = true;  // ← Always true, never changed
    auto lineEndDelimiterSize = HTTP_NEWLINE.size();
```

### Issue
- `useCRLF` is always `true` and never modified
- This suggests incomplete implementation or dead code
- The variable is used later but its purpose is unclear

### Recommended Fix
Either remove the unused variable or implement proper CRLF/LF detection:
```cpp
// Option 1: Remove if not needed
// Option 2: Implement detection
auto useCRLF = std::search(bufferStart, bufferEnd, HTTP_NEWLINE.begin(), HTTP_NEWLINE.end()) != bufferEnd;
```

---

## Bug #9: Missing Validation in `ContentType::parseFromSerializedJson`

**File:** `include/siddiqsoft/private/http_frame.hpp`  
**Function:** `parseFromSerializedJson` (line ~350)  
**Severity:** LOW

### Description
The method silently fails if JSON parsing fails, which could hide errors.

### Current Code (Lines 350-360)
```cpp
void parseFromSerializedJson(const std::string& s)
{
    try {
        auto obj      = nlohmann::json::parse(s);
        body          = obj.dump();
        remainingSize = length = body.length();
        type                   = CONTENT_APPLICATION_JSON;
        offset                 = 0;
    }
    catch (...) {
        // Silently fails!
    }
}
```

### Issue
- Exceptions are silently swallowed
- Caller has no way to know if parsing succeeded or failed
- Could lead to sending empty or invalid content

### Recommended Fix
```cpp
void parseFromSerializedJson(const std::string& s)
{
    try {
        auto obj      = nlohmann::json::parse(s);
        body          = obj.dump();
        remainingSize = length = body.length();
        type                   = CONTENT_APPLICATION_JSON;
        offset                 = 0;
    }
    catch (const std::exception& ex) {
        // Log the error or re-throw
        std::print(std::cerr, "Failed to parse JSON: {}\n", ex.what());
        // Optionally re-throw or set a flag
    }
}
```

---

## Summary Table

| Bug # | Severity | File | Function | Issue |
|-------|----------|------|----------|-------|
| 1 | HIGH | restcl_unix.hpp | onSendCallback | Integer overflow in offset calculation |
| 2 | MEDIUM | restcl_unix.hpp | onSendCallback | Missing empty check before memcpy |
| 3 | MEDIUM | restcl_unix.hpp | getEasyHandle | Race condition in pool size check |
| 4 | HIGH | restcl_unix.hpp | onSendCallback | Incorrect bounds check with underflow |
| 5 | LOW | rest_response.hpp | parseHeaders | Use of goto instead of loop |
| 6 | MEDIUM | http_frame.hpp | setUri | Missing URI validation |
| 7 | LOW | rest_request.hpp | encode | Missing URI/method validation |
| 8 | LOW | rest_response.hpp | parseStartLine | Uninitialized/unused variable |
| 9 | LOW | http_frame.hpp | parseFromSerializedJson | Silent exception handling |

---

## Recommendations

1. **Immediate Action:** Fix bugs #1, #2, and #4 as they pose buffer overflow and memory safety risks
2. **High Priority:** Fix bug #3 to prevent race conditions in multi-threaded scenarios
3. **Medium Priority:** Fix bugs #6 and #7 to improve input validation
4. **Code Quality:** Address bugs #5, #8, and #9 to improve maintainability and error handling

---

## Testing Recommendations

1. Add unit tests for `onSendCallback` with various buffer sizes and content lengths
2. Add stress tests for concurrent `getEasyHandle` calls
3. Add validation tests for malformed URIs
4. Add tests for edge cases in header parsing
5. Add tests for JSON parsing failures in `parseFromSerializedJson`
