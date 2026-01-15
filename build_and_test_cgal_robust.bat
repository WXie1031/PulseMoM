@echo off
REM Build and Test CGAL Robust 2D Implementation
echo Building CGAL Robust 2D Test...

REM Set up environment
set CGAL_DIR=D:\Project\MoM\PulseMoM\libs\CGAL-6.1
set BOOST_DIR=D:\Project\MoM\PulseMoM\libs\boost_1_89_0
set INCLUDE_DIRS=-I..\src -I%CGAL_DIR%\include -I%BOOST_DIR%
set LIB_DIRS=-L%CGAL_DIR%\lib -L%BOOST_DIR%\stage\lib

REM Compile the test
g++ -std=c++14 %INCLUDE_DIRS% -c ..\src\mesh\cgal_surface_mesh.cpp -o cgal_surface_mesh.o
if errorlevel 1 goto :error

g++ -std=c++14 %INCLUDE_DIRS% -c ..\src\mesh\mesh_mesh.c -o mesh_mesh.o
if errorlevel 1 goto :error

g++ -std=c++14 %INCLUDE_DIRS% -c ..\src\geometry\geom_geometry.c -o geom_geometry.o
if errorlevel 1 goto :error

g++ -std=c++14 %INCLUDE_DIRS% tests\test_cgal_robust_2d.cpp cgal_surface_mesh.o mesh_mesh.o geom_geometry.o -o test_cgal_robust_2d %LIB_DIRS% -lCGAL -lgmp -lmpfr
if errorlevel 1 goto :error

echo Build successful! Running tests...
echo.

REM Run the test
test_cgal_robust_2d.exe
goto :end

:error
echo Build failed!
exit /b 1

:end
echo.
echo Test completed.