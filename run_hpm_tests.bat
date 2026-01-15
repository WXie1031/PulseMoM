@echo off
REM run_hpm_tests.bat
REM HPM算法测试套件运行脚本

echo 🛰️ 运行卫星高功率微波激励算法测试
echo ═══════════════════════════════════════════════════════════

REM 设置路径
set "TEST_DIR=tests\test_hpm"
set "BUILD_DIR=%TEST_DIR%\build"

REM 检查构建目录
if not exist "%BUILD_DIR%" (
    echo ❌ 构建目录不存在，请先运行 build_hpm_test_suite.bat
    exit /b 1
)

echo 🎯 测试配置:
echo   - 卫星模型: weixing_v1 (STL格式)
echo   - 激励频率: 10 GHz
echo   - 入射角度: θ=45°, φ=45°, ψ=45°
echo   - 材料: PEC (理想电导体)
echo   - 对比算法: FDTD, PEEC, MoM
echo.

REM 运行PEEC测试
echo 🔬 运行PEEC算法测试...
cd /d "%BUILD_DIR%"
peec_satellite_test.exe
if %errorlevel% neq 0 (
    echo ❌ PEEC测试失败
    exit /b 1
)
echo ✅ PEEC测试完成
echo.

REM 运行MoM测试
echo 🔬 运行MoM算法测试...
mom_satellite_test.exe
if %errorlevel% neq 0 (
    echo ❌ MoM测试失败
    exit /b 1
)
echo ✅ MoM测试完成
echo.

REM 运行对比验证
echo 🔍 运行算法对比验证...
compare_algorithms.exe
if %errorlevel% neq 0 (
    echo ❌ 对比验证失败
    exit /b 1
)
echo ✅ 对比验证完成
echo.

REM 显示结果
echo 📊 测试结果摘要:
echo ═══════════════════════════════════════════════════════════

if exist "peec_satellite_results.txt" (
    echo ✅ PEEC结果: peec_satellite_results.txt
) else (
    echo ⚠️  PEEC结果文件未找到
)

if exist "mom_satellite_results.txt" (
    echo ✅ MoM结果: mom_satellite_results.txt
) else (
    echo ⚠️  MoM结果文件未找到
)

if exist "algorithm_comparison_report.txt" (
    echo ✅ 对比报告: algorithm_comparison_report.txt
) else (
    echo ⚠️  对比报告未找到
)

echo.
echo 🎯 测试完成！
echo.
echo 📋 后续分析:
echo   1. 查看详细对比报告: algorithm_comparison_report.txt
echo   2. 分析各算法的精度和性能差异
echo   3. 验证与FDTD基准的符合程度
echo   4. 评估算法适用性和优化方向
echo.
pause