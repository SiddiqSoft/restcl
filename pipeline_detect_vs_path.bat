rem detect the location of Visual Studio vcvars and set the path
@echo off
setlocal

rem Path to vswhere.exe (usually installed with VS)
set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

rem Find the installation path of the latest Visual Studio version
for /f "usebackq tokens=*" %%i in (`"%vswhere%" -latest -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo Visual Studio installation not found.
    exit /b 1
)

rem vcvarsall.bat location for VS 2017+
set "VCVARS_PATH=%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat"

endlocal
