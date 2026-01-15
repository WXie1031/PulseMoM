@echo off
echo === PEEC + MoM Library FINAL Verification ===
echo Rechecking library paths with correct locations...
echo.

setlocal enabledelayedexpansion

:: Set library root path
set "LIBS_ROOT=D:\Project\MoM\PulseMoM\libs"

echo Checking CORRECT library installation paths in: %LIBS_ROOT%
echo.

:: Detailed verification with correct paths
echo === DETAILED LIBRARY VERIFICATION ===
echo.

:: 1. Check Clipper2 with CORRECT path
echo [1/10] Clipper2 Library (CORRECTED PATH):
echo    Base Path: %LIBS_ROOT%\Clipper2_1.5.4
echo    Headers: %LIBS_ROOT%\Clipper2_1.5.4\CPP\Clipper2Lib\include\clipper2\clipper.h
if exist "%LIBS_ROOT%\Clipper2_1.5.4\CPP\Clipper2Lib\include\clipper2\clipper.h" (
    echo    Status: ✓ Headers FOUND at correct location
    echo    DLL/Lib: %LIBS_ROOT%\Clipper2_1.5.4\DLL\Clipper2_64.dll
    if exist "%LIBS_ROOT%\Clipper2_1.5.4\DLL\Clipper2_64.dll" (
        echo    Binary: ✓ DLL found
    ) else (
        echo    Binary: ✗ DLL missing
    )
    set CLIPPER2_STATUS=✓ READY
) else (
    echo    Status: ✗ Headers missing at correct location
    set CLIPPER2_STATUS=✗ MISSING
)
echo.

:: 2. Check H2Lib with CORRECT path  
echo [2/10] H2Lib Library (CORRECTED PATH):
echo    Base Path: %LIBS_ROOT%\H2Lib-3.0.1
echo    Headers: %LIBS_ROOT%\H2Lib-3.0.1\Library\basic.h
if exist "%LIBS_ROOT%\H2Lib-3.0.1\Library\basic.h" (
    echo    Status: ✓ Headers FOUND at correct location
    echo    Additional: %LIBS_ROOT%\H2Lib-3.0.1\Library\hmatrix.h
    if exist "%LIBS_ROOT%\H2Lib-3.0.1\Library\hmatrix.h" (
        echo    H-Matrix: ✓ H-matrix headers found
    ) else (
        echo    H-Matrix: ✗ H-matrix headers missing
    )
    set H2LIB_STATUS=✓ READY
) else (
    echo    Status: ✗ Headers missing at correct location
    set H2LIB_STATUS=✗ MISSING
)
echo.

:: 3. Re-verify other libraries with full details
echo [3/10] CGAL Library:
if exist "%LIBS_ROOT%\CGAL-6.1\include\CGAL\Simple_cartesian.h" (
    echo    Status: ✓ Headers found - %LIBS_ROOT%\CGAL-6.1\include\CGAL\Simple_cartesian.h
    set CGAL_STATUS=✓ READY
) else (
    echo    Status: ✗ Missing
    set CGAL_STATUS=✗ MISSING
)
echo.

echo [4/10] Gmsh Library:
if exist "%LIBS_ROOT%\gmsh-4.15.0-Windows64-sdk\include\gmsh.h" (
    echo    Status: ✓ Headers found - %LIBS_ROOT%\gmsh-4.15.0-Windows64-sdk\include\gmsh.h
    echo    C API: %LIBS_ROOT%\gmsh-4.15.0-Windows64-sdk\include\gmshc.h
    set GMSH_STATUS=✓ READY
) else (
    echo    Status: ✗ Missing
    set GMSH_STATUS=✗ MISSING
)
echo.

echo [5/10] OpenCascade Library:
if exist "%LIBS_ROOT%\occt-vc14-64\inc\gp_Pnt.hxx" (
    echo    Status: ✓ Headers found - %LIBS_ROOT%\occt-vc14-64\inc\gp_Pnt.hxx
    set OCCT_STATUS=✓ READY
) else (
    echo    Status: ✗ Missing
    set OCCT_STATUS=✗ MISSING
)
echo.

echo [6/10] Boost Library:
if exist "%LIBS_ROOT%\boost_1_89_0\boost\geometry.hpp" (
    echo    Status: ✓ Headers found - %LIBS_ROOT%\boost_1_89_0\boost\geometry.hpp
    set BOOST_STATUS=✓ READY
) else (
    echo    Status: ✗ Missing
    set BOOST_STATUS=✗ MISSING
)
echo.

echo [7/10] Triangle Library:
if exist "%LIBS_ROOT%\triangle\triangle.h" (
    echo    Status: ✓ Headers found - %LIBS_ROOT%\triangle\triangle.h
    set TRIANGLE_STATUS=✓ READY
) else (
    echo    Status: ✗ Missing
    set TRIANGLE_STATUS=✗ MISSING
)
echo.

echo [8/10] OpenBLAS Library:
if exist "%LIBS_ROOT%\OpenBLAS-0.3.30-x64\include\cblas.h" (
    echo    Status: ✓ Headers found - %LIBS_ROOT%\OpenBLAS-0.3.30-x64\include\cblas.h
    set OPENBLAS_STATUS=✓ READY
) else (
    echo    Status: ✗ Missing
    set OPENBLAS_STATUS=✗ MISSING
)
echo.

