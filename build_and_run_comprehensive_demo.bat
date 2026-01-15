@echo off
REM build_and_run_comprehensive_demo.bat
REM 综合网格生成平台演示构建脚本

echo 🎯 构建综合网格生成平台演示程序
echo ═══════════════════════════════════════════════════════════

REM 检查编译器
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo ❌ 未找到MSVC编译器，请使用Visual Studio开发者命令提示符
    exit /b 1
)

echo 🔍 检查库文件...

REM 检查各个库文件
set "missing_libs="

if not exist "libs\CGAL-6.1\include\CGAL\basic.h" (
    set missing_libs=%missing_libs% CGAL
)

if not exist "libs\gmsh-4.15.0-Windows64-sdk\include\gmsh.h" (
    set missing_libs=%missing_libs% Gmsh
)

if not exist "libs\occt-vc14-64\inc\STEPControl_StepModelType.hxx" (
    set missing_libs=%missing_libs% OpenCascade
)

if not exist "libs\Clipper2_1.5.4\CPP\Clipper2Lib\include\clipper2\clipper.h" (
    set missing_libs=%missing_libs% Clipper2
)

if not exist "libs\triangle\triangle.h" (
    set missing_libs=%missing_libs% Triangle
)

if defined missing_libs (
    echo ⚠️  缺少以下库: %missing_libs%
    echo 请确认所有库已正确安装
)

echo 🔨 开始编译综合演示程序...

REM 设置包含路径
set "INCLUDE_PATHS=/I. /Isrc /Isrc\mesh"
set "INCLUDE_PATHS=%INCLUDE_PATHS% /Ilibs\CGAL-6.1\include"
set "INCLUDE_PATHS=%INCLUDE_PATHS% /Ilibs\gmsh-4.15.0-Windows64-sdk\include"
set "INCLUDE_PATHS=%INCLUDE_PATHS% /Ilibs\occt-vc14-64\inc"
set "INCLUDE_PATHS=%INCLUDE_PATHS% /Ilibs\Clipper2_1.5.4\CPP\Clipper2Lib\include"
set "INCLUDE_PATHS=%INCLUDE_PATHS% /Ilibs\boost_1_89_0"
set "INCLUDE_PATHS=%INCLUDE_PATHS% /Ilibs\triangle"

REM 设置库路径
set "LIB_PATHS=/LIBPATH:libs\CGAL-6.1\lib /LIBPATH:libs\gmsh-4.15.0-Windows64-sdk\lib"
set "LIB_PATHS=%LIB_PATHS% /LIBPATH:libs\occt-vc14-64\win64\lib"
set "LIB_PATHS=%LIB_PATHS% /LIBPATH:libs\Clipper2_1.5.4\DLL"

REM 编译选项
set "COMPILE_OPTS=/std:c++17 /O2 /MD /EHsc /DNDEBUG"
set "COMPILE_OPTS=%COMPILE_OPTS% /DCGAL_ENABLED /DGMSH_ENABLED /DOPENCASCADE_ENABLED"
set "COMPILE_OPTS=%COMPILE_OPTS% /DCLIPPER2_ENABLED /DTRIANGLE_ENABLED"

REM 链接库
set "LINK_LIBS=cgal-vc140-mt-6.1.lib gmsh.lib TKSTEP.lib TKIGES.lib"
set "LINK_LIBS=%LINK_LIBS% clipper2.lib kernel32.lib user32.lib"

REM 编译命令
echo 📋 编译命令:
echo cl %COMPILE_OPTS% %INCLUDE_PATHS% comprehensive_mesh_demo.cpp ^
     src\mesh\cgal_surface_mesh.cpp ^
     src\mesh\gmsh_surface_mesh.cpp ^
     src\mesh\opencascade_cad_import.cpp ^
     src\mesh\clipper2_triangle_2d.cpp ^
     /Fe:comprehensive_mesh_demo.exe /link %LIB_PATHS% %LINK_LIBS%

cl %COMPILE_OPTS% %INCLUDE_PATHS% comprehensive_mesh_demo.cpp ^
   src\mesh\cgal_surface_mesh.cpp ^
   src\mesh\gmsh_surface_mesh.cpp ^
   src\mesh\opencascade_cad_import.cpp ^
   src\mesh\clipper2_triangle_2d.cpp ^
   /Fe:comprehensive_mesh_demo.exe /link %LIB_PATHS% %LINK_LIBS%

if %errorlevel% neq 0 (
    echo ❌ 编译失败！
    exit /b 1
)

echo ✅ 编译成功！
echo.

REM 运行演示程序
echo 🚀 运行综合网格生成平台演示...
echo ═══════════════════════════════════════════════════════════
comprehensive_mesh_demo.exe

echo.
echo 🎯 演示完成！
echo.
echo 📁 生成的文件:
echo   - comprehensive_mesh_demo.exe (演示程序)
echo   - *.obj, *.pdb (编译中间文件)
echo.
echo 📊 查看详细报告:
echo   - COMPREHENSIVE_MESH_PLATFORM_INTEGRATION_REPORT.md
echo   - *_INTEGRATION_REPORT.md (各库详细报告)
echo.

REM 清理可选
set /p cleanup=是否清理中间文件? (y/n): 
if /i "%cleanup%"=="y" (
    echo 🧹 清理中间文件...
    del *.obj *.pdb 2>nul
    echo ✅ 清理完成
)

echo 🎉 综合网格生成平台集成验证完成！
pause