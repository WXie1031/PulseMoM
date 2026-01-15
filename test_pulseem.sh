#!/bin/bash
# PulseEM Unified Executable Test Script
# Tests all modes and functionality of the pulseem executable

echo "==================================="
echo "PulseEM Unified Executable Test Suite"
echo "==================================="

# Configuration
PULSEEM_BIN="./bin/pulseem"
TEST_DIR="test_data"
RESULTS_DIR="test_results"

# Create test directories
mkdir -p $TEST_DIR $RESULTS_DIR

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Function to run a test
run_test() {
    local test_name="$1"
    local test_cmd="$2"
    local expected_result="$3"
    
    echo ""
    echo "Testing: $test_name"
    echo "Command: $test_cmd"
    
    if eval "$test_cmd"; then
        if [ "$expected_result" = "pass" ]; then
            echo "✅ PASS: $test_name"
            ((TESTS_PASSED++))
        else
            echo "❌ FAIL: $test_name (expected failure but passed)"
            ((TESTS_FAILED++))
        fi
    else
        if [ "$expected_result" = "fail" ]; then
            echo "✅ PASS: $test_name (expected failure)"
            ((TESTS_PASSED++))
        else
            echo "❌ FAIL: $test_name"
            ((TESTS_FAILED++))
        fi
    fi
}

# Create test geometry files
create_test_geometry() {
    echo "Creating test geometry files..."
    
    # Simple antenna geometry
    cat > $TEST_DIR/antenna.geo << 'EOF'
// Simple dipole antenna geometry
Point(1) = {0, 0, 0, 0.001};
Point(2) = {0, 0.05, 0, 0.001};
Point(3) = {0, -0.05, 0, 0.001};
Line(1) = {1, 2};
Line(2) = {1, 3};
Physical Line("antenna") = {1, 2};
EOF

    # PCB structure
    cat > $TEST_DIR/pcb.geo << 'EOF'
// PCB microstrip structure
Point(1) = {0, 0, 0, 0.001};
Point(2) = {0.1, 0, 0, 0.001};
Point(3) = {0.1, 0.005, 0, 0.001};
Point(4) = {0, 0.005, 0, 0.001};
Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};
Line Loop(1) = {1, 2, 3, 4};
Plane Surface(1) = {1};
Physical Surface("microstrip") = {1};
EOF

    # Hybrid geometry
    cat > $TEST_DIR/hybrid.geo << 'EOF'
// Combined antenna + PCB geometry
Include "antenna.geo";
Include "pcb.geo";
Physical Surface("antenna_region") = {1};
Physical Surface("pcb_region") = {2};
EOF
}

# Create test if executable exists
if [ ! -f "$PULSEEM_BIN" ]; then
    echo "❌ Error: pulseem executable not found at $PULSEEM_BIN"
    echo "Please build the project first with: make pulseem"
    exit 1
fi

# Create test geometry
create_test_geometry

echo ""
echo "==================================="
echo "Starting PulseEM Tests"
echo "==================================="

# Test 1: Version information
run_test "Version Information" \
    "$PULSEEM_BIN version" \
    "pass"

# Test 2: Help system
run_test "Help System" \
    "$PULSEEM_BIN help" \
    "pass"

# Test 3: Invalid mode (should fail)
run_test "Invalid Mode" \
    "$PULSEEM_BIN invalid_mode -i $TEST_DIR/antenna.geo" \
    "fail"

# Test 4: Missing input file (should fail)
run_test "Missing Input File" \
    "$PULSEEM_BIN mom" \
    "fail"

# Test 5: MoM mode basic execution
run_test "MoM Basic Execution" \
    "$PULSEEM_BIN mom -i $TEST_DIR/antenna.geo -f 1:2:3 -o $RESULTS_DIR/mom_test" \
    "pass"

# Test 6: PEEC mode basic execution
run_test "PEEC Basic Execution" \
    "$PULSEEM_BIN peec -i $TEST_DIR/pcb.geo -f 0.1:1:3 -o $RESULTS_DIR/peec_test" \
    "pass"

# Test 7: Hybrid mode basic execution
run_test "Hybrid Basic Execution" \
    "$PULSEEM_BIN hybrid -i $TEST_DIR/hybrid.geo -f 1:5:3 -o $RESULTS_DIR/hybrid_test" \
    "pass"

# Test 8: MoM with advanced options
run_test "MoM with Advanced Options" \
    "$PULSEEM_BIN mom -i $TEST_DIR/antenna.geo -f 1:10:11 --aca --mlfmm -t 2 -b" \
    "pass"

# Test 9: PEEC with SPICE export
run_test "PEEC with SPICE Export" \
    "$PULSEEM_BIN peec -i $TEST_DIR/pcb.geo -f 1:5:6 --skin-effect --spice $RESULTS_DIR/test.sp" \
    "pass"

# Test 10: Hybrid with coupling method
run_test "Hybrid with Coupling Method" \
    "$PULSEEM_BIN hybrid -i $TEST_DIR/hybrid.geo -f 1:3:4 --hybrid-method schur --hybrid-max-iter 50" \
    "pass"

# Test 11: Thread configuration
run_test "Thread Configuration" \
    "$PULSEEM_BIN mom -i $TEST_DIR/antenna.geo -f 1:2:3 -t 8" \
    "pass"

# Test 12: Verbosity levels
run_test "Verbosity Level 0" \
    "$PULSEEM_BIN mom -i $TEST_DIR/antenna.geo -f 1:2:3 -v 0" \
    "pass"

run_test "Verbosity Level 3" \
    "$PULSEEM_BIN mom -i $TEST_DIR/antenna.geo -f 1:2:3 -v 3" \
    "pass"

echo ""
echo "==================================="
echo "Test Summary"
echo "==================================="
echo "Tests Passed: $TESTS_PASSED"
echo "Tests Failed: $TESTS_FAILED"
echo "Total Tests: $((TESTS_PASSED + TESTS_FAILED))"

if [ $TESTS_FAILED -eq 0 ]; then
    echo "✅ All tests passed!"
    exit 0
else
    echo "❌ Some tests failed!"
    exit 1
fi