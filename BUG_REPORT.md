# Bug Report: restcl REST Client Library

## Summary
This document details bugs and potential issues found in the restcl REST client library.

---

## Critical Bugs

### 1. **Memory Leak in CurlContextBundle Destructor** 
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (lines 220-235)  
**Severity:** HIGH  
**Issue:** The destructor attempts to return a moved-from `shared_ptr` to the pool.

```cpp
~CurlContextBundle()
{
    if (_contents) _contents.reset();
    if (_hndl) {
        _pool.checkin(std::move(_hndl));  // ← BUG: _hndl is moved
        _hndl.reset();                     // ← This resets an already-moved object
    }
}
```

**Problem:** After `std::move(_hndl)` is called, the `_hndl` shared_ptr is in a moved-from state (empty). Calling `.reset()` on an empty shared_ptr is harmless, but the real issue is that if `checkin()` throws an exception, the handle is lost.

**Fix:** Remove the redundant `.reset()` call or restructure to ensure exception safety:
```cpp
~CurlContextBundle()
{
    if (_contents) _contents.reset();
    if (_hndl) {
        _pool.checkin(std::move(_hndl));
        // Don't call reset() on a moved-from object
    }
}
```

---

### 2. **Potential Null Pointer Dereference in onSendCallback**
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (lines 520-560)  
**Severity:** MEDIUM  
**Issue:** The function doesn't validate `content->body.data()` before using it.

```cpp
static size_t onSendCallback(char* libCurlBuffer, size_t size, size_t nmemb, void* contentPtr)
{
    if (ContentType* content {reinterpret_cast<ContentType*>(contentPtr)};
        (libCurlBuffer != nullptr) && (contentPtr != nullptr) && (size > 0))
    {
        auto sizeToSendToLibCurlBuffer = size * nmemb;
        if (content->remainingSize) {
            auto dataSizeToCopyToLibCurl = content->remainingSize;
            if (dataSizeToCopyToLibCurl > sizeToSendToLibCurlBuffer) {
                dataSizeToCopyToLibCurl = sizeToSendToLibCurlBuffer;
            }
            memcpy(libCurlBuffer, content->body.data() + content->offset, dataSizeToCopyToLibCurl);
            // ↑ BUG: content->body could be empty, making .data() return nullptr
```

**Problem:** If `content->body` is empty, `content->body.data()` returns `nullptr`, and `memcpy` will crash.

**Fix:** Add validation:
```cpp
if (content->remainingSize && !content->body.empty()) {
    memcpy(libCurlBuffer, content->body.data() + content->offset, dataSizeToCopyToLibCurl);
    // ...
}
```

---

### 3. **Integer Overflow in onSendCallback**
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (line 545)  
**Severity:** MEDIUM  
**Issue:** Potential integer overflow when calculating offset.

```cpp
content->offset += dataSizeToCopyToLibCurl;
// If offset + dataSizeToCopyToLibCurl > SIZE_MAX, this overflows
```

**Problem:** If `content->offset` is close to `SIZE_MAX` and `dataSizeToCopyToLibCurl` is large, the addition overflows.

**Fix:** Add bounds checking:
```cpp
if (content->offset > content->length - dataSizeToCopyToLibCurl) {
    // Prevent overflow
    content->remainingSize = 0;
} else {
    content->offset += dataSizeToCopyToLibCurl;
}
```

---

### 4. **Race Condition in LibCurlSingleton::getEasyHandle**
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (lines 280-310)  
**Severity:** MEDIUM  
**Issue:** Non-atomic check-then-act pattern on `curlHandlePool.size()`.

```cpp
if (isInitialized && (curlHandlePool.size() > 0)) {
    // Between this check and checkout(), another thread could empty the pool
    auto ctxbndl = std::shared_ptr<CurlContextBundle>(
            new CurlContextBundle {curlHandlePool, std::move(curlHandlePool.checkout())});
```

**Problem:** The pool size check is not atomic with the checkout operation. Another thread could drain the pool between the check and the actual checkout, causing `checkout()` to throw or return null.

**Fix:** Let `checkout()` handle the empty pool case gracefully, or use a try-catch:
```cpp
try {
    if (isInitialized) {
        auto ctxbndl = std::shared_ptr<CurlContextBundle>(
                new CurlContextBundle {curlHandlePool, std::move(curlHandlePool.checkout())});
        return ctxbndl;
    }
} catch (std::runtime_error& re) {
    // Handle empty pool
}
```

