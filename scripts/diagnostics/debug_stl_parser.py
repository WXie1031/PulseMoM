#!/usr/bin/env python3
"""
Debug STL parser to understand the parsing issue
"""

import os

def debug_stl_parsing():
    """Debug the STL parsing process"""
    
    stl_file = 'tests/test_hpm/weixing_v1.stl'
    
    print(f"Debugging STL file: {stl_file}")
    print(f"File exists: {os.path.exists(stl_file)}")
    
    if not os.path.exists(stl_file):
        return
    
    with open(stl_file, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    lines = content.strip().split('\n')
    print(f"Total lines: {len(lines)}")
    
    # Count different line types
    facet_count = 0
    vertex_count = 0
    endfacet_count = 0
    
    for i, line in enumerate(lines[:100]):  # Check first 100 lines
        line = line.strip()
        if line.startswith('facet normal'):
            facet_count += 1
            print(f"Line {i+1}: FACET - {line}")
        elif line.startswith('vertex'):
            vertex_count += 1
            parts = line.split()
            print(f"Line {i+1}: VERTEX - {parts[1:4] if len(parts) >= 4 else 'INVALID'}")
        elif line.startswith('endfacet'):
            endfacet_count += 1
            print(f"Line {i+1}: ENDFACET")
    
    print(f"\nFirst 100 lines summary:")
    print(f"  Facet lines: {facet_count}")
    print(f"  Vertex lines: {vertex_count}")
    print(f"  Endfacet lines: {endfacet_count}")

if __name__ == "__main__":
    debug_stl_parsing()