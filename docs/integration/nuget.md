# NuGet Package Integration

For Visual Studio C++ projects on Windows, `restcl` is published on [nuget.org](https://www.nuget.org/packages/SiddiqSoft.restcl/).

---

## Installation

### Package Manager Console

Open the Package Manager Console in Visual Studio and execute:

```powershell
Install-Package SiddiqSoft.restcl
```

### Visual Studio Package Manager UI

1. Right-click your project in Solution Explorer and select **Manage NuGet Packages...**
2. Search for `SiddiqSoft.restcl`
3. Click **Install**.

---

## Visual Studio Project Setup

1. Open project properties (**Project -> Properties**).
2. Set **C++ Language Standard** to **Preview - Features from the Latest C++ Working Draft (/std:c++latest)** or **ISO C++23 Standard (/std:C++23)**.
3. Make sure `nlohmann.json` and required dependencies are restored automatically by NuGet.

```cpp
#include "siddiqsoft/restcl.hpp"

using namespace siddiqsoft;
using namespace siddiqsoft::restcl_literals;

int main()
{
    WinHttpRESTClient wrc("MyWindowsApp/1.0");
    auto resp = wrc.send("https://httpbin.org/get"_GET);
    return 0;
}
```
