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

# Set library paths for local installations
set(CMAKE_PREFIX_PATH 
    ${CMAKE_SOURCE_DIR}/libs/gmsh-4.15.0-Windows64-sdk
    ${CMAKE_SOURCE_DIR}/libs/occt-vc14-64
    ${CMAKE_SOURCE_DIR}/libs/boost_1_89_0
    ${CMAKE_SOURCE_DIR}/libs/OpenBLAS-0.3.30-x64
    ${CMAKE_SOURCE_DIR}/libs/petsc-3.24.1
    ${CMAKE_PREFIX_PATH}
)

# CGAL integration
find_package(CGAL CONFIG QUIET)
if(CGAL_FOUND)
    message(STATUS "CGAL found: ${CGAL_VERSION}")
    add_definitions(-DUSE_CGAL)
else()
    message(STATUS "CGAL not found, using fallback implementations")
endif()

# Gmsh integration - use C API for Windows compatibility
set(GMSH_ROOT ${CMAKE_SOURCE_DIR}/libs/gmsh-4.15.0-Windows64-sdk)
if(EXISTS ${GMSH_ROOT}/include/gmshc.h)
    set(GMSH_INCLUDE_DIR ${GMSH_ROOT}/include)
    set(GMSH_LIBRARY ${GMSH_ROOT}/lib/gmsh.dll.lib)
    if(EXISTS ${GMSH_LIBRARY})
        message(STATUS "Gmsh C API found")
        add_definitions(-DUSE_GMSH)
    endif()
endif()

