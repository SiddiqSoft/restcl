@echo off
cd /d c:\Users\maas\source\repos\siddiqsoft\restcl
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsX64_arm64.bat"
cmake --preset Windows-arm64-Debug
cmake --build --preset Windows-arm64-Debug
ctest --preset Windows-arm64-Debug
