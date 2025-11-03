#!/usr/bin/env python3
import os, subprocess, time, statistics

ROOT = os.path.dirname(os.path.dirname(__file__))
RUBOLT = os.path.join(ROOT, 'src', 'rubolt.exe' if os.name == 'nt' else 'rubolt')
BENCH_DIR = os.path.join(ROOT, 'benchmarks')

CASES = [
    ('loop', os.path.join(BENCH_DIR, 'loop.rbo')),
    ('recursion', os.path.join(BENCH_DIR, 'recursion.rbo')),
    ('io', os.path.join(BENCH_DIR, 'io.rbo')),
]

N = 5

results = {}
for name, path in CASES:
    times = []
    for _ in range(N):
        t0 = time.perf_counter()
        subprocess.run([RUBOLT, path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        t1 = time.perf_counter()
        times.append(t1 - t0)
    results[name] = (statistics.mean(times), statistics.stdev(times))

print('Benchmark results (s):')
for name, (mean, stdev) in results.items():
    print(f'{name:10s} mean={mean:.6f} stdev={stdev:.6f}')
