cmake_minimum_required(VERSION 3.16)
project(PEEC_MoM_Unified_Framework VERSION 1.0.0 LANGUAGES C CXX)

# vcpkg integration
# Set vcpkg root path (updated to new location)
set(VCPKG_ROOT "D:/Project/vcpkg-master" CACHE PATH "vcpkg root directory")
if(EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/x64-windows")
    set(CGAL_DIR "${VCPKG_ROOT}/installed/x64-windows/share/cgal" CACHE PATH "CGAL config directory")
else()
    message(WARNING "vcpkg toolchain file not found at ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    message(WARNING "Please ensure vcpkg is installed at D:/Project/vcpkg-master")
endif()

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: enforce UTF-8 source encoding to avoid codepage warnings
if(MSVC)
  add_compile_options(/utf-8)
endif()

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler flags for different build types
if(MSVC)
  set(CMAKE_C_FLAGS_DEBUG "/Zi /Od /DDEBUG")
  set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG")
  add_compile_options(/W4)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
  set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
  add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

# Layered Greens implementation selection (Sommerfeld vs TMM)
option(USE_TMM_GREENS "Use TMM-based layered Greens implementation" OFF)
if (USE_TMM_GREENS)
  add_compile_definitions(GREENS_IMPL_TMM=1)
  message(STATUS "Layered Greens: using TMM implementation")
else()
  add_compile_definitions(GREENS_IMPL_SOMMERFELD=1)
  message(STATUS "Layered Greens: using Sommerfeld implementation")
endif()
if(MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /DDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/W4>)
  add_compile_definitions($<$<COMPILE_LANGUAGE:CXX>:_CRT_SECURE_NO_WARNINGS>)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall> $<$<COMPILE_LANGUAGE:CXX>:-Wextra> $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>)
endif()

# Find required packages
find_package(OpenMP REQUIRED)
find_package(Threads REQUIRED)

# ============================================================================
# vcpkg库集成（优先使用vcpkg，fallback到本地安装）
# ============================================================================

# OpenBLAS - 优先使用vcpkg
find_package(OpenBLAS QUIET)
if(OpenBLAS_FOUND)
    message(STATUS "OpenBLAS found via vcpkg: ${OpenBLAS_VERSION}")
    add_definitions(-DUSE_OPENBLAS -DENABLE_OPENBLAS)
    set(USE_OPENBLAS TRUE)
else()
    # Fallback to local installation
    set(OpenBLAS_DIR ${CMAKE_SOURCE_DIR}/libs/OpenBLAS-0.3.30-x64)
    if(EXISTS ${OpenBLAS_DIR}/lib/cmake/openblas/OpenBLASConfig.cmake)
        find_package(OpenBLAS QUIET PATHS ${OpenBLAS_DIR})
        if(OpenBLAS_FOUND)
            message(STATUS "OpenBLAS found in local libs")
            add_definitions(-DUSE_OPENBLAS -DENABLE_OPENBLAS)
            set(USE_OPENBLAS TRUE)
        endif()
    endif()
endif()

# HDF5 - 优先使用vcpkg
find_package(HDF5 QUIET COMPONENTS C HL)
if(HDF5_FOUND)
    message(STATUS "HDF5 found via vcpkg: ${HDF5_VERSION}")
    add_definitions(-DUSE_HDF5 -DENABLE_HDF5)
    set(USE_HDF5 TRUE)
else()
    # Fallback to system installation
    find_package(HDF5 QUIET)
    if(HDF5_FOUND)
        message(STATUS "HDF5 found in system")
        add_definitions(-DUSE_HDF5 -DENABLE_HDF5)
        set(USE_HDF5 TRUE)
    endif()
endif()

# SuperLU - 优先使用vcpkg
find_package(SuperLU QUIET)
if(SuperLU_FOUND)
    message(STATUS "SuperLU found via vcpkg")
    add_definitions(-DUSE_SUPERLU -DENABLE_SUPERLU)
    set(USE_SUPERLU TRUE)
else()
    message(STATUS "SuperLU not found, using dense matrix conversion")
endif()

# GSL - 优先使用vcpkg
find_package(GSL QUIET)
if(GSL_FOUND)
    message(STATUS "GSL found via vcpkg: ${GSL_VERSION}")
    add_definitions(-DUSE_GSL -DENABLE_GSL)
    set(USE_GSL TRUE)
else()
    message(STATUS "GSL not found, using custom integration methods")
endif()

# CGAL - 优先使用vcpkg
find_package(CGAL CONFIG QUIET)
if(CGAL_FOUND)
    message(STATUS "CGAL found via vcpkg: ${CGAL_VERSION}")
    add_definitions(-DUSE_CGAL)
    set(USE_CGAL TRUE)
else()
    # Fallback to local installation
    set(CMAKE_PREFIX_PATH 
        ${CMAKE_SOURCE_DIR}/libs/CGAL-6.1
        ${CMAKE_PREFIX_PATH}
    )
    find_package(CGAL CONFIG QUIET)
    if(CGAL_FOUND)
        message(STATUS "CGAL found in local libs")
        add_definitions(-DUSE_CGAL)
        set(USE_CGAL TRUE)
    else()
        message(STATUS "CGAL not found, using fallback implementations")
    endif()
endif()

# Gmsh - 优先使用vcpkg
find_package(Gmsh QUIET)
if(Gmsh_FOUND)
    message(STATUS "Gmsh found via vcpkg: ${Gmsh_VERSION}")
    add_definitions(-DUSE_GMSH)
    set(USE_GMSH TRUE)
else()
    # Fallback to local installation
    set(GMSH_ROOT ${CMAKE_SOURCE_DIR}/libs/gmsh-4.15.0-Windows64-sdk)
    if(EXISTS ${GMSH_ROOT}/include/gmshc.h)
        set(GMSH_INCLUDE_DIR ${GMSH_ROOT}/include)
        set(GMSH_LIBRARY ${GMSH_ROOT}/lib/gmsh.dll.lib)
        if(EXISTS ${GMSH_LIBRARY})
            message(STATUS "Gmsh C API found in local libs")
            add_definitions(-DUSE_GMSH)
            set(USE_GMSH TRUE)
        endif()
    endif()
endif()

# OpenCascade - 优先使用vcpkg
find_package(OpenCascade QUIET)
if(OpenCascade_FOUND)
    message(STATUS "OpenCascade found via vcpkg")
    add_definitions(-DUSE_OPENCASCADE)
    set(USE_OPENCASCADE TRUE)
else()
    # Fallback to local installation
    set(OpenCascade_DIR ${CMAKE_SOURCE_DIR}/libs/occt-vc14-64)
    if(EXISTS ${OpenCascade_DIR}/inc/STEPControl_Reader.hxx)
        message(STATUS "OpenCascade found in local libs")
        set(OpenCascade_INCLUDE_DIR ${OpenCascade_DIR}/inc)
        file(GLOB OpenCascade_LIBRARIES ${OpenCascade_DIR}/win64/vc14/lib/*.lib)
        add_definitions(-DUSE_OPENCASCADE)
        set(USE_OPENCASCADE TRUE)
    endif()
endif()

# Clipper2 integration
set(Clipper2_ROOT ${CMAKE_SOURCE_DIR}/libs/Clipper2_1.5.4)
if(EXISTS ${Clipper2_ROOT}/CPP/Clipper2Lib/include/clipper2/clipper.h)
    set(Clipper2_INCLUDE_DIR ${Clipper2_ROOT}/CPP/Clipper2Lib/include)
    add_definitions(-DUSE_CLIPPER2)
    message(STATUS "Clipper2 found")
endif()

# Triangle integration
set(Triangle_ROOT ${CMAKE_SOURCE_DIR}/libs/triangle)
if(EXISTS ${Triangle_ROOT}/triangle.h)
    set(Triangle_INCLUDE_DIR ${Triangle_ROOT})
    add_definitions(-DUSE_TRIANGLE)
    message(STATUS "Triangle found")
endif()

# Boost integration - 优先使用vcpkg
find_package(Boost CONFIG QUIET)
if(Boost_FOUND)
    message(STATUS "Boost found via vcpkg: ${Boost_VERSION}")
    add_definitions(-DUSE_BOOST)
    set(USE_BOOST TRUE)
else()
    # Fallback to local installation
    set(BOOST_ROOT ${CMAKE_SOURCE_DIR}/libs/boost_1_89_0)
    find_package(Boost CONFIG QUIET)
    if(Boost_FOUND)
        message(STATUS "Boost found in local libs: ${Boost_VERSION}")
        add_definitions(-DUSE_BOOST)
        set(USE_BOOST TRUE)
    endif()
endif()

# Eigen (可选) - 优先使用vcpkg
find_package(Eigen3 QUIET)
if(Eigen3_FOUND)
    message(STATUS "Eigen3 found via vcpkg: ${Eigen3_VERSION}")
    add_definitions(-DUSE_EIGEN)
    set(USE_EIGEN TRUE)
endif()

# PETSc integration (source build required)
set(PETSC_DIR ${CMAKE_SOURCE_DIR}/libs/petsc-3.24.1)
if(EXISTS ${PETSC_DIR}/include/petsc.h)
    set(PETSC_INCLUDE_DIR ${PETSC_DIR}/include)
    message(STATUS "PETSc headers found (source build required)")
    add_definitions(-DUSE_PETSC)
endif()

# H2Lib integration (source build)
set(H2LIB_DIR ${CMAKE_SOURCE_DIR}/libs/H2Lib-3.0.1)
if(EXISTS ${H2LIB_DIR}/Library/h2lib.h)
    set(H2LIB_INCLUDE_DIR ${H2LIB_DIR}/Library)
    message(STATUS "H2Lib headers found (source build required)")
    add_definitions(-DUSE_H2LIB)
endif()

# Optional: Find CUDA for GPU acceleration
find_package(CUDA QUIET)
if(CUDA_FOUND)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 14)
    add_definitions(-DUSE_CUDA)
endif()

# Optional: Find OpenCL for GPU acceleration
find_package(OpenCL QUIET)
if(OpenCL_FOUND)
    add_definitions(-DUSE_OPENCL)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src/discretization/geometry
    ${CMAKE_SOURCE_DIR}/src/discretization/mesh
    ${CMAKE_SOURCE_DIR}/src/operators/integration
    ${CMAKE_SOURCE_DIR}/src/operators/kernels
    ${CMAKE_SOURCE_DIR}/src/operators/greens
    ${CMAKE_SOURCE_DIR}/src/operators/assembler
    ${CMAKE_SOURCE_DIR}/src/operators/coupling
    ${CMAKE_SOURCE_DIR}/src/backend/solvers
    ${CMAKE_SOURCE_DIR}/src/backend/gpu
    ${CMAKE_SOURCE_DIR}/src/io/file_formats
    ${CMAKE_SOURCE_DIR}/src/io/api
    ${CMAKE_SOURCE_DIR}/src/io/cli
    ${CMAKE_SOURCE_DIR}/src/orchestration/execution
    ${CMAKE_SOURCE_DIR}/src/orchestration/hybrid_solver
    ${CMAKE_SOURCE_DIR}/src/orchestration/workflow
    ${CMAKE_SOURCE_DIR}/src/solvers/mom
    ${CMAKE_SOURCE_DIR}/src/solvers/peec
    ${CMAKE_SOURCE_DIR}/src/utils
)

# Add library-specific include directories
if(CGAL_FOUND)
    include_directories(${CGAL_INCLUDE_DIRS})
endif()

if(DEFINED GMSH_INCLUDE_DIR)
    include_directories(${GMSH_INCLUDE_DIR})
endif()

if(DEFINED OpenCascade_INCLUDE_DIR)
    include_directories(${OpenCascade_INCLUDE_DIR})
endif()

if(DEFINED Clipper2_INCLUDE_DIR)
    include_directories(${Clipper2_INCLUDE_DIR})
endif()

if(DEFINED Triangle_INCLUDE_DIR)
    include_directories(${Triangle_INCLUDE_DIR})
endif()

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

if(DEFINED PETSC_INCLUDE_DIR)
    include_directories(${PETSC_INCLUDE_DIR})
endif()

if(DEFINED H2LIB_INCLUDE_DIR)
    include_directories(${H2LIB_INCLUDE_DIR})
endif()

if(OpenBLAS_FOUND)
    include_directories(${OpenBLAS_INCLUDE_DIRS})
endif()

# MSVC: enable include trace to diagnose header resolution
if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:/showIncludes> $<$<COMPILE_LANGUAGE:CXX>:/showIncludes>)
endif()

# Source files for core library
if(MSVC)
    set(CORE_SOURCES
        src/discretization/geometry/core_geometry.c
        src/operators/integration/integration_utils.c
        src/operators/kernels/electromagnetic_kernels.c
        src/operators/greens/layered_greens_function_unified.c
        src/discretization/mesh/mesh_pipeline.c
        src/operators/kernels/kernel_mfie.c
        src/operators/kernels/kernel_cfie.c
        src/operators/integration/integral_nearly_singular.c
    )
else()
    set(CORE_SOURCES
        src/discretization/geometry/core_geometry.c
        src/discretization/mesh/core_mesh_unified.c
        src/operators/kernels/core_kernels.c
        src/operators/assembler/core_assembler.c
        src/backend/solvers/core_solver.c
        src/backend/gpu/gpu_acceleration.c
        src/io/file_formats/spice_export.c
        src/operators/integration/integration_utils.c
        src/operators/kernels/electromagnetic_kernels.c
        src/operators/greens/layered_greens_function_unified.c
        src/discretization/mesh/mesh_pipeline.c
        src/operators/kernels/kernel_mfie.c
        src/operators/kernels/kernel_cfie.c
        src/operators/integration/integral_nearly_singular.c
    )
endif()

# Source files for mesh generation with library integrations
if (MSVC)
  set(MESH_SOURCES
      src/discretization/mesh/mesh_engine.c
      src/discretization/mesh/mesh_subsystem.c
  )
else()
  set(MESH_SOURCES
      src/discretization/mesh/mesh_engine.c
      src/discretization/mesh/mesh_algorithms.c
      src/discretization/mesh/mesh_subsystem.c
  )
endif()

# Add CGAL mesh implementation if available
if(CGAL_FOUND AND NOT MSVC)
    list(APPEND MESH_SOURCES src/discretization/mesh/cgal_surface_mesh.cpp)
endif()

# Source files for CAD import with library integrations
set(CAD_SOURCES
    src/discretization/geometry/cad_mesh_generation.c
)

# Add OpenCascade CAD import if available
if(DEFINED USE_OPENCASCADE)
    list(APPEND CAD_SOURCES src/discretization/geometry/opencascade_cad_import.cpp)
endif() 

# Add Gmsh surface mesh generation if available
if(DEFINED USE_GMSH)
    #TODO: Add Gmsh surface mesh generation
    list(APPEND CAD_SOURCES src/cad/cad_gmsh_surface.cpp)
endif()

# Source files for 2D geometry processing
set(GEOMETRY_SOURCES
    src/discretization/geometry/pcb_ic_structures.c
)

# Add Clipper2 2D operations if available
if(DEFINED USE_CLIPPER2)
    list(APPEND GEOMETRY_SOURCES src/discretization/geometry/clipper2_adapter.cpp)
endif()

# Add Boost.Geometry operations if available
if(DEFINED USE_BOOST)
    #TODO: Add Boost.Geometry operations
    list(APPEND GEOMETRY_SOURCES src/geometry/boost_geometry_adapter.cpp)
endif()

# Source files for MoM solver
set(MOM_SOURCES
    src/solvers/mom/mom_solver_unified.c
    src/solvers/mom/mom_matvec.c
    src/discretization/mesh/tri_mesh.c
)

# Source files for PEEC solver
set(PEEC_SOURCES
    src/solvers/peec/peec_solver_unified.c
    src/discretization/mesh/manhattan_mesh.c
)

# Create core library (excluded from ALL to avoid building experimental modules by default)
add_library(electromagnetic_kernel_library STATIC EXCLUDE_FROM_ALL ${CORE_SOURCES})
if(MSVC)
    target_link_libraries(electromagnetic_kernel_library PUBLIC OpenMP::OpenMP_C Threads::Threads)
else()
    target_link_libraries(electromagnetic_kernel_library PUBLIC OpenMP::OpenMP_C Threads::Threads m)
endif()

# Create shared library for Python wrapper compatibility
add_library(electromagnetic_kernels SHARED ${CORE_SOURCES})
if(MSVC)
    target_link_libraries(electromagnetic_kernels PUBLIC OpenMP::OpenMP_C Threads::Threads)
else()
    target_link_libraries(electromagnetic_kernels PUBLIC OpenMP::OpenMP_C Threads::Threads m)
endif()

# Set proper export symbols for Windows
if(MSVC)
    target_compile_definitions(electromagnetic_kernels PRIVATE BUILDING_ELECTROMAGNETIC_KERNELS_DLL)
endif()

if(CUDA_FOUND)
    target_link_libraries(electromagnetic_kernel_library PUBLIC ${CUDA_LIBRARIES})
endif()

if(OpenCL_FOUND)
    target_link_libraries(electromagnetic_kernel_library PUBLIC ${OpenCL_LIBRARIES})
endif()

if(OpenBLAS_FOUND)
    target_link_libraries(electromagnetic_kernel_library PUBLIC ${OpenBLAS_LIBRARIES})
endif()

if(DEFINED PETSC_INCLUDE_DIR)
    # PETSc requires compilation from source - add as include for now
    target_include_directories(electromagnetic_kernel_library PUBLIC ${PETSC_INCLUDE_DIR})
endif()

if(DEFINED H2LIB_INCLUDE_DIR)
    # H2Lib requires compilation from source - add as include for now
    target_include_directories(electromagnetic_kernel_library PUBLIC ${H2LIB_INCLUDE_DIR})
endif()

# Create geometry processing library
add_library(geometry_processing_library STATIC ${GEOMETRY_SOURCES})

# Create mesh generation library
add_library(mesh_generation_library STATIC ${MESH_SOURCES})
target_link_libraries(mesh_generation_library PUBLIC geometry_processing_library)

# Add library-specific linking for mesh library
if(CGAL_FOUND)
    target_link_libraries(mesh_generation_library PUBLIC ${CGAL_LIBRARIES})
endif()

if(NOT MSVC)
  add_library(cad_import_library STATIC ${CAD_SOURCES})
  target_link_libraries(cad_import_library PUBLIC mesh_generation_library geometry_processing_library)
endif()

# Add library-specific linking for CAD library
if(TARGET cad_import_library)
  if(DEFINED GMSH_LIBRARY)
      target_link_libraries(cad_import_library PUBLIC ${GMSH_LIBRARY})
  endif()
  if(DEFINED OpenCascade_LIBRARIES)
      target_link_libraries(cad_import_library PUBLIC ${OpenCascade_LIBRARIES})
  endif()
endif()

# Create MoM solver library (unified version, MSVC-compatible)
set(MOM_SOURCES 
    src/solvers/mom/mom_solver_unified.c
    src/solvers/mom/mom_matvec.c
    src/discretization/mesh/tri_mesh.c
)
add_library(mom_solver STATIC ${MOM_SOURCES})
target_link_libraries(mom_solver PUBLIC geometry_processing_library electromagnetic_kernel_library)

# Create PEEC solver library (unified version with comprehensive geometry support, MSVC-compatible)
set(PEEC_SOURCES 
    src/solvers/peec/peec_solver_unified.c
    src/discretization/mesh/manhattan_mesh.c
    #TODO: Add Manhattan mesh generation，peec_manhattan_mesh.c
    src/solvers/peec/peec_manhattan_mesh.c
    src/solvers/peec/peec_non_manhattan_geometry.c
    #TODO: Add Triangular mesh generation
    src/solvers/peec/peec_triangular_mesh.c
    src/solvers/peec/peec_geometry_support.c
    src/solvers/peec/peec_directionality_kernels.c
)
add_library(peec_solver STATIC ${PEEC_SOURCES})
target_link_libraries(peec_solver PUBLIC mesh_generation_library)

# --- Apps ---
# Standalone executables for MoM and PEEC solvers
# These can be built separately or together

# Option to build standalone executables
option(BUILD_STANDALONE_EXECUTABLES "Build standalone MoM and PEEC executables" ON)
option(BUILD_MOM_EXE "Build MoM solver executable" ${BUILD_STANDALONE_EXECUTABLES})
option(BUILD_PEEC_EXE "Build PEEC solver executable" ${BUILD_STANDALONE_EXECUTABLES})

# MoM Solver Executable
if(BUILD_MOM_EXE)
    add_executable(mom_solver_exe
        src/apps/mom_cli/main_mom.c
    )
    target_link_libraries(mom_solver_exe
        PRIVATE
        mom_solver
        electromagnetic_kernel_library
        geometry_processing_library
        mesh_generation_library
        core_base_library
    )
    if(OpenBLAS_FOUND)
        target_link_libraries(mom_solver_exe PRIVATE ${OpenBLAS_LIBRARIES})
    endif()
    if(HDF5_FOUND)
        target_link_libraries(mom_solver_exe PRIVATE ${HDF5_LIBRARIES})
    endif()
    target_link_libraries(mom_solver_exe PRIVATE OpenMP::OpenMP_C Threads::Threads)
endif()

# PEEC Solver Executable
if(BUILD_PEEC_EXE)
    add_executable(peec_solver_exe
        src/apps/peec_cli/main_peec.c
    )
    target_link_libraries(peec_solver_exe
        PRIVATE
        peec_solver
        electromagnetic_kernel_library
        mesh_generation_library
        geometry_processing_library
        core_base_library
    )
    if(OpenBLAS_FOUND)
        target_link_libraries(peec_solver_exe PRIVATE ${OpenBLAS_LIBRARIES})
    endif()
    if(HDF5_FOUND)
        target_link_libraries(peec_solver_exe PRIVATE ${HDF5_LIBRARIES})
    endif()
    target_link_libraries(peec_solver_exe PRIVATE OpenMP::OpenMP_C Threads::Threads)
endif()

# NOTE: Legacy CLI applications in apps/ folder have been deprecated.
# Use src/io/cli/cli_main.c (new architecture) or Python interface instead.
# Python interface: python_interface/mom_peec_ctypes.py

# Library integration test executable
if(MSVC)
  add_executable(library_integration_test tests/library_integration_test_msvc_stub.c)
else()
  add_executable(library_integration_test tests/library_integration_test.c)
  target_link_libraries(library_integration_test 
      mesh_generation_library 
      geometry_processing_library
      ${CMAKE_DL_LIBS}
  )
  if(TARGET cad_import_library)
    target_link_libraries(library_integration_test cad_import_library)
  endif()
  target_link_libraries(library_integration_test electromagnetic_kernel_library)
endif()

# Comprehensive test suite
if(MSVC)
  add_executable(comprehensive_test_suite tests/comprehensive_test_suite_msvc_stub.c)
else()
  add_executable(comprehensive_test_suite tests/comprehensive_test_suite.c)
  target_link_libraries(comprehensive_test_suite 
      mesh_generation_library geometry_processing_library)
  if(TARGET cad_import_library)
    target_link_libraries(comprehensive_test_suite cad_import_library)
  endif()
  target_link_libraries(comprehensive_test_suite electromagnetic_kernel_library)
  if(TARGET peec_solver)
    target_link_libraries(comprehensive_test_suite peec_solver)
  endif()
  if(TARGET mom_solver)
    target_link_libraries(comprehensive_test_suite mom_solver)
  endif()
endif()

# MTL arbitrary routing example (without MTL solver for now)
# add_executable(mtl_arbitrary_routing_example examples/mtl_arbitrary_routing_example.c)
# target_link_libraries(mtl_arbitrary_routing_example electromagnetic_kernel_library)

# Installation
# NOTE: Legacy CLI targets (mom_cli, peec_cli) have been removed.
# Use src/io/cli/cli_main.c or Python interface instead.
# set(APP_TARGETS mom_cli peec_cli)
# install(TARGETS ${APP_TARGETS}
#     RUNTIME DESTINATION bin

set(LIB_TARGETS electromagnetic_kernel_library 
    mesh_generation_library geometry_processing_library)
if(TARGET cad_import_library)
  list(APPEND LIB_TARGETS cad_import_library)
endif()
if(TARGET peec_solver)
  list(APPEND LIB_TARGETS peec_solver)
endif()
if(TARGET mom_solver)
  list(APPEND LIB_TARGETS mom_solver)
endif()
install(TARGETS ${LIB_TARGETS}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY src/solvers src/discretization/mesh src/discretization/geometry
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# CPack configuration for distribution
set(CPACK_PACKAGE_NAME "PEEC-MoM-Unified-Framework")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Commercial-grade PEEC-MoM electromagnetic simulation framework")
set(CPACK_PACKAGE_VENDOR "Electromagnetic Simulation Group")
set(CPACK_PACKAGE_CONTACT "support@em-sim.org")

include(CPack)

# Testing
enable_testing()
if(TARGET comprehensive_test_suite)
  add_test(NAME ComprehensiveValidation 
      COMMAND comprehensive_test_suite)
endif()

# Benchmark target
if(TARGET comprehensive_test_suite)
  add_custom_target(benchmark
      COMMAND comprehensive_test_suite
      DEPENDS comprehensive_test_suite
      COMMENT "Running comprehensive benchmark suite"
  )
endif()

# Documentation target
add_custom_target(docs
    COMMAND doxygen Doxyfile
    COMMENT "Generating documentation"
)

# Performance profiling target
add_custom_target(profile
    COMMAND valgrind --tool=callgrind ./comprehensive_test_suite
    DEPENDS comprehensive_test_suite
    COMMENT "Running performance profiling"
)
# --- Probe executables for dependency verification ---
if (CGAL_FOUND)
  add_executable(cgal_probe src/probes/cgal_probe.cpp)
  target_link_libraries(cgal_probe PRIVATE CGAL::CGAL)
  set_source_files_properties(src/probes/cgal_probe.cpp PROPERTIES LANGUAGE CXX)
endif()

if (Boost_FOUND)
  add_executable(boost_probe src/probes/boost_probe.cpp)
  target_link_libraries(boost_probe PRIVATE Boost::headers)
  set_source_files_properties(src/probes/boost_probe.cpp PROPERTIES LANGUAGE CXX)
endif()
if (DEFINED GMSH_LIBRARY AND DEFINED GMSH_INCLUDE_DIR)
  add_executable(gmsh_probe src/probes/gmsh_probe.c)
  target_include_directories(gmsh_probe PRIVATE ${GMSH_INCLUDE_DIR})
  target_link_libraries(gmsh_probe PRIVATE ${GMSH_LIBRARY})
endif()

if (OpenBLAS_FOUND)
  add_executable(openblas_probe src/probes/openblas_probe.c)
  # Force linking against import library to avoid accidental .dll linkage
  if (EXISTS ${OpenBLAS_DIR}/lib/libopenblas.lib)
    target_link_libraries(openblas_probe PRIVATE ${OpenBLAS_DIR}/lib/libopenblas.lib)
  elseif (TARGET OpenBLAS::OpenBLAS)
    target_link_libraries(openblas_probe PRIVATE OpenBLAS::OpenBLAS)
  endif()
endif()

# Suppress policy warnings for modern behavior
if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()
if(POLICY CMP0146)
  cmake_policy(SET CMP0146 NEW)
endif()
