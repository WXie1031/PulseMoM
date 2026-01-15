@echo off
echo === PEEC + MoM Library Verification Test ===
echo Testing library compilation without CMake...
echo.

setlocal enabledelayedexpansion

:: Set up include paths
set "INCLUDE_PATHS=-I../libs/CGAL-6.1/include"
set "INCLUDE_PATHS=!INCLUDE_PATHS! -I../libs/gmsh-4.15.0-Windows64-sdk/include"
set "INCLUDE_PATHS=!INCLUDE_PATHS! -I../libs/occt-vc14-64/inc"
set "INCLUDE_PATHS=!INCLUDE_PATHS! -I../libs/boost_1_89_0"
set "INCLUDE_PATHS=!INCLUDE_PATHS! -I../libs/Clipper2_1.5.4"
set "INCLUDE_PATHS=!INCLUDE_PATHS! -I../libs/OpenBLAS-0.3.30-x64/include"
set "INCLUDE_PATHS=!INCLUDE_PATHS! -I../libs/petsc-3.24.1/include"
set "INCLUDE_PATHS=!INCLUDE_PATHS! -I../libs/H2Lib-3.0.1"
set "INCLUDE_PATHS=!INCLUDE_PATHS! -I../libs/triangle"

echo Include paths configured:
echo !INCLUDE_PATHS!
echo.

:: Test individual libraries
echo Testing individual library headers...

:: Test 1: CGAL
echo [1/10] Testing CGAL headers...
echo #include ^<CGAL/Simple_cartesian.h^> > test_cgal.cpp
echo int main() { return 0; } >> test_cgal.cpp
g++ !INCLUDE_PATHS! -c test_cgal.cpp -o test_cgal.o 2>nul
if !errorlevel! equ 0 (
    echo  ✓ CGAL headers available
    set CGAL_AVAILABLE=1
) else (
    echo  ✗ CGAL headers missing
    set CGAL_AVAILABLE=0
)

:: Test 2: Gmsh
echo [2/10] Testing Gmsh headers...
echo #include ^<gmsh.h^> > test_gmsh.cpp
echo int main() { return 0; } >> test_gmsh.cpp
g++ !INCLUDE_PATHS! -c test_gmsh.cpp -o test_gmsh.o 2>nul
if !errorlevel! equ 0 (
    echo  ✓ Gmsh headers available
    set GMSH_AVAILABLE=1
) else (
    echo  ✗ Gmsh headers missing
    set GMSH_AVAILABLE=0
)

:: Test 3: OpenCascade
echo [3/10] Testing OpenCascade headers...
echo #include ^<gp_Pnt.hxx^> > test_occt.cpp
echo int main() { return 0; } >> test_occt.cpp
g++ !INCLUDE_PATHS! -c test_occt.cpp -o test_occt.o 2>nul
if !errorlevel! equ 0 (
    echo  ✓ OpenCascade headers available
    set OCCT_AVAILABLE=1
) else (
    echo  ✗ OpenCascade headers missing
    set OCCT_AVAILABLE=0
)

:: Test 4: Boost
echo [4/10] Testing Boost headers...
echo #include ^<boost/geometry.hpp^> > test_boost.cpp
echo int main() { return 0; } >> test_boost.cpp
g++ !INCLUDE_PATHS! -c test_boost.cpp -o test_boost.o 2>nul
if !errorlevel! equ 0 (
    echo  ✓ Boost headers available
    set BOOST_AVAILABLE=1
) else (
    echo  ✗ Boost headers missing
    set BOOST_AVAILABLE=0
)

:: Test 5: Clipper2
echo [5/10] Testing Clipper2 headers...
echo #include ^<clipper2/clipper.h^> > test_clipper.cpp
echo int main() { return 0; } >> test_clipper.cpp
g++ !INCLUDE_PATHS! -c test_clipper.cpp -o test_clipper.o 2>nul
if !errorlevel! equ 0 (
    echo  ✓ Clipper2 headers available
    set CLIPPER_AVAILABLE=1
) else (
    echo  ✗ Clipper2 headers missing
    set CLIPPER_AVAILABLE=0
)

:: Test 6: Triangle
echo [6/10] Testing Triangle headers...
echo #include ^<triangle.h^> > test_triangle.cpp
echo int main() { return 0; } >> test_triangle.cpp
g++ !INCLUDE_PATHS! -c test_triangle.cpp -o test_triangle.o 2>nul
if !errorlevel! equ 0 (
    echo  ✓ Triangle headers available
    set TRIANGLE_AVAILABLE=1
) else (
    echo  ✗ Triangle headers missing
    set TRIANGLE_AVAILABLE=0
)

