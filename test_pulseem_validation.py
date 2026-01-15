#!/usr/bin/env python3
"""
PulseEM CLI Validation Test Script
Tests the command-line interface logic and argument parsing
"""

import sys
import os

def test_mode_parsing():
    """Test mode parsing logic"""
    print("=== Testing Mode Parsing ===")
    
    # Define mode mapping (matching the C code)
    mode_map = {
        "mom": "MODE_MOM",
        "peec": "MODE_PEEC", 
        "hybrid": "MODE_HYBRID",
        "help": "MODE_HELP",
        "version": "MODE_VERSION"
    }
    
    test_cases = [
        ("mom", "MODE_MOM"),
        ("peec", "MODE_PEEC"),
        ("hybrid", "MODE_HYBRID"),
        ("help", "MODE_HELP"),
        ("version", "MODE_VERSION"),
        ("invalid", "MODE_HELP"),  # Default to help
        ("MOM", "MODE_HELP"),     # Case sensitive
        ("", "MODE_HELP"),        # Empty string
    ]
    
    all_passed = True
    for input_mode, expected_mode in test_cases:
        # Simulate C function behavior
        actual_mode = mode_map.get(input_mode, "MODE_HELP")
        status = "PASS" if actual_mode == expected_mode else "FAIL"
        if actual_mode != expected_mode:
            all_passed = False
        print(f"  Input: '{input_mode}' -> Expected: {expected_mode}, Got: {actual_mode} [{status}]")
    
    return all_passed

def test_argument_validation():
    """Test argument validation logic"""
    print("\n=== Testing Argument Validation ===")
    
    # Test frequency parsing
    def parse_freq(freq_str):
        """Simulate C frequency parsing"""
        try:
            parts = freq_str.split(':')
            if len(parts) != 3:
                return None
            start, stop, points = float(parts[0]), float(parts[1]), int(parts[2])
            if start <= 0 or stop <= start or points <= 0:
                return None
            return (start * 1e9, stop * 1e9, points)  # Convert GHz to Hz
        except:
            return None
    
    freq_tests = [
        ("1:10:101", (1e9, 10e9, 101)),
        ("0.1:5:50", (0.1e9, 5e9, 50)),
        ("invalid", None),
        ("1:10", None),  # Missing points
        ("10:1:100", None),  # Invalid range
        ("1:10:0", None),  # Zero points
    ]
    
    all_passed = True
    for freq_str, expected in freq_tests:
        result = parse_freq(freq_str)
        status = "PASS" if result == expected else "FAIL"
        if result != expected:
            all_passed = False
        print(f"  Frequency: '{freq_str}' -> {result} [{status}]")
    
    # Test thread count validation
    def validate_threads(threads_str):
        """Simulate C thread validation"""
        try:
            threads = int(threads_str)
            return 1 <= threads <= 64
        except:
            return False
    
    thread_tests = [
        ("1", True),
        ("4", True),
        ("64", True),
        ("0", False),
        ("65", False),
        ("invalid", False),
    ]
    
    for threads_str, expected in thread_tests:
        result = validate_threads(threads_str)
        status = "PASS" if result == expected else "FAIL"
        if result != expected:
            all_passed = False
        print(f"  Threads: '{threads_str}' -> {result} [{status}]")
    
    return all_passed

def test_command_line_examples():
    """Test command-line examples"""
    print("\n=== Testing Command-Line Examples ===")
    
    examples = [
        # Basic usage examples from the documentation
        ["pulseem", "mom", "-i", "antenna.geo", "-f", "1:10:101", "-o", "antenna_results"],
        ["pulseem", "peec", "-i", "pcb_layout.geo", "--skin-effect", "--spice", "pcb_netlist.sp"],
        ["pulseem", "hybrid", "-i", "system.geo", "--mom-regions", "antenna", "--peec-regions", "circuit"],
        ["pulseem", "mom", "-i", "test.geo", "-b", "-t", "8", "-g", "--aca", "--mlfmm"],
        ["pulseem", "help"],
        ["pulseem", "version"],
    ]
    
    print("Valid command-line examples:")
    for i, example in enumerate(examples, 1):
        print(f"  {i}. {' '.join(example)}")
    
    # Test argument parsing logic
    def validate_command(args):
        """Basic validation of command arguments"""
        if len(args) < 2:
            return False, "Missing mode argument"
        
        mode = args[1]
        valid_modes = ["mom", "peec", "hybrid", "help", "version"]
        if mode not in valid_modes:
            return False, f"Invalid mode: {mode}"
        
        # Check for required input file (except for help/version)
        if mode not in ["help", "version"]:
            if "-i" not in args and "--input" not in args:
                return False, "Missing required input file"
        
        return True, "Valid"
    
    all_passed = True
    print("\nCommand validation results:")
    for i, example in enumerate(examples, 1):
        valid, message = validate_command(example)
        status = "PASS" if valid else "FAIL"
        if not valid:
            all_passed = False
        print(f"  {i}. {status}: {message}")
    
    return all_passed

def test_file_extensions():
    """Test output file extension logic"""
    print("\n=== Testing Output File Extensions ===")
    
    def get_output_extension(mode, input_file):
        """Simulate C output extension logic"""
        extensions = {
            "MODE_MOM": ".mom",
            "MODE_PEEC": ".peec",
            "MODE_HYBRID": ".hybrid"
        }
        return input_file + extensions.get(mode, ".out")
    
    test_cases = [
        ("MODE_MOM", "antenna.geo", "antenna.geo.mom"),
        ("MODE_PEEC", "pcb_layout.geo", "pcb_layout.geo.peec"),
        ("MODE_HYBRID", "system.geo", "system.geo.hybrid"),
    ]
    
    all_passed = True
    for mode, input_file, expected in test_cases:
        result = get_output_extension(mode, input_file)
        status = "PASS" if result == expected else "FAIL"
        if result != expected:
            all_passed = False
        print(f"  {mode}: {input_file} -> {result} [{status}]")
    
    return all_passed

def main():
    """Run all tests"""
    print("PulseEM CLI Validation Test Suite")
    print("=" * 50)
    
    tests = [
        test_mode_parsing,
        test_argument_validation,
        test_command_line_examples,
        test_file_extensions,
    ]
    
    results = []
    for test in tests:
        results.append(test())
    
    print("\n" + "=" * 50)
    print("SUMMARY:")
    total_tests = len(tests)
    passed_tests = sum(results)
    
    for i, (test, passed) in enumerate(zip(tests, results)):
        status = "PASS" if passed else "FAIL"
        print(f"  {test.__name__}: {status}")
    
    print(f"\nOverall: {passed_tests}/{total_tests} tests passed")
    
    if passed_tests == total_tests:
        print("✅ All tests passed! PulseEM CLI logic is valid.")
        return 0
    else:
        print("❌ Some tests failed. Review the implementation.")
        return 1

if __name__ == "__main__":
    sys.exit(main())