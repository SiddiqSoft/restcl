rem detect the location of Visual Studio vcvars and set the path
@echo off
setlocal

if exist "%VCVARS_PATH%" (
    echo Found: "%VCVARS_PATH%"
    call "%VCVARS_PATH%" %PROCESSOR_ARCHITECTURE%
) else (
    echo Could not find vcvarsall.bat at %VCVARS_PATH%
)

endlocal
