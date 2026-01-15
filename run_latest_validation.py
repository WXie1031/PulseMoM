#!/usr/bin/env python3
"""
Simple script to run latest computational libraries validation
"""

import sys
import os

# Add the src directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

from tests.latest_libraries_validation import LatestLibrariesValidationSuite

def main():
    """Run validation and print results"""
    print("=" * 80)
    print("Latest Computational Libraries Validation Suite")
    print("=" * 80)
    
    # Create validation suite and run tests
    suite = LatestLibrariesValidationSuite()
    results = suite.run_all_tests()
    
    # Print summary
    print("\n" + "=" * 80)
    print("VALIDATION SUMMARY")
    print("=" * 80)
    
    summary = results['validation_summary']
    print(f"Overall Status: {summary['overall_status']}")
    print(f"Success Rate: {summary['success_rate']:.1%}")
    print(f"Tests Passed: {summary['passed_tests']}/{summary['total_tests']}")
    
    if results['recommendations']:
        print("\nRECOMMENDATIONS:")
        for i, rec in enumerate(results['recommendations'], 1):
            print(f"{i}. {rec}")
    
    print("\n" + "=" * 80)
    
    return results

if __name__ == "__main__":
    main()