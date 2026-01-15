@echo off
REM Build and Test Gmsh 3D Surface Mesh Integration
echo Building Gmsh Surface Mesh Test...

REM Set up environment
set GMSH_DIR=D:\Project\MoM\PulseMoM\libs\gmsh-4.15.0-Windows64-sdk
set INCLUDE_DIRS=-I..\src -I%GMSH_DIR%\include
set LIB_DIRS=-L%GMSH_DIR%\lib

REM Compile the test
echo Compiling Gmsh surface mesh implementation...
g++ -std=c++14 %INCLUDE_DIRS% -c ..\src\mesh\gmsh_surface_mesh.cpp -o gmsh_surface_mesh.o
if errorlevel 1 goto :error

echo Compiling test suite...
g++ -std=c++14 %INCLUDE_DIRS% tests\test_gmsh_surface_mesh.cpp gmsh_surface_mesh.o -o test_gmsh_surface_mesh %LIB_DIRS% -lgmsh -lstdc++
if errorlevel 1 goto :error

echo Build successful! Running tests...
echo.

REM Run the test
test_gmsh_surface_mesh.exe
goto :end

:error
echo Build failed!
echo.
echo Troubleshooting tips:
echo 1. Check that Gmsh is properly installed at: %GMSH_DIR%
echo 2. Verify that gmsh.h_cwrap is available in the include directory
echo 3. Check that gmsh.dll is available in the lib directory
echo 4. Ensure Visual C++ runtime libraries are available
exit /b 1

:end
echo.
echo Test completed.