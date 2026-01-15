@echo off
echo.
echo ╔══════════════════════════════════════════════════════════════════════════════╗
echo ║                    PulseMoM Optimized Build System                         ║
echo ║              Advanced PCB Electromagnetic Simulator                        ║
echo ╚══════════════════════════════════════════════════════════════════════════════╝
echo.

REM Check for Visual Studio environment
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: Visual Studio compiler not found.
    echo.
    echo Please run this script from a Visual Studio Developer Command Prompt.
    echo Alternatively, install Visual Studio 2022 with C++ development tools.
    echo.
    echo To open Developer Command Prompt:
    echo   Start Menu -^> Visual Studio 2022 -^> Developer Command Prompt
    exit /b 1
)

REM Parse command line arguments
set BUILD_TYPE=Release
set PLATFORM=x64
set USE_OPENMP=1
set USE_AVX2=1
set CLEAN_BUILD=0
set RUN_TESTS=1
set VERBOSE=0

:parse_args
if "%~1"=="" goto :build
if /i "%~1"=="--debug" set BUILD_TYPE=Debug
if /i "%~1"=="--release" set BUILD_TYPE=Release
if /i "%~1"=="--x86" set PLATFORM=Win32
if /i "%~1"=="--x64" set PLATFORM=x64
if /i "%~1"=="--no-openmp" set USE_OPENMP=0
if /i "%~1"=="--no-avx2" set USE_AVX2=0
if /i "%~1"=="--clean" set CLEAN_BUILD=1
if /i "%~1"=="--no-tests" set RUN_TESTS=0
if /i "%~1"=="--verbose" set VERBOSE=1
if /i "%~1"=="--help" goto :show_help
shift
goto :parse_args

:show_help
echo Usage: %~nx0 [options]
echo.
echo Options:
echo   --debug           Build in Debug mode (default: Release)
echo   --release         Build in Release mode
echo   --x86             Build for 32-bit platform
echo   --x64             Build for 64-bit platform (default)
echo   --no-openmp       Disable OpenMP parallelization
echo   --no-avx2         Disable AVX2 vectorization
echo   --clean           Clean build directory before building
echo   --no-tests        Skip running validation tests
echo   --verbose         Enable verbose output
echo   --help            Show this help message
echo.
echo Examples:
echo   %~nx0                           ^> Build Release x64 with all optimizations
echo   %~nx0 --debug --x86            ^> Build Debug x86
echo   %~nx0 --clean --verbose        ^> Clean build and verbose output
echo   %~nx0 --no-openmp --no-tests   ^> Build without OpenMP and skip tests
exit /b 0

:build
echo Build Configuration:
echo   Build Type: %BUILD_TYPE%
echo   Platform:   %PLATFORM%
echo   OpenMP:     %USE_OPENMP%
echo   AVX2:       %USE_AVX2%
echo   Clean:      %CLEAN_BUILD%
echo   Tests:      %RUN_TESTS%
echo   Verbose:    %VERBOSE%
echo.

REM Create build directory
set BUILD_DIR=build_%PLATFORM%_%BUILD_TYPE%
if %CLEAN_BUILD%==1 (
    echo Cleaning build directory...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
)

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

REM Configure with CMake
echo.
echo Configuring project with CMake...
echo.

set CMAKE_ARGS=-G "Visual Studio 17 2022" -A %PLATFORM%

REM Set vcpkg toolchain
set VCPKG_ROOT=D:\Project\vcpkg-master
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
if exist "%CMAKE_TOOLCHAIN_FILE%" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%"
    echo Using vcpkg toolchain: %CMAKE_TOOLCHAIN_FILE%
) else (
    echo WARNING: vcpkg toolchain not found at %CMAKE_TOOLCHAIN_FILE%
    echo Building without vcpkg integration
)

if %BUILD_TYPE%==Debug (
    set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_BUILD_TYPE=Debug
) else (
    set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_BUILD_TYPE=Release
)

if %USE_OPENMP%==1 (
    set CMAKE_ARGS=%CMAKE_ARGS% -DUSE_OPENMP=ON
) else (
    set CMAKE_ARGS=%CMAKE_ARGS% -DUSE_OPENMP=OFF
)

if %USE_AVX2%==1 (
    set CMAKE_ARGS=%CMAKE_ARGS% -DUSE_AVX2=ON
) else (
    set CMAKE_ARGS=%CMAKE_ARGS% -DUSE_AVX2=OFF
)

if %VERBOSE%==1 (
    echo CMake arguments: %CMAKE_ARGS%
    echo.
)

cmake %CMAKE_ARGS% ..
if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed.
    echo.
    echo Common issues:
    echo   - CMake not installed or not in PATH
    echo   - Visual Studio not properly configured
    echo   - Missing C++ development tools
    echo.
    echo To install CMake: https://cmake.org/download/
    cd ..
    exit /b 1
)

REM Build the project
echo.
echo Building project...
echo Configuration: %BUILD_TYPE% (%PLATFORM%)
echo.

if %VERBOSE%==1 (
    cmake --build . --config %BUILD_TYPE% --parallel --verbose
) else (
    cmake --build . --config %BUILD_TYPE% --parallel
)

if %errorlevel% neq 0 (
    echo.
    echo ERROR: Build failed.
    echo.
    echo Common build issues:
    echo   - Missing dependencies or include files
    echo   - Compiler errors in source code
    echo   - Insufficient memory for compilation
    echo.
    cd ..
    exit /b 1
)

echo.
echo Build completed successfully!
echo.

REM Show build results
set EXECUTABLE_PATH=%BUILD_TYPE%\PulseMoM.exe
if exist %EXECUTABLE_PATH% (
    echo Executable location: %BUILD_DIR%\%EXECUTABLE_PATH%
    echo.
    
    REM Get file size
    for %%I in ("%EXECUTABLE_PATH%") do set FILE_SIZE=%%~zI
    echo Executable size: %FILE_SIZE% bytes
    echo.
    
    REM Run validation tests if requested
    if %RUN_TESTS%==1 (
        echo Running validation tests...
        echo.
        %EXECUTABLE_PATH%
        if %errorlevel% neq 0 (
            echo.
            echo WARNING: Some validation tests failed.
            echo Check the output above for details.
        )
    ) else (
        echo Skipping validation tests (--no-tests specified)
    )
) else (
    echo WARNING: Executable not found at expected location
)

echo.
echo Build Summary:
echo ==============
echo Build Type:    %BUILD_TYPE%
echo Platform:      %PLATFORM%
echo OpenMP:         %USE_OPENMP%
echo AVX2:           %USE_AVX2%
echo Directory:      %BUILD_DIR%
echo Status:         SUCCESS
echo.

REM Create convenience batch files
echo @echo off > run_tests.bat
echo %EXECUTABLE_PATH% >> run_tests.bat
echo pause >> run_tests.bat

echo @echo off > run_optimized.bat
echo %EXECUTABLE_PATH% --benchmark --validation --multiscale --parallel >> run_optimized.bat
echo pause >> run_optimized.bat

echo.
echo Created convenience scripts:
echo   run_tests.bat     - Run validation tests
echo   run_optimized.bat - Run comprehensive analysis
echo.

cd ..
echo Build process completed. Check %BUILD_DIR% for results.
echo.