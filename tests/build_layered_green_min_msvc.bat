@echo off
setlocal

if not defined VSINSTALLDIR (
  echo [ERROR] Please run in a MSVC Developer Command Prompt.
  exit /b 1
)

echo [BUILD] test_layered_green_min.exe
cl /nologo /I ..\src\core ..\src\core\electromagnetic_kernels.c test_layered_green_min.c /Fe:test_layered_green_min.exe
if %errorlevel% neq 0 (
  echo [ERROR] Build failed for test_layered_green_min.exe
  exit /b %errorlevel%
)
echo [RUN] test_layered_green_min.exe
test_layered_green_min.exe

echo [BUILD] mom_cli.exe (stub, green-scan)
cl /nologo /DMOM_CLI_STUB_ONLY /I ..\src\core ..\apps\mom_cli.c ..\src\core\electromagnetic_kernels.c ..\src\core\core_geometry.c ..\src\core\core_mesh_unified.c /Fe:mom_cli.exe
if %errorlevel% neq 0 (
  echo [ERROR] Build failed for mom_cli.exe
  exit /b %errorlevel%
)
echo [RUN] mom_cli.exe --green-scan
mom_cli.exe --green-scan

echo [DONE]
exit /b 0