---

### 5. **Uninitialized Variable in extractStartLine**
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (lines 1050-1080)  
**Severity:** LOW  
**Issue:** Variable `sc` is initialized to 0 but may not be set if `curl_easy_getinfo` fails.

```cpp
void extractStartLine(CurlContextBundlePtr ctxCurl, rest_response<>& dest)
{
    CURLcode rc {CURLE_OK};
    long     sc {0};  // ← Initialized to 0

    if (rc = curl_easy_getinfo(ctxCurl->curlHandle(), CURLINFO_RESPONSE_CODE, &sc); rc == CURLE_OK) {
        // ...
        dest.setStatus(sc, "");  // ← Uses sc
    }
    // If rc != CURLE_OK, sc is never set but dest.setStatus() is never called
}
```

**Problem:** While the code is safe (sc is initialized), the logic is unclear. If `curl_easy_getinfo` fails, `dest.setStatus()` is never called, leaving the response with status code 0.

**Fix:** Add explicit error handling:
```cpp
if (rc = curl_easy_getinfo(ctxCurl->curlHandle(), CURLINFO_RESPONSE_CODE, &sc); rc != CURLE_OK) {
    std::print(std::cerr, "{} - Failed to get response code: {}\n", __func__, curl_easy_strerror(rc));
    dest.setStatus(0, "Unknown");
    return;
}
```

---

## Logic Bugs

### 6. **Incorrect Retry Logic in Thread Pool**
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (lines 420-480)  
**Severity:** MEDIUM  
**Issue:** The retry loop has a subtle bug in the condition.

```cpp
while (arg.retryCounter--) {  // ← Decrements AFTER checking
    // ...
    if (auto resp = send(arg.request); resp && resp->success()) {
        // Success - break
        break;
    } else {
        failCount++;
        // Inform the client of the failure
        dispatchCallback(arg.callback, arg.request, std::unexpected<int>(0));
    }
    ++retryCount;
}
```

**Problem:** When a request fails, the callback is dispatched immediately with `std::unexpected<int>(0)`. However, the loop continues and may retry. This means the callback is called multiple times for a single request, which is confusing and may not match user expectations.

**Expected Behavior:** The callback should only be called once, either on success or after all retries are exhausted.

**Fix:** Only dispatch callback on final success or after all retries fail:
```cpp
bool success = false;
while (arg.retryCounter--) {
    if (auto resp = send(arg.request); resp && resp->success()) {
        success = true;
        dispatchCallback(arg.callback, arg.request, resp);
        break;
    }
    failCount++;
}
if (!success) {
    dispatchCallback(arg.callback, arg.request, std::unexpected<int>(0));
}
```

---

### 7. **Missing Null Check in extractHeadersFromLibCurl**
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (lines 575-590)  
**Severity:** LOW  
**Issue:** No null check on `currentHeader` before dereferencing.

```cpp
auto& extractHeadersFromLibCurl(CurlContextBundlePtr ctxCurl, http_frame<>& dest)
{
    int          headerCount {0};
    curl_header* currentHeader {nullptr};
    curl_header* previousHeader {nullptr};

    do {
        if (currentHeader = curl_easy_nextheader(ctxCurl->curlHandle(), CURLH_HEADER, -1, previousHeader); currentHeader) {
            dest.setHeader(currentHeader->name, currentHeader->value);  // ← Safe due to if condition
            previousHeader = currentHeader;
            headerCount++;
        }
    } while (currentHeader);  // ← Loop continues while currentHeader is non-null
    
    return dest;
}
```

**Problem:** The code is actually safe because the `if` condition checks `currentHeader` before dereferencing. However, the logic is confusing. The loop should exit when `currentHeader` is null.

**Fix:** Clarify the logic:
```cpp
while (true) {
    if (currentHeader = curl_easy_nextheader(ctxCurl->curlHandle(), CURLH_HEADER, -1, previousHeader); !currentHeader) {
        break;
    }
    dest.setHeader(currentHeader->name, currentHeader->value);
    previousHeader = currentHeader;
    headerCount++;
}
```

---

### 8. **Potential Issue with Empty Content-Type in setContent**
**File:** `include/siddiqsoft/private/http_frame.hpp` (lines 380-395)  
**Severity:** LOW  
**Issue:** Inconsistent validation logic.