:: Test 7: OpenBLAS
echo [7/10] Testing OpenBLAS headers...
echo #include ^<cblas.h^> > test_openblas.cpp
echo int main() { return 0; } >> test_openblas.cpp
g++ !INCLUDE_PATHS! -c test_openblas.cpp -o test_openblas.o 2>nul
if !errorlevel! equ 0 (
    echo  ✓ OpenBLAS headers available
    set OPENBLAS_AVAILABLE=1
) else (
    echo  ✗ OpenBLAS headers missing
    set OPENBLAS_AVAILABLE=0
)

:: Test 8: PETSc
echo [8/10] Testing PETSc headers...
echo #include ^<petsc.h^> > test_petsc.cpp
echo int main() { return 0; } >> test_petsc.cpp
g++ !INCLUDE_PATHS! -c test_petsc.cpp -o test_petsc.o 2>nul
if !errorlevel! equ 0 (
    echo  ✓ PETSc headers available
    set PETSC_AVAILABLE=1
) else (
    echo  ✗ PETSc headers missing
    set PETSC_AVAILABLE=0
)

:: Test 9: H2Lib
echo [9/10] Testing H2Lib headers...
echo #include ^<basic.h^> > test_h2lib.cpp
echo int main() { return 0; } >> test_h2lib.cpp
g++ !INCLUDE_PATHS! -c test_h2lib.cpp -o test_h2lib.o 2>nul
if !errorlevel! equ 0 (
    echo  ✓ H2Lib headers available
    set H2LIB_AVAILABLE=1
) else (
    echo  ✗ H2Lib headers missing
    set H2LIB_AVAILABLE=0
)

:: Test 10: Embree
echo [10/10] Testing Embree headers...
echo #include ^<embree4/rtcore.h^> > test_embree.cpp
echo int main() { return 0; } >> test_embree.cpp
g++ !INCLUDE_PATHS! -c test_embree.cpp -o test_embree.o 2>nul
if !errorlevel! equ 0 (
    echo  ✓ Embree headers available
    set EMBREE_AVAILABLE=1
) else (
    echo  ✗ Embree headers missing
    set EMBREE_AVAILABLE=0
)

echo.
echo === SUMMARY ===
echo Library Availability Status:
echo CGAL:       !CGAL_AVAILABLE!
echo Gmsh:       !GMSH_AVAILABLE!
echo OpenCascade: !OCCT_AVAILABLE!
echo Boost:      !BOOST_AVAILABLE!
echo Clipper2:   !CLIPPER_AVAILABLE!
echo Triangle:   !TRIANGLE_AVAILABLE!
echo OpenBLAS:   !OPENBLAS_AVAILABLE!
echo PETSc:      !PETSC_AVAILABLE!
echo H2Lib:      !H2LIB_AVAILABLE!
echo Embree:     !EMBREE_AVAILABLE!

:: Calculate availability
echo.
echo Ready for immediate use:
if !CGAL_AVAILABLE! equ 1 echo  • CGAL - Computational Geometry
echo.
echo Ready for mesh generation:
if !GMSH_AVAILABLE! equ 1 echo  • Gmsh - 3D surface meshing
if !TRIANGLE_AVAILABLE! equ 1 echo  • Triangle - 2D triangulation
if !CLIPPER_AVAILABLE! equ 1 echo  • Clipper2 - 2D polygon operations
echo.
echo Ready for CAD import:
if !OCCT_AVAILABLE! equ 1 echo  • OpenCascade - CAD geometry processing
echo.
echo Ready for numerical computation:
if !BOOST_AVAILABLE! equ 1 echo  • Boost.Geometry - Geometric algorithms
if !OPENBLAS_AVAILABLE! equ 1 echo  • OpenBLAS - Linear algebra
if !PETSC_AVAILABLE! equ 1 echo  • PETSc - Sparse matrix solvers
if !H2LIB_AVAILABLE! equ 1 echo  • H2Lib - H-matrix compression
echo.
echo Ready for visualization:
if !EMBREE_AVAILABLE! equ 1 echo  • Embree - Ray tracing acceleration
echo.

:: Cleanup
del test_*.cpp test_*.o 2>nul

echo Verification complete! Check the summary above for available libraries.
pause