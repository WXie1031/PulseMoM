#!/usr/bin/env python3
"""
Test Files Cleanup Script

Analyzes tests directory and identifies redundant/duplicate test files.
Provides recommendations for which files to keep and which to remove.
"""

import os
import sys
from pathlib import Path
from typing import Dict, List, Tuple
import hashlib
from collections import defaultdict


def get_file_hash(filepath: str) -> str:
    """Calculate MD5 hash of file"""
    hash_md5 = hashlib.md5()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


def analyze_test_files(tests_dir: Path) -> Dict[str, List[Tuple[str, int, str]]]:
    """
    Analyze test files and group by similarity
    
    Returns:
        Dictionary mapping category to list of (filename, size, hash)
    """
    categories = defaultdict(list)
    
    for py_file in tests_dir.glob("*.py"):
        # Analyze all Python files, not just test_*.py
            size = py_file.stat().st_size
            file_hash = get_file_hash(str(py_file))
            
            # Categorize by name patterns
            name = py_file.name.lower()
            if "benchmark" in name:
                if "advanced" in name:
                    if "clean" in name:
                        categories["advanced_benchmark_clean"].append((py_file.name, size, file_hash))
                    elif "enhanced" in name:
                        categories["advanced_benchmark_enhanced"].append((py_file.name, size, file_hash))
                    elif "production" in name:
                        categories["advanced_benchmark_production"].append((py_file.name, size, file_hash))
                    else:
                        categories["advanced_benchmark"].append((py_file.name, size, file_hash))
                elif "simplified" in name and "production" in name:
                    categories["simplified_production_benchmark"].append((py_file.name, size, file_hash))
                else:
                    categories["benchmark"].append((py_file.name, size, file_hash))
            elif "validation" in name:
                if "simplified" in name:
                    categories["simplified_validation"].append((py_file.name, size, file_hash))
                elif "focused" in name:
                    categories["focused_validation"].append((py_file.name, size, file_hash))
                else:
                    categories["validation"].append((py_file.name, size, file_hash))
            elif "library" in name or "libraries" in name:
                if "simplified" in name:
                    categories["simplified_library_validation"].append((py_file.name, size, file_hash))
                else:
                    categories["library_validation"].append((py_file.name, size, file_hash))
            else:
                categories["other"].append((py_file.name, size, file_hash))
    
    return categories


def identify_redundant_files(categories: Dict[str, List[Tuple[str, int, str]]]) -> Dict[str, List[str]]:
    """
    Identify redundant files based on similarity
    
    Returns:
        Dictionary mapping "keep" to files to keep, "remove" to files to remove
    """
    recommendations = {
        "keep": [],
        "remove": [],
        "review": []  # Files that need manual review
    }
    
    # Advanced benchmark files - keep the most complete one
    benchmark_groups = [
        ("advanced_benchmark_clean", "advanced_benchmark_testing_clean.py"),
        ("advanced_benchmark_enhanced", "enhanced_advanced_benchmark.py"),
        ("advanced_benchmark_production", "production_advanced_benchmark.py"),
    ]
    
    # Find the largest/most complete advanced benchmark
    advanced_benchmarks = []
    for cat, default_name in benchmark_groups:
        if cat in categories:
            for name, size, _ in categories[cat]:
                advanced_benchmarks.append((name, size, cat))
    
    if len(advanced_benchmarks) > 1:
        # Keep the largest one (most complete)
        advanced_benchmarks.sort(key=lambda x: x[1], reverse=True)
        recommendations["keep"].append(advanced_benchmarks[0][0])
        for name, _, _ in advanced_benchmarks[1:]:
            recommendations["remove"].append(name)
    
    # Validation tests - keep focused, remove simplified if similar
    if "focused_validation" in categories and "simplified_validation" in categories:
        # Keep focused (more comprehensive)
        for name, _, _ in categories["focused_validation"]:
            recommendations["keep"].append(name)
        for name, _, _ in categories["simplified_validation"]:
            recommendations["remove"].append(name)
    elif "focused_validation" in categories:
        for name, _, _ in categories["focused_validation"]:
            recommendations["keep"].append(name)
    elif "simplified_validation" in categories:
        for name, _, _ in categories["simplified_validation"]:
            recommendations["keep"].append(name)
    
    # Library validation - keep non-simplified version
    if "library_validation" in categories and "simplified_library_validation" in categories:
        for name, _, _ in categories["library_validation"]:
            recommendations["keep"].append(name)
        for name, _, _ in categories["simplified_library_validation"]:
            recommendations["remove"].append(name)
    elif "library_validation" in categories:
        for name, _, _ in categories["library_validation"]:
            recommendations["keep"].append(name)
    elif "simplified_library_validation" in categories:
        for name, _, _ in categories["simplified_library_validation"]:
            recommendations["keep"].append(name)
    
    # Keep all other test files
    for cat, files in categories.items():
        if cat not in ["advanced_benchmark_clean", "advanced_benchmark_enhanced", 
                      "advanced_benchmark_production", "focused_validation", 
                      "simplified_validation", "library_validation", 
                      "simplified_library_validation"]:
            for name, _, _ in files:
                if name not in recommendations["remove"]:
                    recommendations["keep"].append(name)
    
    return recommendations


