@echo off
REM Setup script for vcpkg integration
REM This script sets up environment variables for vcpkg

set VCPKG_ROOT=D:\Project\vcpkg-master
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

echo ========================================
echo vcpkg Setup Script
echo ========================================
echo.
echo VCPKG_ROOT: %VCPKG_ROOT%
echo CMAKE_TOOLCHAIN_FILE: %CMAKE_TOOLCHAIN_FILE%
echo.

REM Check if vcpkg exists
if not exist "%VCPKG_ROOT%" (
    echo ERROR: vcpkg directory not found at %VCPKG_ROOT%
    echo Please ensure vcpkg is installed at the correct location.
    exit /b 1
)

if not exist "%CMAKE_TOOLCHAIN_FILE%" (
    echo ERROR: vcpkg toolchain file not found at %CMAKE_TOOLCHAIN_FILE%
    echo Please ensure vcpkg is properly installed.
    exit /b 1
)

echo vcpkg found successfully!
echo.
echo To use vcpkg with CMake, set the following environment variable:
echo   set CMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN_FILE%
echo.
echo Or use it directly in CMake command:
echo   cmake -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%" ...
echo.
echo To install dependencies, run:
echo   %VCPKG_ROOT%\vcpkg.exe install --triplet x64-windows
echo.
echo Current dependencies (from vcpkg.json):
echo   - boost-geometry (^>=1.89.0)
echo   - openblas (^>=0.3.30)
echo   - cgal (^>=6.1.0)
echo   - eigen3 (^>=3.4.0)
echo   - gmsh (^>=4.15.0)
echo   - opencascade (^>=7.8.0)
echo   - clipper2 (^>=1.5.0)
echo   - triangle (^>=1.6.0)
echo   - openmp (Windows)
echo   - cuda (^>=12.0, Windows x64)
echo   - opencl (^>=3.0)
echo   - petsc (^>=3.24.0)
echo.
echo Setting environment variables for current session...
setx VCPKG_ROOT "%VCPKG_ROOT%" >nul 2>&1
setx CMAKE_TOOLCHAIN_FILE "%CMAKE_TOOLCHAIN_FILE%" >nul 2>&1
set VCPKG_ROOT=%VCPKG_ROOT%
set CMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN_FILE%

echo.
echo Environment variables set for current session.
echo Note: setx requires a new command prompt to take effect globally.
echo.
echo Setup complete!
pause

