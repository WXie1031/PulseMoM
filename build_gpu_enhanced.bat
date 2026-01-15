@echo off
REM Enhanced build script for PulseMoM with GPU acceleration
REM Supports CUDA, cuBLAS, cuSOLVER, cuSPARSE, and multi-GPU optimization

echo ============================================
echo PulseMoM GPU-Accelerated Build Script
echo ============================================
echo.

REM Set default values
set BUILD_TYPE=Release
set USE_CUDA=ON
set USE_OPENMP=ON
set USE_AVX2=ON
set USE_MKL=OFF
set USE_MPI=OFF
set BUILD_TESTS=ON
set BUILD_BENCHMARKS=ON
set ENABLE_PROFILING=OFF
set CUDA_ARCH=sm_70
set MAX_GPUS=8

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :check_cuda
if /i "%~1"=="debug" set BUILD_TYPE=Debug
if /i "%~1"=="release" set BUILD_TYPE=Release
if /i "%~1"=="relwithdebinfo" set BUILD_TYPE=RelWithDebInfo
if /i "%~1"=="no-cuda" set USE_CUDA=OFF
if /i "%~1"=="no-openmp" set USE_OPENMP=OFF
if /i "%~1"=="no-avx2" set USE_AVX2=OFF
if /i "%~1"=="with-mkl" set USE_MKL=ON
if /i "%~1"=="with-mpi" set USE_MPI=ON
if /i "%~1"=="no-tests" set BUILD_TESTS=OFF
if /i "%~1"=="no-benchmarks" set BUILD_BENCHMARKS=OFF
if /i "%~1"=="with-profiling" set ENABLE_PROFILING=ON
if /i "%~1"=="help" goto :show_help
shift
goto :parse_args

:check_cuda
REM Check for CUDA installation
echo Checking CUDA installation...
if "%USE_CUDA%"=="ON" (
    where nvcc >nul 2>nul
    if %ERRORLEVEL% NEQ 0 (
        echo WARNING: CUDA compiler (nvcc) not found in PATH
        echo GPU acceleration will be disabled
        set USE_CUDA=OFF
    ) else (
        echo Found CUDA compiler: 
        nvcc --version | findstr "release"
        
        REM Detect CUDA architecture
        echo Detecting optimal CUDA architecture...
        nvidia-smi >nul 2>nul
        if %ERRORLEVEL% EQU 0 (
            echo Detected NVIDIA GPU(s):
            nvidia-smi --query-gpu=name,compute_cap --format=csv,noheader
            
            REM Try to detect compute capability automatically
            for /f "tokens=2 delims=." %%i in ('nvidia-smi --query-gpu=compute_cap --format=csv,noheader ^| head -1') do (
                set COMPUTE_MAJOR=%%i
            )
            for /f "tokens=3 delims=." %%i in ('nvidia-smi --query-gpu=compute_cap --format=csv,noheader ^| head -1') do (
                set COMPUTE_MINOR=%%i
            )
            set CUDA_ARCH=sm_%COMPUTE_MAJOR%%COMPUTE_MINOR%
            echo Setting CUDA architecture to: %CUDA_ARCH%
        ) else (
            echo WARNING: nvidia-smi not found, using default architecture: %CUDA_ARCH%
        )
    )
)
echo.

REM Create build directory
set BUILD_DIR=build_%BUILD_TYPE%_GPU
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

REM Configure with CMake
echo Configuring with CMake...
echo Build Type: %BUILD_TYPE%
echo CUDA Support: %USE_CUDA%
echo OpenMP Support: %USE_OPENMP%
echo AVX2 Support: %USE_AVX2%
echo Intel MKL: %USE_MKL%
echo MPI Support: %USE_MPI%
echo Build Tests: %BUILD_TESTS%
echo Build Benchmarks: %BUILD_BENCHMARKS%
echo Profiling: %ENABLE_PROFILING%
echo CUDA Architecture: %CUDA_ARCH%
echo.

REM Set vcpkg toolchain
set VCPKG_ROOT=D:\Project\vcpkg-master
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
if exist "%CMAKE_TOOLCHAIN_FILE%" (
    echo Using vcpkg toolchain: %CMAKE_TOOLCHAIN_FILE%
    set VCPKG_ARG=-DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%"
) else (
    echo WARNING: vcpkg toolchain not found, building without vcpkg
    set VCPKG_ARG=
)
echo.

cmake -G "Visual Studio 17 2022" -A x64 ^
    %VCPKG_ARG% ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DUSE_CUDA=%USE_CUDA% ^
    -DUSE_OPENMP=%USE_OPENMP% ^
    -DUSE_AVX2=%USE_AVX2% ^
    -DUSE_MKL=%USE_MKL% ^
    -DUSE_MPI=%USE_MPI% ^
    -DBUILD_TESTS=%BUILD_TESTS% ^
    -DBUILD_BENCHMARKS=%BUILD_BENCHMARKS% ^
    -DENABLE_PROFILING=%ENABLE_PROFILING% ^
    -DCMAKE_CUDA_ARCHITECTURES=%CUDA_ARCH% ^
    -DCMAKE_CUDA_FLAGS="-arch=%CUDA_ARCH% -O3" ^
    -DCUDA_TOOLKIT_ROOT_DIR="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.0" ^
    -DCUBLAS_LIBRARY="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.0/lib/x64/cublas.lib" ^
    -DCUSOLVER_LIBRARY="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.0/lib/x64/cusolver.lib" ^
    -DCUSPARSE_LIBRARY="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.0/lib/x64/cusparse.lib" ^
    -DNVML_LIBRARY="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.0/lib/x64/nvml.lib" ^
    -DCMAKE_INSTALL_PREFIX="../install_%BUILD_TYPE%_GPU" ^
    -f ../CMakeLists_optimized.txt ..

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    cd ..
    exit /b 1
)