```cpp
auto& setContent(const std::string& ctype, const std::string& c)
{
    if (ctype.empty() && !c.empty()) throw std::invalid_argument("Content-Type cannot be empty");
    if (!ctype.empty() && c.empty())
        throw std::invalid_argument(std::format("Content-Type is {} but no content provided!", ctype).c_str());

    if (!ctype.empty() && !c.empty() && content) {
        // Set content
    }
    return *this;
}
```

**Problem:** The function throws an exception if `ctype` is empty but `c` is not. However, it silently does nothing if both are empty. This is inconsistent with the intent.

**Fix:** Clarify the contract:
```cpp
auto& setContent(const std::string& ctype, const std::string& c)
{
    if (ctype.empty() && !c.empty()) 
        throw std::invalid_argument("Content-Type cannot be empty when content is provided");
    if (!ctype.empty() && c.empty())
        throw std::invalid_argument(std::format("Content-Type is {} but no content provided!", ctype));

    if (!ctype.empty() && !c.empty() && content) {
        content->body = c;
        content->offset = 0;
        content->type = ctype;
        content->remainingSize = content->length = c.length();
        headers[HF_CONTENT_TYPE] = content->type;
        headers[HF_CONTENT_LENGTH] = content->length;
    }
    return *this;
}
```

---

## Design Issues

### 9. **Callback Called Multiple Times on Retry**
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (lines 420-480)  
**Severity:** MEDIUM  
**Issue:** The callback is invoked on every failed retry attempt, not just on final completion.

```cpp
if (auto resp = send(arg.request); resp && resp->success()) {
    // Success
    dispatchCallback(arg.callback, arg.request, resp);
    break;
} else {
    failCount++;
    // BUG: Callback is called here on every failure
    dispatchCallback(arg.callback, arg.request, std::unexpected<int>(0));
}
```

**Problem:** Users expect the callback to be called once per request, but it's called multiple times if retries are enabled. This violates the principle of least surprise.

**Fix:** Only call callback on final success or after all retries exhausted (see Bug #6).

---

### 10. **Inconsistent Error Handling in prepareCurlHeaders**
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (lines 1000-1040)  
**Severity:** LOW  
**Issue:** Exceptions are silently caught and ignored.

```cpp
try {
    for (auto& [k, v] : req.getHeaders().items()) {
        // ... header processing ...
    }
}
catch (...) {
    // Silently ignore all exceptions
}
```

**Problem:** If an exception occurs during header processing, it's silently ignored. This makes debugging difficult.

**Fix:** Log the exception:
```cpp
catch (const std::exception& ex) {
    std::print(std::cerr, "{} - Exception while processing headers: {}\n", __func__, ex.what());
}
```

---

## Minor Issues

### 11. **Unused Variable in onReceiveCallback**
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (line 500)  
**Severity:** TRIVIAL  
**Issue:** The `headerCount` variable is declared but never used.

```cpp
auto& extractHeadersFromLibCurl(CurlContextBundlePtr ctxCurl, http_frame<>& dest)
{
    int          headerCount {0};  // ← Declared but never used
    // ...
    headerCount++;  // ← Incremented but never read
}
```

**Fix:** Remove the unused variable or use it for logging.

---

### 12. **Inconsistent Null Checks**
**File:** `include/siddiqsoft/private/restcl_unix.hpp` (lines 220-235)  
**Severity:** TRIVIAL  
**Issue:** Inconsistent null check patterns.

```cpp
if (_contents) _contents.reset();  // ← Checks before reset
if (_hndl) {
    _pool.checkin(std::move(_hndl));
    _hndl.reset();  // ← No check before reset
}
```

**Problem:** `reset()` on a null shared_ptr is safe, but the inconsistency suggests unclear intent.

**Fix:** Be consistent:
```cpp
if (_contents) _contents.reset();
if (_hndl) {
    _pool.checkin(std::move(_hndl));
}
```

---

## Recommendations

1. **Add comprehensive error handling** for all libcurl operations
2. **Fix the retry callback logic** to only call callbacks once per request
3. **Add bounds checking** for integer operations
4. **Improve thread safety** in the singleton and pool management
5. **Add logging** for debugging difficult issues
6. **Add unit tests** for edge cases (empty content, null pointers, etc.)
7. **Document the callback behavior** clearly in the API documentation

---

## Testing Suggestions

1. Test with empty request bodies
2. Test with very large request bodies (> 1GB)
3. Test concurrent requests with retries enabled
4. Test with network failures and timeouts
5. Test with malformed responses
6. Test with missing headers
7. Stress test with thousands of concurrent requests

