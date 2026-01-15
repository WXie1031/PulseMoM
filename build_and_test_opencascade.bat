@echo off
REM OpenCascade CAD Import Integration Build Script
REM This script builds and tests the OpenCascade CAD import functionality

echo =========================================
echo OpenCascade CAD Import Build and Test
echo =========================================
echo.

REM Set up environment
setlocal enabledelayedexpansion

REM Set OpenCascade paths
set OCCT_ROOT=..\libs\occt-vc14-64
set OCCT_INC=%OCCT_ROOT%\inc
set OCCT_LIB=%OCCT_ROOT%\win64\vc14\lib

REM Set compiler and flags
set CXX=g++
set CXXFLAGS=-std=c++11 -O2 -Wall -DWIN32 -D_WINDOWS
set INCLUDES=-I%OCCT_INC% -I..\src\mesh
set LIBRARIES=-L%OCCT_LIB% -lTKernel -lTKMath -lTKG2d -lTKG3d -lTKGeomBase -lTKBRep -lTKGeomAlgo -lTKTopAlgo -lTKHLR -lTKShHealing -lTKMesh -lTKService -lTKDE -lTKXSBase -lTKDEIGES -lTKDESTEP -lTKDESTL -lTKRWMesh

echo Setting up OpenCascade environment...
echo OCCT_ROOT: %OCCT_ROOT%
echo OCCT_INC: %OCCT_INC%
echo OCCT_LIB: %OCCT_LIB%
echo.

REM Check if OpenCascade libraries exist
if not exist "%OCCT_LIB%\TKKernel.lib" (
    echo ERROR: OpenCascade libraries not found at %OCCT_LIB%
    echo Please ensure OpenCascade is properly installed.
    exit /b 1
)

echo OpenCascade libraries found.
echo.

REM Create build directory
if not exist "build_opencascade" mkdir build_opencascade
cd build_opencascade

echo Building OpenCascade CAD import library...
echo.

REM Compile the OpenCascade CAD import implementation
%CXX% %CXXFLAGS% %INCLUDES% -c ..\..\src\mesh\opencascade_cad_import.cpp -o opencascade_cad_import.o

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to compile OpenCascade CAD import implementation
    cd ..
    exit /b 1
)

echo OpenCascade CAD import implementation compiled successfully.
echo.

echo Building test suite...
echo.

REM Compile the test suite
%CXX% %CXXFLAGS% %INCLUDES% -c ..\..\tests\test_opencascade_cad_import.cpp -o test_opencascade_cad_import.o

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to compile test suite
    cd ..
    exit /b 1
)

echo Test suite compiled successfully.
echo.

echo Linking test executable...
echo.

REM Link the test executable
%CXX% %CXXFLAGS% opencascade_cad_import.o test_opencascade_cad_import.o %LIBRARIES% -o test_opencascade_cad_import.exe

if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to link test executable
    cd ..
    exit /b 1
)

echo Test executable linked successfully.
echo.

echo Running OpenCascade CAD import tests...
echo.

REM Run the test suite
.\test_opencascade_cad_import.exe

set TEST_RESULT=%ERRORLEVEL%

echo.
echo =========================================

if %TEST_RESULT% equ 0 (
    echo OpenCascade CAD import tests PASSED
) else (
    echo OpenCascade CAD import tests FAILED with error code %TEST_RESULT%
)

echo =========================================
echo.

REM Return to original directory
cd ..

REM Exit with test result
exit /b %TEST_RESULT%