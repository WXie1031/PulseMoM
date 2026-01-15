@echo off
REM Visual Studio项目生成脚本
REM 用于快速生成Visual Studio解决方案文件

setlocal enabledelayedexpansion

echo ========================================
echo PulseMoM Visual Studio Project Generator
echo ========================================
echo.

REM 检查CMake是否可用
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found in PATH!
    echo Please install CMake and add it to PATH.
    pause
    exit /b 1
)

REM 创建build目录
if not exist build (
    echo Creating build directory...
    mkdir build
) else (
    echo Build directory already exists.
)

cd build

REM 检测Visual Studio版本
echo Detecting Visual Studio installation...
if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_GENERATOR=Visual Studio 17 2022
    echo Found Visual Studio 2022
) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_GENERATOR=Visual Studio 17 2022
    echo Found Visual Studio 2022 Professional
) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_GENERATOR=Visual Studio 17 2022
    echo Found Visual Studio 2022 Enterprise
) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set VS_GENERATOR=Visual Studio 16 2019
    echo Found Visual Studio 2019
) else (
    set VS_GENERATOR=Visual Studio 17 2022
    echo Using default: Visual Studio 17 2022
    echo (If this fails, please specify the generator manually)
)

echo.
echo Generating Visual Studio project files...
echo Generator: %VS_GENERATOR%
echo.

cmake .. -G "%VS_GENERATOR%" -A x64

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo SUCCESS: Project files generated!
    echo ========================================
    echo.
    echo Solution file: build\PEEC_MoM_Unified_Framework.sln
    echo.
    set /p OPEN="Open in Visual Studio? (Y/N): "
    if /i "!OPEN!"=="Y" (
        echo Opening Visual Studio...
        start PEEC_MoM_Unified_Framework.sln
    )
) else (
    echo.
    echo ========================================
    echo ERROR: Failed to generate project files!
    echo ========================================
    echo.
    echo Please check:
    echo 1. CMake is installed and in PATH
    echo 2. Visual Studio is installed
    echo 3. Required dependencies are available
    echo 4. Check the error messages above
    echo.
)

cd ..
pause

