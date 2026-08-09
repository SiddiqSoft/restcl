# Integration Guide

`restcl` is a header-only library designed for quick integration into modern C++ projects.

---

## Integration Options

Choose your preferred integration method:

=== "CMake / Submodules"

    Ideal for cross-platform CMake projects on Windows, Linux, or macOS.

    ```cmake
    add_subdirectory(vendor/restcl)
    target_link_libraries(your_target PRIVATE siddiqsoft::restcl)
    ```

    [View CMake Integration Guide :octicons-arrow-right-24:](cmake.md)

=== "NuGet Package"

    Ideal for Visual Studio C++ projects on Windows.

    ```powershell
    Install-Package SiddiqSoft.restcl
    ```

    [View NuGet Integration Guide :octicons-arrow-right-24:](nuget.md)

---

## Header Includes

Include the primary header to automatically load the appropriate implementation for your platform:

```cpp
#include "siddiqsoft/restcl.hpp"
```

If you only target Windows with WinHTTP explicitly:

```cpp
#include "siddiqsoft/restcl_winhttp.hpp"
```
