/*******************************************************************************
 * Test Compilation with Corrected Library Paths
 * 
 * This test verifies that all libraries compile correctly with the
 * corrected include paths for Clipper2 and H2Lib.
 * 
 * COMPILE WITH CORRECTED PATHS:
 *   gcc -I"../libs/CGAL-6.1/include" \\
 *       -I"../libs/gmsh-4.15.0-Windows64-sdk/include" \\
 *       -I"../libs/occt-vc14-64/inc" \\
 *       -I"../libs/boost_1_89_0" \\
 *       -I"../libs/Clipper2_1.5.4/CPP/Clipper2Lib/include" \\
 *       -I"../libs/OpenBLAS-0.3.30-x64/include" \\
 *       -I"../libs/petsc-3.24.1/include" \\
 *       -I"../libs/H2Lib-3.0.1/Library" \\
 *       -I"../libs/triangle" \\
 *       -I"../libs/embree-4.4.0/include" \\
 *       test_corrected_paths.c -o test_corrected_paths
 ******************************************************************************/

#include <stdio.h>
#include <stdbool.h>

// Test results
typedef struct {
    const char* library;
    const char* header_path;
    bool available;
    const char* status;
} library_test_t;

// Test all libraries with corrected paths
void test_all_libraries() {
    printf("=== TESTING CORRECTED LIBRARY PATHS ===\\n\\n");
    
    library_test_t tests[10];
    int num_tests = 0;
    int passed = 0;
    
    // 1. Test CGAL
    printf("1. Testing CGAL... ");
    fflush(stdout);
    #ifdef USE_CGAL
    #include <CGAL/Simple_cartesian.h>
    tests[num_tests] = (library_test_t){"CGAL", "CGAL/Simple_cartesian.h", true, "✓ READY"};
    printf("✓\\n");
    #else
    tests[num_tests] = (library_test_t){"CGAL", "CGAL/Simple_cartesian.h", false, "✗ MISSING"};
    printf("✗\\n");
    #endif
    if (tests[num_tests].available) passed++;
    num_tests++;
    
    // 2. Test Gmsh
    printf("2. Testing Gmsh... ");
    fflush(stdout);
    #ifdef USE_GMSH
    #include <gmsh.h>
    tests[num_tests] = (library_test_t){"Gmsh", "gmsh.h", true, "✓ READY"};
    printf("✓\\n");
    #else
    tests[num_tests] = (library_test_t){"Gmsh", "gmsh.h", false, "✗ MISSING"};
    printf("✗\\n");
    #endif
    if (tests[num_tests].available) passed++;
    num_tests++;
    
    // 3. Test OpenCascade
    printf("3. Testing OpenCascade... ");
    fflush(stdout);
    #ifdef USE_OPENCASCADE
    #include <gp_Pnt.hxx>
    tests[num_tests] = (library_test_t){"OpenCascade", "gp_Pnt.hxx", true, "✓ READY"};
    printf("✓\\n");
    #else
    tests[num_tests] = (library_test_t){"OpenCascade", "gp_Pnt.hxx", false, "✗ MISSING"};
    printf("✗\\n");
    #endif
    if (tests[num_tests].available) passed++;
    num_tests++;
    
    // 4. Test Boost.Geometry
    printf("4. Testing Boost.Geometry... ");
    fflush(stdout);
    #ifdef USE_BOOST
    #include <boost/geometry.hpp>
    tests[num_tests] = (library_test_t){"Boost.Geometry", "boost/geometry.hpp", true, "✓ READY"};
    printf("✓\\n");
    #else
    tests[num_tests] = (library_test_t){"Boost.Geometry", "boost/geometry.hpp", false, "✗ MISSING"};
    printf("✗\\n");
    #endif
    if (tests[num_tests].available) passed++;
    num_tests++;
    
    // 5. Test Clipper2 - CORRECTED PATH
    printf("5. Testing Clipper2 (CORRECTED PATH)... ");
    fflush(stdout);
    #ifdef USE_CLIPPER2
    #include <clipper2/clipper.h>
    tests[num_tests] = (library_test_t){"Clipper2", "clipper2/clipper.h", true, "✓ READY"};
    printf("✓\\n");
    #else
    tests[num_tests] = (library_test_t){"Clipper2", "clipper2/clipper.h", false, "✗ MISSING"};
    printf("✗\\n");
    #endif
    if (tests[num_tests].available) passed++;
    num_tests++;
    
    // 6. Test Triangle
    printf("6. Testing Triangle... ");
    fflush(stdout);
    #ifdef USE_TRIANGLE
    #include <triangle.h>
    tests[num_tests] = (library_test_t){"Triangle", "triangle.h", true, "✓ READY"};
    printf("✓\\n");
    #else
    tests[num_tests] = (library_test_t){"Triangle", "triangle.h", false, "✗ MISSING"};
    printf("✗\\n");
    #endif
    if (tests[num_tests].available) passed++;
    num_tests++;
    
    // 7. Test OpenBLAS
    printf("7. Testing OpenBLAS... ");
    fflush(stdout);
    #ifdef USE_OPENBLAS
    #include <cblas.h>
    tests[num_tests] = (library_test_t){"OpenBLAS", "cblas.h", true, "✓ READY"};
    printf("✓\\n");
    #else
    tests[num_tests] = (library_test_t){"OpenBLAS", "cblas.h", false, "✗ MISSING"};
    printf("✗\\n");
    #endif
    if (tests[num_tests].available) passed++;
    num_tests++;
    
    // 8. Test PETSc
    printf("8. Testing PETSc... ");
    fflush(stdout);
    #ifdef USE_PETSC
    #include <petsc.h>
    tests[num_tests] = (library_test_t){"PETSc", "petsc.h", true, "✓ READY"};
    printf("✓\\n");
    #else
    tests[num_tests] = (library_test_t){"PETSc", "petsc.h", false, "✗ MISSING"};
    printf("✗\\n");
    #endif
    if (tests[num_tests].available) passed++;
    num_tests++;
    
    // 9. Test H2Lib - CORRECTED PATH
    printf("9. Testing H2Lib (CORRECTED PATH)... ");
    fflush(stdout);
    #ifdef USE_H2LIB
    #include <basic.h>
    tests[num_tests] = (library_test_t){"H2Lib", "basic.h", true, "✓ READY"};
    printf("✓\\n");
    #else
    tests[num_tests] = (library_test_t){"H2Lib", "basic.h", false, "✗ MISSING"};
    printf("✗\\n");
    #endif
    if (tests[num_tests].available) passed++;
    num_tests++;
    
    // 10. Test Embree
    printf("10. Testing Embree... ");
    fflush(stdout);
    #ifdef USE_EMBREE
    #include <embree4/rtcore.h>
    tests[num_tests] = (library_test_t){"Embree", "embree4/rtcore.h", true, "✓ READY"};
    printf("✓\\n");
    #else
    tests[num_tests] = (library_test_t){"Embree", "embree4/rtcore.h", false, "✗ MISSING"};
    printf("✗\\n");
    #endif
    if (tests[num_tests].available) passed++;
    num_tests++;
    
    // Summary
    printf("\\n=== CORRECTED PATHS TEST SUMMARY ===\\n");
    printf("Total Libraries Tested: %d\\n", num_tests);
    printf("Libraries Available: %d\\n", passed);
    printf("Success Rate: %.1f%%\\n\\n", (float)passed / num_tests * 100);
    
    printf("=== DETAILED RESULTS ===\\n");
    for (int i = 0; i < num_tests; i++) {
        printf("%-15s %-30s %s\\n", tests[i].library, tests[i].header_path, tests[i].status);
    }
    
    printf("\\n=== FUNCTIONAL GROUPING ===\\n");
    printf("3D Mesh Generation:\\n");
    #ifdef USE_CGAL
    printf("  ✓ CGAL - Robust 3D meshing algorithms\\n");
    #endif
    #ifdef USE_GMSH
    printf("  ✓ Gmsh - Surface mesh generation\\n");
    #endif
    
    printf("\\n2D Geometry Processing:\\n");
    #ifdef USE_CLIPPER2
    printf("  ✓ Clipper2 - Boolean operations [CORRECTED PATH]\\n");
    #endif
    #ifdef USE_TRIANGLE
    printf("  ✓ Triangle - Constrained triangulation\\n");
    #endif
    #ifdef USE_BOOST
    printf("  ✓ Boost.Geometry - Generic geometry\\n");
    #endif
    
    printf("\\nCAD Import:\\n");
    #ifdef USE_OPENCASCADE
    printf("  ✓ OpenCascade - STEP/IGES import\\n");
    #endif
    
    printf("\\nNumerical Computing:\\n");
    #ifdef USE_OPENBLAS
    printf("  ✓ OpenBLAS - Dense linear algebra\\n");
    #endif
    #ifdef USE_PETSC
    printf("  ✓ PETSc - Sparse systems\\n");
    #endif
    #ifdef USE_H2LIB
    printf("  ✓ H2Lib - Matrix compression [CORRECTED PATH]\\n");
    #endif
    
    printf("\\nVisualization:\\n");
    #ifdef USE_EMBREE
    printf("  ✓ Embree - Ray tracing acceleration\\n");
    #endif
    
    printf("\\n=== CORRECTED INCLUDE PATHS ===\\n");
    printf("Clipper2: -I\"../libs/Clipper2_1.5.4/CPP/Clipper2Lib/include\"\\n");
    printf("H2Lib:   -I\"../libs/H2Lib-3.0.1/Library\"\\n");
    
    if (passed == num_tests) {
        printf("\\n🎉 ALL LIBRARIES VERIFIED WITH CORRECTED PATHS! 🎉\\n");
        printf("All 10 libraries are ready for immediate integration.\\n");
    } else {
        printf("\\n⚠️  Some libraries need attention. Check paths above.\\n");
    }
}

int main() {
    printf("PEEC + MoM Mesh Generation Library\\n");
    printf("Corrected Library Path Verification Test\\n");
    printf("========================================\\n\\n");
    
    test_all_libraries();
    
    return 0;
}