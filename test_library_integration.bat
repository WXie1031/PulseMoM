@echo off
echo ================================================================================
echo 🏗️  PEEC-MoM 网格生成库集成测试
echo ================================================================================
echo.

REM Test library availability
echo 📊 正在测试库可用性...
echo.

REM Check each library directory
echo [1/9] 检查 CGAL 库...
if exist "libs\CGAL-6.1\include\CGAL\version.h" (
    echo    ✅ CGAL 6.1 找到
) else (
    echo    ❌ CGAL 未找到
)

echo [2/9] 检查 Gmsh 库...
if exist "libs\gmsh-4.15.0-Windows64-sdk\include\gmshc.h" (
    echo    ✅ Gmsh 4.15.0 找到
) else (
    echo    ❌ Gmsh 未找到
)

echo [3/9] 检查 OpenCascade 库...
if exist "libs\occt-vc14-64\inc\STEPControl_Reader.hxx" (
    echo    ✅ OpenCascade 7.9.2 找到
) else (
    echo    ❌ OpenCascade 未找到
)

echo [4/9] 检查 Clipper2 库...
if exist "libs\Clipper2_1.5.4\CPP\Clipper2Lib\include\clipper2\clipper.h" (
    echo    ✅ Clipper2 1.5.4 找到
) else (
    echo    ❌ Clipper2 未找到
)

echo [5/9] 检查 Triangle 库...
if exist "libs\triangle\triangle.h" (
    echo    ✅ Triangle 找到
) else (
    echo    ❌ Triangle 未找到
)

echo [6/9] 检查 Boost 库...
if exist "libs\boost_1_89_0\boost\geometry.hpp" (
    echo    ✅ Boost 1.89.0 找到
) else (
    echo    ❌ Boost 未找到
)

echo [7/9] 检查 OpenBLAS 库...
if exist "libs\OpenBLAS-0.3.30-x64\include\openblas_config.h" (
    echo    ✅ OpenBLAS 0.3.30 找到
) else (
    echo    ❌ OpenBLAS 未找到
)

echo [8/9] 检查 PETSc 库...
if exist "libs\petsc-3.24.1\include\petsc.h" (
    echo    ✅ PETSc 3.24.1 找到
) else (
    echo    ❌ PETSc 未找到
)

echo [9/9] 检查 H2Lib 库...
if exist "libs\H2Lib-3.0.1\Library\h2lib.h" (
    echo    ✅ H2Lib 3.0.1 找到
) else (
    echo    ❌ H2Lib 未找到
)

echo.
echo ================================================================================
echo 🎯 库集成测试完成！
echo ================================================================================
echo.
echo 💡 下一步建议：
echo    • 运行 CMake 配置测试（需要安装 CMake）
echo    • 编译库集成测试程序
echo    • 开始实现 CAD 导入和表面网格生成
echo.

pause