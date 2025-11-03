#!/usr/bin/env python3
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(__file__))
RUBOLT = os.path.join(ROOT, 'src', 'rubolt.exe' if os.name == 'nt' else 'rubolt')
TEST_DIR = os.path.join(ROOT, 'Lib', 'test')

if not os.path.exists(RUBOLT):
    print('Rubolt binary not found. Build first.', file=sys.stderr)
    sys.exit(1)

fails = 0
for root, _, files in os.walk(TEST_DIR):
    for f in files:
        if f.endswith('.rbo'):
            path = os.path.join(root, f)
            print(f'Running {os.path.relpath(path, ROOT)}...')
            res = subprocess.run([RUBOLT, path])
            if res.returncode != 0:
                print(f'✗ Failed: {f}', file=sys.stderr)
                fails += 1

if fails:
    print(f'Failed tests: {fails}')
    sys.exit(1)
print('✓ All tests passed!')
