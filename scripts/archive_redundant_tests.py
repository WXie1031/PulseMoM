#!/usr/bin/env python3
"""Archive redundant test files"""

import shutil
from pathlib import Path

tests_dir = Path(__file__).parent.parent / 'tests'
archive_dir = tests_dir / 'archive'
archive_dir.mkdir(exist_ok=True)

files_to_archive = [
    'enhanced_advanced_benchmark.py',
    'production_advanced_benchmark.py',
    'simplified_latest_libraries_validation.py',
    'simplified_validation_test.py',
]

for filename in files_to_archive:
    src = tests_dir / filename
    if src.exists():
        dst = archive_dir / filename
        shutil.move(str(src), str(dst))
        print(f'Moved {filename} to archive')
