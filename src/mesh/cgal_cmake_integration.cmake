/********************************************************************************
 *  PulseEM - CGAL Integration CMake Configuration
 *
 *  Enhanced CMakeLists.txt with CGAL integration for commercial-grade mesh generation
 ********************************************************************************/

# Enhanced CMakeLists.txt - Add to your existing file
cmake_minimum_required(VERSION 3.18)

# Find CGAL package with required components
find_package(CGAL REQUIRED COMPONENTS 
    Core 
    Mesh_2 
    Mesh_3 
    Polyhedron 
    Surface_mesh
    Triangulation_2
    Triangulation_3
)

# Set CGAL-specific flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CGAL_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CGAL_C_FLAGS}")

# Include CGAL directories
include_directories(${CGAL_INCLUDE_DIRS})

# Define CGAL-enabled compilation
add_definitions(-DUSE_CGAL)

# Create CGAL mesh library
set(CGAL_MESH_SOURCES
    src/mesh/cgal_mesh_engine.cpp
    src/mesh/cgal_surface_mesh.cpp
    src/mesh/cgal_volume_mesh.cpp
    src/mesh/cgal_quality_assessment.cpp
    src/mesh/cgal_optimization.cpp
    src/mesh/cgal_electromagnetic_adaptation.cpp
)

add_library(cgal_mesh SHARED ${CGAL_MESH_SOURCES})
target_link_libraries(cgal_mesh 
    PUBLIC 
        ${CGAL_LIBRARIES}
        electromagnetic_kernel_library
        OpenMP::OpenMP_CXX
)

# Set C++ standard for CGAL
target_compile_features(cgal_mesh PRIVATE cxx_std_17)

# Platform-specific CGAL optimizations
if(WIN32)
    target_compile_definitions(cgal_mesh PRIVATE CGAL_USE_GMPXX)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(cgal_mesh PUBLIC pthread)
endif()

# Installation rules for CGAL mesh library
install(TARGETS cgal_mesh
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Export CGAL mesh targets
export(TARGETS cgal_mesh FILE CGALMeshTargets.cmake)

# Create CGAL-enabled executables
add_executable(pulseem_cgal_demo apps/pulseem_cgal_demo.cpp)
target_link_libraries(pulseem_cgal_demo 
    PUBLIC 
        cgal_mesh
        mom_solver
        peec_solver
)

# CGAL-specific test suite
if(BUILD_TESTS)
    enable_testing()
    
    add_executable(cgal_mesh_tests tests/cgal_mesh_tests.cpp)
    target_link_libraries(cgal_mesh_tests 
        PUBLIC 
            cgal_mesh
            gtest
            gtest_main
    )
    
    add_test(NAME CGALMeshTests COMMAND cgal_mesh_tests)
endif()

# CGAL performance benchmarks
if(BUILD_BENCHMARKS)
    add_executable(cgal_mesh_benchmarks benchmarks/cgal_mesh_benchmarks.cpp)
    target_link_libraries(cgal_mesh_benchmarks 
        PUBLIC 
            cgal_mesh
            benchmark
    )
endif()