REM Build the project
echo.
echo Building PulseMoM with GPU acceleration...
cmake --build . --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)

REM Run validation tests if enabled
if "%BUILD_TESTS%"=="ON" (
    echo.
    echo Running validation tests...
    ctest -C %BUILD_TYPE% --output-on-failure
    
    if %ERRORLEVEL% NEQ 0 (
        echo WARNING: Some tests failed
    )
)

REM Run benchmarks if enabled
if "%BUILD_BENCHMARKS%"=="ON" (
    echo.
    echo Running performance benchmarks...
    echo This may take several minutes...
    
    if "%USE_CUDA%"=="ON" (
        echo Running GPU-accelerated benchmarks...
        %BUILD_TYPE%\PulseMoM.exe --benchmark --gpu --validation --multiscale --parallel
    ) else (
        echo Running CPU benchmarks...
        %BUILD_TYPE%\PulseMoM.exe --benchmark --validation --multiscale --parallel
    )
)

REM Generate performance report
echo.
echo Generating performance report...
echo ============================================ > performance_report.txt
echo PulseMoM GPU Acceleration Performance Report >> performance_report.txt
echo ============================================ >> performance_report.txt
echo Build Configuration: >> performance_report.txt
echo   Build Type: %BUILD_TYPE% >> performance_report.txt
echo   CUDA Support: %USE_CUDA% >> performance_report.txt
echo   CUDA Architecture: %CUDA_ARCH% >> performance_report.txt
echo   OpenMP Support: %USE_OPENMP% >> performance_report.txt
echo   AVX2 Support: %USE_AVX2% >> performance_report.txt
echo   Intel MKL: %USE_MKL% >> performance_report.txt
echo   MPI Support: %USE_MPI% >> performance_report.txt
echo. >> performance_report.txt

if "%USE_CUDA%"=="ON" (
    echo GPU Information: >> performance_report.txt
    nvidia-smi --query-gpu=name,memory.total,memory.free,compute_cap --format=csv >> performance_report.txt 2>nul
    echo. >> performance_report.txt
    
    echo CUDA Information: >> performance_report.txt
    nvcc --version | findstr "release" >> performance_report.txt
    echo. >> performance_report.txt
)

echo Build completed successfully! >> performance_report.txt
echo Check install_%BUILD_TYPE%_GPU for installation files. >> performance_report.txt

REM Install the project
echo.
echo Installing PulseMoM...
cmake --install . --config %BUILD_TYPE%

cd ..
echo.
echo ============================================
echo Build completed successfully!
echo ============================================
echo.
echo Installation directory: install_%BUILD_TYPE%_GPU
echo Performance report: %BUILD_DIR%\performance_report.txt
echo.
echo GPU Optimization Features Available:
echo - GPU-accelerated Sommerfeld integration
echo - H-matrix compression with ACA algorithm
echo - cuBLAS/cuSOLVER linear algebra optimization
echo - Multi-GPU work distribution with load balancing
echo - Mixed-precision iterative solvers
echo - Advanced preconditioning techniques
echo - Real-time performance monitoring
echo.

if "%USE_CUDA%"=="ON" (
    echo To run GPU demonstrations:
    echo   install_%BUILD_TYPE%_GPU\bin\PulseMoM.exe --gpu-demo
echo   install_%BUILD_TYPE%_GPU\bin\PulseMoM.exe --benchmark --gpu
echo   install_%BUILD_TYPE%_GPU\bin\PulseMoM.exe --multiscale --parallel
echo.
)

exit /b 0

:show_help
echo Usage: build_gpu_enhanced.bat [options]
echo.
echo Options:
echo   debug           - Build in Debug mode (default: Release)
echo   release         - Build in Release mode
echo   relwithdebinfo  - Build in RelWithDebInfo mode
echo   no-cuda         - Disable CUDA support
echo   no-openmp       - Disable OpenMP support
echo   no-avx2         - Disable AVX2 vectorization
echo   with-mkl        - Enable Intel MKL support
echo   with-mpi        - Enable MPI support
echo   no-tests        - Disable building tests
echo   no-benchmarks   - Disable building benchmarks
echo   with-profiling  - Enable performance profiling
echo   help            - Show this help message
echo.
echo Examples:
echo   build_gpu_enhanced.bat                    - Default Release build with CUDA
echo   build_gpu_enhanced.bat debug            - Debug build with CUDA
echo   build_gpu_enhanced.bat release no-cuda  - Release build without CUDA
echo   build_gpu_enhanced.bat with-mkl with-mpi - Build with MKL and MPI
echo.
exit /b 0