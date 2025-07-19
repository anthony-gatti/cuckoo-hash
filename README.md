# Transactional and Concurrent Cuckoo Hash Set

This project implements and benchmarks several versions of an open-addressed hash set, including both concurrent and transactional variants. The main goal is to evaluate trade-offs in correctness, synchronization, and performance under various workloads and contention levels.

## Overview

The project consists of five C++ implementations of an open-addressed hash set:

1. **Sequential Version 1** – Naive linear probing.
2. **Sequential Version 2** – Optimized with cuckoo hashing, logical deletions, and migration heuristics.
3. **Concurrent Version 1** – Naive striped locking with fallback to global locking.
4. **Concurrent Version 2** – Optimized with atomic reads, early exits, and improved stripe management.
5. **Transactional** – Based on STM, with writes inside transactions and reads performed speculatively.

## Build Instructions

```bash
make
```
This builds all versions of the hash set along with utilities for running benchmarks.

## Running Benchmarks
To run the experiments and gather timing data:
```bash
python3 benchmark.py
```
To generate the performance plots:
```bash
python3 plot.py
```
These scripts benchmark throughput under various configurations:
- Number of buckets: 1,000 and 10,000
- Number of operations: 10,000 and 100,000
- Thread counts: 1 to 16

## Implementation Highlights

### Sequential Version 2 (Optimized)
- Dual-table cuckoo hashing with bounded migration (max 32 hops).
- Logical deletions via valid flags to reduce overhead.
- Avoids unnecessary writes by checking membership before modifying.

### Concurrent Version 2 (Optimized)
- Stripe-based locks with atomic contains operations.
- Fine-grained synchronization only when necessary.
- Early-out optimizations for add/remove based on fast containment checks.

### Transactional
- Designed using a software transactional memory (STM) model.
- Read computations done outside of transactions for efficiency.
- Transactions only wrap critical write operations.

## Results Summary
- Low thread counts: Optimized sequential was fastest due to low lock overhead.
- High thread counts: Optimized concurrent version outperformed all others, demonstrating good scalability.
- Transactional version: Generally slower than others, especially in high-contention scenarios, due to retry overhead.
- Unoptimized concurrent version: Slowest across the board due to excessive locking and contention.

### Key Takeaways
- Optimizations like logical deletion, atomic reads, and early exits drastically improve performance.
- Lock striping scales well with threads but must be carefully managed to avoid global lock fallback.
- STM is viable but incurs overhead that must be justified by application-level benefits.