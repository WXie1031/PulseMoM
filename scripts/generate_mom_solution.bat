@echo off
REM Generate MoM Solver Visual Studio Solution
REM This script creates a standalone solution for MoM solver only

echo ========================================
echo Generating MoM Solver Solution
echo ========================================
echo.

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
cd /d %PROJECT_ROOT%

REM Create build directory for MoM
if not exist build_mom (
    echo Creating build_mom directory...
    mkdir build_mom
)

cd build_mom

echo.
echo Running CMake to generate Visual Studio solution...
echo.

REM Generate solution using CMake
REM Configure to only build MoM executable
cmake -G "Visual Studio 17 2022" -A x64 ^
    -DBUILD_STANDALONE_EXECUTABLES=ON ^
    -DBUILD_MOM_EXE=ON ^
    -DBUILD_PEEC_EXE=OFF ^
    ..

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo MoM Solver solution generated successfully!
    echo ========================================
    echo.
    echo Solution file: %CD%\MoM_Solver.sln
    echo.
    echo You can now:
    echo   1. Open MoM_Solver.sln in Visual Studio
    echo   2. Build the solution (Ctrl+Shift+B)
    echo   3. Run mom_solver_exe.exe
    echo.
) else (
    echo.
    echo ========================================
    echo ERROR: Failed to generate solution
    echo ========================================
    echo.
    echo Please check:
    echo   1. CMake is installed and in PATH
    echo   2. Visual Studio 2022 is installed
    echo   3. CMakeLists_MoM_Solver.txt exists
    echo.
    exit /b 1
)

cd ..

pause