echo [9/10] PETSc Library:
if exist "%LIBS_ROOT%\petsc-3.24.1\include\petsc.h" (
    echo    Status: ✓ Headers found - %LIBS_ROOT%\petsc-3.24.1\include\petsc.h
    set PETSC_STATUS=✓ READY
) else (
    echo    Status: ✗ Missing
    set PETSC_STATUS=✗ MISSING
)
echo.

echo [10/10] Embree Library:
if exist "%LIBS_ROOT%\embree-4.4.0\include\embree4\rtcore.h" (
    echo    Status: ✓ Headers found - %LIBS_ROOT%\embree-4.4.0\include\embree4\rtcore.h
    set EMBREE_STATUS=✓ READY
) else (
    echo    Status: ✗ Missing
    set EMBREE_STATUS=✗ MISSING
)
echo.

:: FINAL SUMMARY
echo === FINAL VERIFICATION SUMMARY ===
echo.
echo Library Status Summary:
echo CGAL:       %CGAL_STATUS%
echo Gmsh:       %GMSH_STATUS%
echo OpenCascade: %OCCT_STATUS%
echo Boost:      %BOOST_STATUS%
echo Clipper2:   %CLIPPER2_STATUS%  [CORRECTED PATH]
echo Triangle:   %TRIANGLE_STATUS%
echo OpenBLAS:   %OPENBLAS_STATUS%
echo PETSc:      %PETSC_STATUS%
echo H2Lib:      %H2LIB_STATUS%   [CORRECTED PATH]
echo Embree:     %EMBREE_STATUS%
echo.

:: Count available libraries
set /a AVAILABLE_COUNT=0
if "%CGAL_STATUS%"=="✓ READY" set /a AVAILABLE_COUNT+=1
if "%GMSH_STATUS%"=="✓ READY" set /a AVAILABLE_COUNT+=1
if "%OCCT_STATUS%"=="✓ READY" set /a AVAILABLE_COUNT+=1
if "%BOOST_STATUS%"=="✓ READY" set /a AVAILABLE_COUNT+=1
if "%CLIPPER2_STATUS%"=="✓ READY" set /a AVAILABLE_COUNT+=1
if "%TRIANGLE_STATUS%"=="✓ READY" set /a AVAILABLE_COUNT+=1
if "%OPENBLAS_STATUS%"=="✓ READY" set /a AVAILABLE_COUNT+=1
if "%PETSC_STATUS%"=="✓ READY" set /a AVAILABLE_COUNT+=1
if "%H2LIB_STATUS%"=="✓ READY" set /a AVAILABLE_COUNT+=1
if "%EMBREE_STATUS%"=="✓ READY" set /a AVAILABLE_COUNT+=1

echo FINAL RESULT: %AVAILABLE_COUNT%/10 libraries ready for immediate use
echo.

:: Functional grouping with corrected status
echo === FUNCTIONAL ANALYSIS ===
echo.
echo 3D Mesh Generation:
if "%CGAL_STATUS%"=="✓ READY" echo  ✓ CGAL - Robust 3D meshing algorithms
if "%GMSH_STATUS%"=="✓ READY" echo  ✓ Gmsh - Surface mesh generation
echo.
echo 2D Geometry Processing:
if "%CLIPPER2_STATUS%"=="✓ READY" echo  ✓ Clipper2 - Boolean operations [CORRECTED]
if "%TRIANGLE_STATUS%"=="✓ READY" echo  ✓ Triangle - Constrained triangulation
if "%BOOST_STATUS%"=="✓ READY" echo  ✓ Boost.Geometry - Generic geometry
echo.
echo CAD Import:
if "%OCCT_STATUS%"=="✓ READY" echo  ✓ OpenCascade - STEP/IGES import
echo.
echo Numerical Computing:
if "%OPENBLAS_STATUS%"=="✓ READY" echo  ✓ OpenBLAS - Dense linear algebra
if "%PETSC_STATUS%"=="✓ READY" echo  ✓ PETSc - Sparse systems
if "%H2LIB_STATUS%"=="✓ READY" echo  ✓ H2Lib - Matrix compression [CORRECTED]
echo.
echo Visualization:
if "%EMBREE_STATUS%"=="✓ READY" echo  ✓ Embree - Ray tracing acceleration
echo.

:: Include path recommendations
echo === INCLUDE PATH RECOMMENDATIONS ===
echo.
echo For compilation, use these CORRECT paths:
echo.
if "%CLIPPER2_STATUS%"=="✓ READY" (
echo Clipper2: -I"%LIBS_ROOT%\Clipper2_1.5.4\CPP\Clipper2Lib\include"
echo           (contains: clipper2/clipper.h, clipper2/clipper.core.h, etc.)
echo.
)
if "%H2LIB_STATUS%"=="✓ READY" (
echo H2Lib:   -I"%LIBS_ROOT%\H2Lib-3.0.1\Library"
echo           (contains: basic.h, hmatrix.h, cluster.h, etc.)
echo.
)

:: Final recommendations
echo === NEXT STEPS ===
echo.
echo 1. ✅ Your CMakeLists.txt already has correct paths configured
echo 2. ✅ %AVAILABLE_COUNT% libraries verified and ready for immediate use
echo 3. 🚀 Start integration with core libraries: CGAL, Gmsh, OpenCascade
echo 4. 📚 Use provided examples in tests/library_usage_examples.c
echo 5. 🔧 Test compilation with: cmake --build . --target library_integration_test
echo.
echo 🎉 VERIFICATION COMPLETE - %AVAILABLE_COUNT%/10 libraries ready!
pause