# Examples Overview

`restcl` provides sample projects demonstrating real-world usage patterns, configuration setups, and build integrations across supported platforms.

---

## Available Examples

| Example | Platform | Description | Link |
| :--- | :--- | :--- | :--- |
| **Cosmos Probes** | Cross-Platform | Health check probe querying a `/ready` endpoint using `GetRESTClient()`, configurable timeouts, and C++23 literal operators. | [Cosmos Probes Documentation](cosmosprobes.md) |

---

## Building the Examples

All examples are configured with CMake and `CPM.cmake` for dependency management. They can be built using standard CMake commands or CMake Presets.

### Quick Start (Command Line)

To build an example directly using CMake:

=== "Linux / macOS"

    ```bash
    cd examples/cosmosprobes
    cmake -B build -DCMAKE_BUILD_TYPE=Debug
    cmake --build build
    ```

=== "Windows (MSVC)"

    ```powershell
    cd examples\cosmosprobes
    cmake -B build -G "Visual Studio 17 2022"
    cmake --build build --config Debug
    ```

### Using CMake Presets

Examples include predefined `CMakePresets.json` configurations matching standard development environments:

```bash
cd examples/cosmosprobes
cmake --preset Darwin
cmake --build --preset Darwin-Debug
```

---

## Integration Patterns Illustrated

Each example demonstrates key `restcl` concepts:

* **Factory Initialization**: Using `siddiqsoft::GetRESTClient()` for cross-platform portability.
* **Singleton Management**: Initializing `LibCurlSingleton::GetInstance()` on POSIX platforms.
* **Configuration**: Customizing timeouts (`connectTimeout`, `timeout`) and trace logging via JSON object configurations.
* **Expressive Request Syntax**: Constructing HTTP requests with user-defined literals like `"http://localhost:8080/ready"_GET`.
* **Response & Error Handling**: Inspecting status codes, headers, and error strings with `std::expected` semantics.
