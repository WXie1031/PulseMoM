#!/usr/bin/env python3
"""
Architecture Validation Script

This script validates that all migrated code follows the six-layer architecture
defined in ARCHITECTURE_GUARD.md.

Checks:
1. Files belong to exactly one layer
2. No cross-layer dependencies
3. No forbidden patterns
4. Include paths are correct
"""

import os
import re
import sys
from pathlib import Path

# Layer definitions
LAYERS = {
    'L1_physics': {
        'path': 'src/L1_physics',
        'allowed_includes': ['common', 'L1_physics'],
        'forbidden_includes': ['L2_', 'L3_', 'L4_', 'L5_', 'L6_', 'core', 'solvers']
    },
    'L2_discretization': {
        'path': 'src/L2_discretization',
        'allowed_includes': ['common', 'L1_physics', 'L2_discretization'],
        'forbidden_includes': ['L3_', 'L4_', 'L5_', 'L6_', 'core', 'solvers']
    },
    'L3_operators': {
        'path': 'src/L3_operators',
        'allowed_includes': ['common', 'L1_physics', 'L2_discretization', 'L3_operators'],
        'forbidden_includes': ['L4_', 'L5_', 'L6_', 'core', 'solvers']
    },
    'L4_backend': {
        'path': 'src/L4_backend',
        'allowed_includes': ['common', 'L3_operators', 'L4_backend'],
        'forbidden_includes': ['L1_', 'L2_', 'L5_', 'L6_', 'core', 'solvers']
    },
    'L5_orchestration': {
        'path': 'src/L5_orchestration',
        'allowed_includes': ['common', 'L1_physics', 'L3_operators', 'L4_backend', 'L5_orchestration'],
        'forbidden_includes': ['L2_', 'L6_', 'core', 'solvers']
    },
    'L6_io': {
        'path': 'src/L6_io',
        'allowed_includes': ['common', 'L5_orchestration', 'L6_io'],
        'forbidden_includes': ['L1_', 'L2_', 'L3_', 'L4_', 'core', 'solvers']
    },
    'common': {
        'path': 'src/common',
        'allowed_includes': ['common'],
        'forbidden_includes': ['L1_', 'L2_', 'L3_', 'L4_', 'L5_', 'L6_', 'core', 'solvers']
    }
}

# Forbidden patterns (from ARCHITECTURE_GUARD.md)
FORBIDDEN_PATTERNS = [
    (r'#include\s+["<].*core.*solver', 'L1/L2/L3 should not include solver'),
    (r'#include\s+["<].*gpu.*acceleration', 'L1/L2/L3 should not include GPU directly'),
    (r'mom_solver_|peec_solver_|mtl_solver_', 'Physics layer should not call solver functions'),
    (r'cudaLaunchKernel|cudaMemcpy', 'L3 should not contain CUDA calls'),
]

def get_file_layer(file_path):
    """Determine which layer a file belongs to."""
    for layer_name, layer_info in LAYERS.items():
        if layer_info['path'] in file_path:
            return layer_name
    return None

def check_includes(file_path, content):
    """Check if includes violate layer boundaries."""
    errors = []
    layer = get_file_layer(file_path)
    
    if not layer:
        return errors
    
    layer_info = LAYERS[layer]
    
    # Find all includes
    include_pattern = r'#include\s+["<]([^">]+)[">]'
    includes = re.findall(include_pattern, content)
    
    for include in includes:
        # Check forbidden includes
        for forbidden in layer_info['forbidden_includes']:
            if forbidden in include:
                errors.append(f"Forbidden include '{include}' in {layer} layer")
        
        # Check if include is from wrong layer
        for other_layer, other_info in LAYERS.items():
            if other_layer == layer:
                continue
            if other_info['path'] in include and other_layer not in layer_info['allowed_includes']:
                errors.append(f"Cross-layer include '{include}' from {layer} to {other_layer}")
    
    return errors

def check_forbidden_patterns(file_path, content):
    """Check for forbidden patterns."""
    errors = []
    
    for pattern, description in FORBIDDEN_PATTERNS:
        matches = re.finditer(pattern, content, re.IGNORECASE)
        for match in matches:
            line_num = content[:match.start()].count('\n') + 1
            errors.append(f"Forbidden pattern at line {line_num}: {description}")
    
    return errors

def validate_file(file_path):
    """Validate a single file."""
    errors = []
    
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # Check includes
        errors.extend(check_includes(file_path, content))
        
        # Check forbidden patterns
        errors.extend(check_forbidden_patterns(file_path, content))
        
    except Exception as e:
        errors.append(f"Error reading file: {e}")
    
    return errors

def validate_architecture(root_dir='src'):
    """Validate all files in the new architecture."""
    root = Path(root_dir)
    all_errors = []
    files_checked = 0
    
    # Check all .c, .h, .cu files in L1-L6 directories
    for layer_name in LAYERS.keys():
        layer_path = root / LAYERS[layer_name]['path']
        if not layer_path.exists():
            continue
        
        for file_path in layer_path.rglob('*'):
            if file_path.suffix in ['.c', '.h', '.cu', '.cpp']:
                files_checked += 1
                errors = validate_file(str(file_path))
                if errors:
                    all_errors.append((str(file_path), errors))
    
    return all_errors, files_checked

def main():
    """Main validation function."""
    print("=" * 70)
    print("Architecture Validation")
    print("=" * 70)
    print()
    
    errors, files_checked = validate_architecture()
    
    print(f"Files checked: {files_checked}")
    print(f"Files with errors: {len(errors)}")
    print()
    
    if errors:
        print("ERRORS FOUND:")
        print("-" * 70)
        for file_path, file_errors in errors:
            print(f"\n{file_path}:")
            for error in file_errors:
                print(f"  ❌ {error}")
        print()
        print("=" * 70)
        print("Validation FAILED")
        print("=" * 70)
        return 1
    else:
        print("=" * 70)
        print("Validation PASSED - All files comply with architecture")
        print("=" * 70)
        return 0

if __name__ == '__main__':
    sys.exit(main())
