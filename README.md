# Transactional & Concurrent Cuckoo Hash Set

This repository implements and benchmarks multiple variants of a cuckoo hash **set** with different concurrency strategies. It compares **sequential** baselines, **striped-lock** concurrent versions, and a **transactional (STM)** approach, and includes a reproducible benchmarking + plotting pipeline.

## Results at a glance

<p align="center">
  <img src="results_100000x10000.png" alt="Throughput (ops/sec) vs. thread count for all variants" width="560">
</p>
<p align="center">
  <img src="results_10000x1000" alt="Scalability across bucket sizes and operation counts" width="560">
</p>

*Reproduce:* see **Reproduce in 60s** below.

---

## Implementations

- **Sequential v1/v2** — Baseline single-threaded variants (micro-optimizations differ).  
- **Concurrent v1** — Coarse/striped locking; simple correctness, moderate contention.  
- **Concurrent v2** — Optimized striped locking; better cache behavior and early-exit on misses.  
- **Transactional (STM)** — Lock-free from the user’s point of view; retries on conflicts.

Each variant exposes set-style operations (e.g., `insert`, `contains`, `erase`) and is compiled into a separate executable.

## Reproduce in 60s

```bash
# 1) Build all variants
make

# 2) Run the benchmark suite (writes CSVs to ./results/ by default)
python3 benchmark.py

# 3) Generate figures (writes PNGs used above)
# If your plot script supports custom outputs:
python3 plot.py   --out-threads docs/plots/throughput_vs_threads.png   --out-buckets docs/plots/scalability_by_buckets.png

# Otherwise (no flags), just run:
# python3 plot.py
# and copy or rename the produced figures into docs/plots/ as referenced above.
```

> **Notes**
> - Ensure your toolchain supports threads: compile flags typically include `-pthread` (already set in the Makefile).  
> - If your transactional build requires a specific STM library or flag, install it first or adjust the Makefile accordingly.


## Benchmarking

The benchmark sweeps across **operation counts**, **bucket counts / table sizes**, and **thread counts**, measuring **throughput (ops/sec)**. CSVs are written to `./results/` and consumed by `plot.py` to generate the two figures above.

Typical command-line options (check `benchmark.py -h` in your repo for the exact set):

```
--ops 100000      # operations per trial
--threads 1..16   # number of worker threads
--buckets 1000    # bucket count / initial capacity
--trials 5        # repeat & average
```

## Highlights

- **Low thread counts:** the **optimized sequential** version is often fastest (no sync overhead).  
- **High thread counts:** **concurrent v2 (striped locks + atomics/early-exit)** scales best as contention rises.  
- **Transactional (STM):** correctness-friendly, but retries under contention reduce throughput (expect slower curves at higher threads).  
- **Unoptimized concurrent:** generally slowest due to coarse locking and cache contention.

## Repository structure

```
.
├── src/                      # C++ sources for each variant
├── Makefile                  # build all variants into separate executables
├── benchmark.py              # runs parameter sweeps, writes CSV to ./results/
├── plot.py                   # reads CSV and produces figures
├── results/                  # CSV output (gitignored if large)
└── docs/
    └── plots/                # PNGs embedded in this README
```

## Tips
- Keep `docs/plots/` images committed so reviewers see results without running anything.  
- Include your exact **CPU model** and **compiler version** at the top of the README if you report specific numbers.  
- For reproducibility, export `OMP_NUM_THREADS` where appropriate and pin CPU frequency if you care about microbench stability.

---

If you use a different CLI for `plot.py`, adjust the `--out-*` names or drop the flags entirely. If you want, share your current `plot.py -h` and I’ll tailor this README’s commands exactly to your script.
