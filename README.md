# High-Performance Radix Sort (C++20)

A high-performance, generic radix sort library for C++ with support for:
- Integral and floating-point types
- `std::string`
- User-defined types via projections
- Sequential and parallel execution

This library is designed for **maximum scalable performance** while **maintaining practical stability**, particularly on large inputs.

---

## Motivation

- When I first learned about radix sort, I was impressed by its "O(n)" complexity, so I tried implementing it in C++ as efficiently as I could. It quickly evolved into an infatuation, and I wanted to see how far I could go with it.
- I also used this as an excuse to learn more about modern C++ features and template metaprogramming.

---

## Features

- **Supports multiple key types**
  - Signed and unsigned integrals
  - Floating-point types (following IEEE-754 convention)
  - `std::string`
  - Composite types via projections

- **Adaptive strategy selection**
  - MSD vs LSD radix selection based on type and execution mode.
  - Automatic fallback to `std::sort` / `std::stable_sort` for degenerate cases.

- **Parallel execution**
  - Efficient work partitioning.
  - Dynamic thread count selection.
  - Safe handling of uneven bucket distributions.

- **Robust against pathological inputs**
  - Duplicates
  - Nearly sorted data
  - Sorted / reverse sorted data

- [**Benchmarked across data shapes**](https://github.com/Devil11Assassin/radix-sort/tree/master/RadixSort/benchmarks)
  - Randomized
  - Sorted
  - Reverse sorted
  - Nearly sorted (95%)
  - Duplicates (256 items repeated across N)

---

## Supported Types

| Type category       | Supported | Notes |
|---------------------|-----------|-------|
| Integral types      | ✅ | Signed and unsigned |
| Floating-point      | ✅ | IEEE-754 compliance |
| `std::string`       | ✅ | MSD radix with safeguards |
| Custom types        | ✅ | Via projection |

---

## Stability

### This library’s guarantee
- The algorithm **preserves stability for all composite and projected types**.
- For primitives and `std::string`, an input-reversal optimization is applied on reverse-sorted data and MSD radix sort falls back to std::sort when it degenerates, both of which break *formal* stability but do not affect relative ordering in typical real-world usage (**practically stable**).
- If strict (*formal*) stability is required, the reversal optimization can be removed, and the MSD fallback can be changed to std::stable_sort.

---

## Algorithm Overview

- Uses **LSD radix sort** for:
  - 1-byte fixed-width keys.
  - Fixed-width keys smaller than 8 bytes when single-threaded.
- Uses **MSD radix sort** for:
  - `std::string` keys.
  - 8-byte (or more) fixed-width keys.
  - Fixed-width keys larger than 1 byte with parallel execution.
- Employs **counting + prefix sum** passes.
- Dynamically partitions work across threads.
- Detects degenerate MSD behavior (excessive depth) and falls back to comparison sort.

---

## Parallel Execution

Parallel sorting is enabled automatically when:
- Input size exceeds a configurable threshold.
- Hardware concurrency is available.

Thread count selection:
- Scales in powers of two once input size exceeds 1 million.
- Capped by hardware and software limits.
- Avoids oversubscription for small inputs.

Parallel execution uses:
- Work-stealing-friendly region partitioning.
- Local and global bucket thresholds.
- Minimal synchronization overhead.

---

## Usage

```cpp
#include <vector>
#include "radix_sort.hpp"

// primitives & std::string
std::vector<int> v = ...;
radix_sort::sort(v);           // single-threaded
radix_sort::sort(v, {}, true); // multi-threaded


// composite types
struct Composite {
  std::string key;
  long long data;
};

std::vector<Composite> v = ...;
radix_sort::sort(v, &Composite::key);       // single-threaded
radix_sort::sort(v, &Composite::key, true); // multi-threaded
```
