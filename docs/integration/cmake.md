# CMake & Submodules

`restcl` provides first-class modern CMake targets for seamless inclusion in your build configuration.

---

## Using `add_subdirectory`

1. Add `restcl` as a Git submodule in your project repository:
   ```bash
   git submodule add https://github.com/SiddiqSoft/restcl.git vendor/restcl
   ```

2. Add the following to your `CMakeLists.txt`:
   ```cmake
   cmake_minimum_required(VERSION 3.20)
   project(MyRestApp LANGUAGES CXX)

   set(CMAKE_CXX_STANDARD 23)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   # Include restcl
   add_subdirectory(vendor/restcl)

   add_executable(MyRestApp main.cpp)
   target_link_libraries(MyRestApp PRIVATE siddiqsoft::restcl)
   ```

---

## Using CPM / FetchContent

You can also pull `restcl` directly via CMake `FetchContent`:

```cmake
include(FetchContent)

FetchContent_Declare(
    restcl
    GIT_REPOSITORY https://github.com/SiddiqSoft/restcl.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(restcl)

target_link_libraries(MyRestApp PRIVATE siddiqsoft::restcl)
```

or use the [cpm-cmake](https://github.com/cpm-cmake/cpm.cmake):

```cmake
# download CPM.cmake..
file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.3/CPM.cmake ${CMAKE_CURRENT_SOURCE_DIR}/pack/CPM.cmake)
# import the helper into our process..
include(pack/CPM.cmake)

..
..

CPMAddPackage("gh:SiddiqSoft/restcl#2.3.8")
target_link_libraries(${PROJECT_NAME} INTERFACE restcl::restcl)

```

---

## Compiler Flags

Ensure C++23 standard support is enabled:

=== "MSVC (Windows)"
    ```cmake
    target_compile_options(MyRestApp PRIVATE /std:c++latest)
    ```

=== "Clang / GCC (Linux/macOS)"
    ```cmake
    target_compile_options(MyRestApp PRIVATE -std=C++23)
    ```

---

## Build Options & Debug Tracing

`restcl` provides a build option `restcl_DEBUG_TRACE` to enable payload trace logging for HTTP requests and responses.

```cmake
# Enable debug payload tracing in CMake
set(restcl_DEBUG_TRACE ON CACHE BOOL "" FORCE)
```

### Options Matrix

| Option | Default | Description |
| :--- | :--- | :--- |
| `restcl_BUILD_TESTS` | `OFF` | Build test suite (`BUILD_TESTS`). |
| `restcl_DEBUG_TRACE` | `OFF` | Enable HTTP verb, header, and payload trace logging via `ScopeTrace`. |

---

## CPM Dependency Cache

`restcl` configures `CPM_SOURCE_CACHE` by default to avoid re-downloading dependencies during clean builds:

* **Windows**: `%LOCALAPPDATA%\CPM\.cpmcache` or `%USERPROFILE%\AppData\Local\CPM\.cpmcache`
* **Linux / macOS**: `$HOME/.cache/.cpmcache`
* **Fallback**: `${CMAKE_BINARY_DIR}/.cpmcache`

You can override `CPM_SOURCE_CACHE` by defining it before building or in CMake command arguments:

```bash
cmake -B build -DCPM_SOURCE_CACHE=/custom/cache/path
```

---

## Building & Running Tests

To build and run the test suite:

```bash
# Configure with test suite enabled
cmake --preset Darwin -Drestcl_BUILD_TESTS=ON

# Build the tests
cmake --build --preset Darwin

# Run tests via ctest
ctest --preset Darwin
```

### Test Categories

* **Unit Tests** (`test_validation.cpp`): Request and response validation, header serialization, error states.
* **Core Functionality** (`test_restcl.cpp`): Synchronous and asynchronous operations, HTTP verbs, client configuration.
* **Serialization** (`test_serializers.cpp`): JSON body encoding and decoding.
* **Integration Tests** (`test_postbin.cpp`): Network IO and live service response handling.
* **Platform Helpers** (`test_libcurl_helpers.cpp`): `libcurl` singleton and callback wrappers.
* **Coverage & Mocks** (`test_mock_and_coverage.cpp`): AddressSanitizer and code coverage validation.



