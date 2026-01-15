@echo off
echo Building PulseMoM for Windows...
echo.

REM Check for Visual Studio
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: Visual Studio compiler not found.
    echo Please run this script from a Visual Studio Developer Command Prompt.
    echo.
    echo Alternative: Use CMake to generate Visual Studio solution:
    echo   cmake -G "Visual Studio 17 2022" -A x64 .
    echo   cmake --build . --config Release
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring project with CMake...
set VCPKG_ROOT=D:\Project\vcpkg-master
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
if exist "%CMAKE_TOOLCHAIN_FILE%" (
    echo Using vcpkg toolchain: %CMAKE_TOOLCHAIN_FILE%
    cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%" ..
) else (
    echo WARNING: vcpkg toolchain not found, building without vcpkg
    cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
)
if %errorlevel% neq 0 (
    echo CMake configuration failed.
    exit /b 1
)

REM Build the project
echo.
echo Building project...
cmake --build . --config Release --parallel
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable location: build\Release\PulseMoM.exe
echo.
echo To run validation tests:
echo   build\Release\PulseMoM.exe

REM Run validation tests if build succeeded
echo.
echo Running validation tests...
Release\PulseMoM.exe

cd ..