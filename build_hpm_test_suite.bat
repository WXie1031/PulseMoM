@echo off
REM build_hpm_test_suite.bat
REM HPM算法测试套件构建脚本

echo 🛰️ 构建卫星高功率微波激励测试套件
echo ═══════════════════════════════════════════════════════════

REM 检查编译器
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo ❌ 未找到MSVC编译器，请使用Visual Studio开发者命令提示符
    exit /b 1
)

REM 设置路径
set "SOURCE_DIR=tests\test_hpm"
set "OUTPUT_DIR=tests\test_hpm\build"

REM 创建输出目录
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo 🔧 设置编译选项...

REM 基本编译选项
set "COMPILE_OPTS=/std:c++17 /O2 /MD /EHsc /DNDEBUG"
set "COMPILE_OPTS=%COMPILE_OPTS% /I. /Isrc /Isrc\mesh"

REM 包含数学库
set "COMPILE_OPTS=%COMPILE_OPTS% /D_USE_MATH_DEFINES"

echo 🔨 编译PEEC测试程序...
cl %COMPILE_OPTS% %SOURCE_DIR%\peec_satellite_test_case.cpp /Fe:%OUTPUT_DIR%\peec_satellite_test.exe
if %errorlevel% neq 0 (
    echo ❌ PEEC测试程序编译失败
    exit /b 1
)
echo ✅ PEEC测试程序编译成功

echo 🔨 编译MoM测试程序...
cl %COMPILE_OPTS% %SOURCE_DIR%\mom_satellite_test_case.cpp /Fe:%OUTPUT_DIR%\mom_satellite_test.exe
if %errorlevel% neq 0 (
    echo ❌ MoM测试程序编译失败
    exit /b 1
)
echo ✅ MoM测试程序编译成功

echo 🔨 编译对比验证程序...
cl %COMPILE_OPTS% %SOURCE_DIR%\compare_fdtd_peec_mom.cpp /Fe:%OUTPUT_DIR%\compare_algorithms.exe
if %errorlevel% neq 0 (
    echo ❌ 对比验证程序编译失败
    exit /b 1
)
echo ✅ 对比验证程序编译成功

echo.
echo 🎯 构建完成！生成的文件:
echo   - %OUTPUT_DIR%\peec_satellite_test.exe
echo   - %OUTPUT_DIR%\mom_satellite_test.exe  
echo   - %OUTPUT_DIR%\compare_algorithms.exe
echo.
echo 🚀 运行测试: run_hpm_tests.bat
echo.
pause