@echo off
REM Generate both MoM and PEEC Solver Visual Studio Solutions

echo ========================================
echo Generating Both Solutions
echo ========================================
echo.

call %~dp0generate_mom_solution.bat
if %ERRORLEVEL% NEQ 0 (
    echo Failed to generate MoM solution
    exit /b 1
)

echo.
echo ========================================
echo.

call %~dp0generate_peec_solution.bat
if %ERRORLEVEL% NEQ 0 (
    echo Failed to generate PEEC solution
    exit /b 1
)

echo.
echo ========================================
echo Both solutions generated successfully!
echo ========================================
echo.
echo MoM Solver: build_mom\MoM_Solver.sln
echo PEEC Solver: build_peec\PEEC_Solver.sln
echo.

pause
