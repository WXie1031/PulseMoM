@echo off
echo === PEEC + MoM Library File Verification ===
echo Checking actual library file locations and accessibility...
echo.

setlocal enabledelayedexpansion

:: Set library root path
set "LIBS_ROOT=D:\Project\MoM\PulseMoM\libs"

echo Checking library installations in: %LIBS_ROOT%
echo.

:: Check each library installation
set TOTAL_LIBS=0
set AVAILABLE_LIBS=0

echo === LIBRARY INSTALLATION STATUS ===
echo.

:: 1. Check CGAL
echo [1/10] CGAL Library:
echo    Path: %LIBS_ROOT%\CGAL-6.1
if exist "%LIBS_ROOT%\CGAL-6.1\include\CGAL\Simple_cartesian.h" (
    echo    Status: ✓ Headers found
    echo    Version: 6.1
    set /a AVAILABLE_LIBS+=1
) else (
    echo    Status: ✗ Headers missing
    echo    Expected: %LIBS_ROOT%\CGAL-6.1\include\CGAL\Simple_cartesian.h
)
set /a TOTAL_LIBS+=1
echo.

:: 2. Check Gmsh
echo [2/10] Gmsh Library:
echo    Path: %LIBS_ROOT%\gmsh-4.15.0-Windows64-sdk
if exist "%LIBS_ROOT%\gmsh-4.15.0-Windows64-sdk\include\gmsh.h" (
    echo    Status: ✓ Headers found
    echo    Version: 4.15.0
    set /a AVAILABLE_LIBS+=1
) else (
    echo    Status: ✗ Headers missing
    echo    Expected: %LIBS_ROOT%\gmsh-4.15.0-Windows64-sdk\include\gmsh.h
)
set /a TOTAL_LIBS+=1
echo.

:: 3. Check OpenCascade
echo [3/10] OpenCascade Library:
echo    Path: %LIBS_ROOT%\occt-vc14-64
if exist "%LIBS_ROOT%\occt-vc14-64\inc\gp_Pnt.hxx" (
    echo    Status: ✓ Headers found
    echo    Version: 7.8.0
    set /a AVAILABLE_LIBS+=1
) else (
    echo    Status: ✗ Headers missing
    echo    Expected: %LIBS_ROOT%\occt-vc14-64\inc\gp_Pnt.hxx
)
set /a TOTAL_LIBS+=1
echo.

:: 4. Check Boost
echo [4/10] Boost Library:
echo    Path: %LIBS_ROOT%\boost_1_89_0
if exist "%LIBS_ROOT%\boost_1_89_0\boost\geometry.hpp" (
    echo    Status: ✓ Headers found
    echo    Version: 1.89.0
    set /a AVAILABLE_LIBS+=1
) else (
    echo    Status: ✗ Headers missing
    echo    Expected: %LIBS_ROOT%\boost_1_89_0\boost\geometry.hpp
)
set /a TOTAL_LIBS+=1
echo.

:: 5. Check Clipper2
echo [5/10] Clipper2 Library:
echo    Path: %LIBS_ROOT%\Clipper2_1.5.4
if exist "%LIBS_ROOT%\Clipper2_1.5.4\*.h" (
    echo    Status: ✓ Headers found
    echo    Version: 1.5.4
    set /a AVAILABLE_LIBS+=1
) else (
    echo    Status: ✗ Headers missing
    echo    Expected: %LIBS_ROOT%\Clipper2_1.5.4\*.h files
)
set /a TOTAL_LIBS+=1
echo.

:: 6. Check Triangle
echo [6/10] Triangle Library:
echo    Path: %LIBS_ROOT%\triangle
if exist "%LIBS_ROOT%\triangle\triangle.h" (
    echo    Status: ✓ Headers found
    echo    Version: 1.6
    set /a AVAILABLE_LIBS+=1
) else (
    echo    Status: ✗ Headers missing
    echo    Expected: %LIBS_ROOT%\triangle\triangle.h
)
set /a TOTAL_LIBS+=1
echo.

:: 7. Check OpenBLAS
echo [7/10] OpenBLAS Library:
echo    Path: %LIBS_ROOT%\OpenBLAS-0.3.30-x64
if exist "%LIBS_ROOT%\OpenBLAS-0.3.30-x64\include\cblas.h" (
    echo    Status: ✓ Headers found
    echo    Version: 0.3.30
    set /a AVAILABLE_LIBS+=1
) else (
    echo    Status: ✗ Headers missing
    echo    Expected: %LIBS_ROOT%\OpenBLAS-0.3.30-x64\include\cblas.h
)
set /a TOTAL_LIBS+=1
echo.

:: 8. Check PETSc
echo [8/10] PETSc Library:
echo    Path: %LIBS_ROOT%\petsc-3.24.1
if exist "%LIBS_ROOT%\petsc-3.24.1\include\petsc.h" (
    echo    Status: ✓ Headers found
    echo    Version: 3.24.1
    set /a AVAILABLE_LIBS+=1
) else (
    echo    Status: ✗ Headers missing
    echo    Expected: %LIBS_ROOT%\petsc-3.24.1\include\petsc.h
)
set /a TOTAL_LIBS+=1
echo.

