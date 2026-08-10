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

   set(CMAKE_CXX_STANDARD 20)
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