def generate_cleanup_report(tests_dir: Path, output_file: str = "test_cleanup_report.md"):
    """Generate cleanup report with recommendations"""
    categories = analyze_test_files(tests_dir)
    recommendations = identify_redundant_files(categories)
    
    report = []
    report.append("# Test Files Cleanup Report\n")
    report.append(f"Generated: {Path(__file__).stat().st_mtime}\n")
    report.append("## Summary\n")
    report.append(f"- Total test files analyzed: {sum(len(files) for files in categories.values())}\n")
    report.append(f"- Files to keep: {len(recommendations['keep'])}\n")
    report.append(f"- Files recommended for removal: {len(recommendations['remove'])}\n")
    report.append("\n## Files to Keep\n")
    
    for name in sorted(recommendations["keep"]):
        report.append(f"- `{name}`\n")
    
    report.append("\n## Files Recommended for Removal\n")
    report.append("These files appear to be redundant or superseded by other versions:\n\n")
    
    for name in sorted(recommendations["remove"]):
        filepath = tests_dir / name
        size_kb = filepath.stat().st_size / 1024
        report.append(f"- `{name}` ({size_kb:.1f} KB)\n")
    
    report.append("\n## File Categories\n")
    for cat, files in sorted(categories.items()):
        report.append(f"\n### {cat}\n")
        for name, size, _ in files:
            size_kb = size / 1024
            status = "KEEP" if name in recommendations["keep"] else "REMOVE" if name in recommendations["remove"] else "REVIEW"
            report.append(f"- `{name}` ({size_kb:.1f} KB) - {status}\n")
    
    report.append("\n## Recommendations\n")
    report.append("1. Review the files marked for removal to ensure they don't contain unique test cases\n")
    report.append("2. Consider consolidating similar benchmark files into a single comprehensive benchmark\n")
    report.append("3. Keep focused validation tests over simplified versions when both exist\n")
    report.append("4. Archive removed files rather than deleting them immediately\n")
    
    report_text = "".join(report)
    
    output_path = tests_dir.parent / output_file
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(report_text)
    
    print(report_text)
    print(f"\nReport saved to: {output_path}")
    
    return recommendations


if __name__ == "__main__":
    project_root = Path(__file__).parent.parent
    tests_dir = project_root / "tests"
    
    if not tests_dir.exists():
        print(f"Tests directory not found: {tests_dir}")
        sys.exit(1)
    
    recommendations = generate_cleanup_report(tests_dir)
    
    # Optionally create a script to move files to archive
    archive_script = tests_dir.parent / "scripts" / "archive_redundant_tests.py"
    with open(archive_script, 'w') as f:
        f.write("#!/usr/bin/env python3\n")
        f.write('"""Archive redundant test files"""\n\n')
        f.write("import shutil\n")
        f.write("from pathlib import Path\n\n")
        f.write("tests_dir = Path(__file__).parent.parent / 'tests'\n")
        f.write("archive_dir = tests_dir / 'archive'\n")
        f.write("archive_dir.mkdir(exist_ok=True)\n\n")
        f.write("files_to_archive = [\n")
        for name in recommendations["remove"]:
            f.write(f"    '{name}',\n")
        f.write("]\n\n")
        f.write("for filename in files_to_archive:\n")
        f.write("    src = tests_dir / filename\n")
        f.write("    if src.exists():\n")
        f.write("        dst = archive_dir / filename\n")
        f.write("        shutil.move(str(src), str(dst))\n")
        f.write("        print(f'Moved {filename} to archive')\n")
    
    print(f"\nArchive script created: {archive_script}")
    print("Run it to move redundant files to tests/archive/")