:: 9. Check H2Lib
echo [9/10] H2Lib Library:
echo    Path: %LIBS_ROOT%\H2Lib-3.0.1
if exist "%LIBS_ROOT%\H2Lib-3.0.1\basic.h" (
    echo    Status: ✓ Headers found
    echo    Version: 3.0.1
    set /a AVAILABLE_LIBS+=1
) else (
    echo    Status: ✗ Headers missing
    echo    Expected: %LIBS_ROOT%\H2Lib-3.0.1\basic.h
)
set /a TOTAL_LIBS+=1
echo.

:: 10. Check Embree
echo [10/10] Embree Library:
echo    Path: %LIBS_ROOT%\embree-4.4.0
if exist "%LIBS_ROOT%\embree-4.4.0\include\embree4\rtcore.h" (
    echo    Status: ✓ Headers found
    echo    Version: 4.4.0
    set /a AVAILABLE_LIBS+=1
) else (
    echo    Status: ✗ Headers missing
    echo    Expected: %LIBS_ROOT%\embree-4.4.0\include\embree4\rtcore.h
)
set /a TOTAL_LIBS+=1
echo.

:: Summary
echo === VERIFICATION SUMMARY ===
echo.
echo Total Libraries Checked: %TOTAL_LIBS%
echo Available Libraries: %AVAILABLE_LIBS%
echo Availability Rate: %AVAILABLE_LIBS%/%TOTAL_LIBS% (available/total)
echo.

:: Detailed recommendations
echo === LIBRARY USAGE RECOMMENDATIONS ===
echo.
echo IMMEDIATE USAGE (Headers Available):
if exist "%LIBS_ROOT%\CGAL-6.1\include\CGAL\Simple_cartesian.h" echo  • CGAL - Computational geometry algorithms
if exist "%LIBS_ROOT%\gmsh-4.15.0-Windows64-sdk\include\gmsh.h" echo  • Gmsh - 3D surface mesh generation
if exist "%LIBS_ROOT%\occt-vc14-64\inc\gp_Pnt.hxx" echo  • OpenCascade - CAD geometry import
if exist "%LIBS_ROOT%\boost_1_89_0\boost\geometry.hpp" echo  • Boost.Geometry - Geometric algorithms
if exist "%LIBS_ROOT%\Clipper2_1.5.4\*.h" echo  • Clipper2 - 2D polygon operations
if exist "%LIBS_ROOT%\triangle\triangle.h" echo  • Triangle - 2D triangulation
if exist "%LIBS_ROOT%\OpenBLAS-0.3.30-x64\include\cblas.h" echo  • OpenBLAS - Linear algebra kernels
if exist "%LIBS_ROOT%\petsc-3.24.1\include\petsc.h" echo  • PETSc - Sparse matrix solvers
if exist "%LIBS_ROOT%\H2Lib-3.0.1\basic.h" echo  • H2Lib - H-matrix compression
if exist "%LIBS_ROOT%\embree-4.4.0\include\embree4\rtcore.h" echo  • Embree - Ray tracing acceleration
echo.

echo FUNCTIONAL GROUPINGS:
echo.
echo 3D Mesh Generation:
if exist "%LIBS_ROOT%\CGAL-6.1\include\CGAL\Simple_cartesian.h" echo  ✓ CGAL - Robust 3D meshing
if exist "%LIBS_ROOT%\gmsh-4.15.0-Windows64-sdk\include\gmsh.h" echo  ✓ Gmsh - Surface mesh generation
echo.
echo 2D Geometry Processing:
if exist "%LIBS_ROOT%\Clipper2_1.5.4\*.h" echo  ✓ Clipper2 - Boolean operations
if exist "%LIBS_ROOT%\triangle\triangle.h" echo  ✓ Triangle - Constrained triangulation
if exist "%LIBS_ROOT%\boost_1_89_0\boost\geometry.hpp" echo  ✓ Boost.Geometry - Generic geometry
echo.
echo CAD Import:
if exist "%LIBS_ROOT%\occt-vc14-64\inc\gp_Pnt.hxx" echo  ✓ OpenCascade - STEP/IGES import
echo.
echo Numerical Computing:
if exist "%LIBS_ROOT%\OpenBLAS-0.3.30-x64\include\cblas.h" echo  ✓ OpenBLAS - Dense linear algebra
if exist "%LIBS_ROOT%\petsc-3.24.1\include\petsc.h" echo  ✓ PETSc - Sparse systems
if exist "%LIBS_ROOT%\H2Lib-3.0.1\basic.h" echo  ✓ H2Lib - Matrix compression
echo.
echo NEXT STEPS:
echo 1. Update CMakeLists.txt with verified library paths
echo 2. Test compilation with actual library linking
echo 3. Create unified include directory if needed
echo 4. Implement library-specific mesh generation modules
echo.
pause