# OpenCascade integration
set(OpenCascade_DIR ${CMAKE_SOURCE_DIR}/libs/occt-vc14-64)
if(EXISTS ${OpenCascade_DIR}/inc/STEPControl_Reader.hxx)
    message(STATUS "OpenCascade found in local libs")
    set(OpenCascade_INCLUDE_DIR ${OpenCascade_DIR}/inc)
    file(GLOB OpenCascade_LIBRARIES ${OpenCascade_DIR}/win64/vc14/lib/*.lib)
    add_definitions(-DUSE_OPENCASCADE)
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

# Boost integration
set(BOOST_ROOT ${CMAKE_SOURCE_DIR}/libs/boost_1_89_0)
find_package(Boost CONFIG QUIET)
if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
    add_definitions(-DUSE_BOOST)
endif()

# OpenBLAS integration
set(OpenBLAS_DIR ${CMAKE_SOURCE_DIR}/libs/OpenBLAS-0.3.30-x64)
if(EXISTS ${OpenBLAS_DIR}/lib/cmake/openblas/OpenBLASConfig.cmake)
    find_package(OpenBLAS QUIET)
    if(OpenBLAS_FOUND)
        message(STATUS "OpenBLAS found")
        add_definitions(-DUSE_OPENBLAS)
    endif()
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
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/solvers/mom
    ${CMAKE_SOURCE_DIR}/src/solvers/peec
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/src/mesh
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
        src/core/core_geometry.c
        src/core/integration_utils.c
        src/core/electromagnetic_kernels.c
        src/core/layered_greens_function_unified.c
        src/core/core_mesh_pipeline.c
        src/core/kernel_mfie.c
        src/core/kernel_cfie.c
        src/core/integral_nearly_singular.c
    )
else()
    set(CORE_SOURCES
        src/core/core_geometry.c
        src/core/core_mesh_unified.c
        src/core/core_kernels.c
        src/core/core_assembler.c
        src/core/core_solver.c
        src/core/gpu_acceleration.c
        src/core/spice_export.c
        src/core/integration_utils.c
        src/core/electromagnetic_kernels.c
        src/core/layered_greens_function_unified.c
        src/core/core_mesh_pipeline.c
        src/core/kernel_mfie.c
        src/core/kernel_cfie.c
        src/core/integral_nearly_singular.c
    )
endif()

# Source files for mesh generation with library integrations
if (MSVC)
  set(MESH_SOURCES
      src/mesh/mesh_engine.c
      src/mesh/mesh_subsystem.c
  )
else()
  set(MESH_SOURCES
      src/mesh/mesh_engine.c
      src/mesh/mesh_algorithms.c
      src/mesh/mesh_subsystem.c
  )
endif()

# Add CGAL mesh implementation if available
if(CGAL_FOUND AND NOT MSVC)
    list(APPEND MESH_SOURCES src/mesh/cgal_surface_mesh.cpp)
endif()

# Source files for CAD import with library integrations
set(CAD_SOURCES
    src/cad/cad_mesh_generation.c
)

# Add OpenCascade CAD import if available
if(DEFINED USE_OPENCASCADE)
    list(APPEND CAD_SOURCES src/mesh/opencascade_cad_import.cpp)
endif()

# Add Gmsh surface mesh generation if available
if(DEFINED USE_GMSH)
    list(APPEND CAD_SOURCES src/cad/cad_gmsh_surface.cpp)
endif()

# Source files for 2D geometry processing
set(GEOMETRY_SOURCES
    src/geometry/pcb_ic_structures.c
)

# Add Clipper2 2D operations if available
if(DEFINED USE_CLIPPER2)
    list(APPEND GEOMETRY_SOURCES src/geometry/clipper2_adapter.cpp)
endif()

# Add Boost.Geometry operations if available
if(DEFINED USE_BOOST)
    list(APPEND GEOMETRY_SOURCES src/geometry/boost_geometry_adapter.cpp)
endif()

# Source files for MoM solver
set(MOM_SOURCES
    src/solvers/mom/mom_solver_unified.c
    src/solvers/mom/tri_mesh.c
)

# Source files for PEEC solver
set(PEEC_SOURCES
    src/solvers/peec/peec_solver_unified.c
    src/solvers/peec/manhattan_mesh.c
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
    src/solvers/mom/tri_mesh.c
)
add_library(mom_solver STATIC ${MOM_SOURCES})
target_link_libraries(mom_solver PUBLIC geometry_processing_library electromagnetic_kernel_library)

# Create PEEC solver library (unified version with comprehensive geometry support, MSVC-compatible)
set(PEEC_SOURCES 
    src/solvers/peec/peec_solver_unified.c
    src/solvers/peec/manhattan_mesh.c
    src/solvers/peec/peec_manhattan_mesh.c
    src/solvers/peec/peec_non_manhattan_geometry.c
    src/solvers/peec/peec_triangular_mesh.c
    src/solvers/peec/peec_geometry_support.c
    src/solvers/peec/peec_directionality_kernels.c
)
add_library(peec_solver STATIC ${PEEC_SOURCES})
target_link_libraries(peec_solver PUBLIC mesh_generation_library)

# --- Apps ---
add_executable(mom_cli apps/mom_cli.c)
if(MSVC)
  set(CORE_BASE_SOURCES
      src/core/core_geometry_min.c
  )
else()
  set(CORE_BASE_SOURCES
      src/core/core_geometry_min.c
      src/core/core_mesh_unified.c
      src/core/layered_greens_function_unified.c
  )
endif()
add_library(core_base_library STATIC ${CORE_BASE_SOURCES})
target_link_libraries(core_base_library PUBLIC OpenMP::OpenMP_C Threads::Threads)
target_link_libraries(mom_cli PRIVATE core_base_library)
if(TARGET electromagnetic_kernels)
  target_link_libraries(mom_cli PRIVATE electromagnetic_kernels)
endif()
target_compile_definitions(mom_cli PRIVATE MOM_CLI_STUB_ONLY=1)
# Optional minimal core library is not required for stub-only CLI

if(MSVC)
  add_executable(peec_cli apps/peec_cli_msvc_stub.c)
  target_link_libraries(peec_cli peec_solver mesh_generation_library)
else()
  add_executable(peec_cli apps/peec_cli.c)
  target_link_libraries(peec_cli peec_solver mesh_generation_library)
endif()

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
set(APP_TARGETS mom_cli peec_cli)
install(TARGETS ${APP_TARGETS}
    RUNTIME DESTINATION bin
)

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

install(DIRECTORY src/core src/solvers src/mesh src/cad src/geometry
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
