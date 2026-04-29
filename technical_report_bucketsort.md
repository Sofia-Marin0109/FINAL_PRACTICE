# Technical Report
## Experimental Analysis: DialSort vs Parallel Bucket Sort

**Course:** ST0245 — Data Structures and Algorithms  
**University:** EAFIT University — School of Applied Sciences and Engineering  
**Lecturer:** Alexander Narváez Berrío  
**Author:** Sofía Marín Bustamante
**Date:** April 2026

---

## 1. Introduction

This report presents the experimental comparison between two sorting algorithms: **DialSort** and **Parallel Bucket Sort**. Both algorithms take fundamentally different approaches to the sorting problem, making this comparison particularly interesting.

DialSort is a non-comparative algorithm that exploits the mathematical property of integers: each value already encodes its own position in the sorted output. Parallel Bucket Sort, on the other hand, divides the data into subsets (buckets), sorts each one independently using multiple CPU threads at the same time, and then reassembles the results.

The central question of this experiment is: **can parallelism compensate for the structural overhead of Bucket Sort, or does DialSort's simpler, sequential approach outperform it despite using only one thread?**

The benchmark was executed **5 independent times** across 45 configurations each (5 input sizes × 3 universe sizes × 3 distributions), for a total of **225 measured comparisons**.

---

## 2. Algorithm Descriptions

### 2.1 DialSort-Counting

DialSort is built on the **Self-Indexing Principle**: an integer value `k` is its own address. No comparison is needed to determine where it belongs — it already knows.

The algorithm works in two phases:

**Phase 1 — Build histogram (ingestion):**
```
for each element k in the input:
    H[k - minimum]++
```
Each element increments the counter at its own position. Time: O(n).

**Phase 2 — Sweep histogram (projection):**
```
for y = 0 to U-1:
    write (y + minimum) exactly H[y] times into the array
```
Reading the histogram left to right produces the sorted sequence directly, because it is already indexed from smallest to largest. Time: O(U + n).

**Total: O(n + U). Extra memory: O(U). Comparisons: 0. Threads: 1.**

### 2.2 Parallel Bucket Sort

Parallel Bucket Sort divides the sorting problem into K independent subproblems, where K is the number of CPU threads available on the machine.

**Phase 1 — Find range:**
```
minimum = min(array)
maximum = max(array)
```
Time: O(n).

**Phase 2 — Scatter (distribute into buckets):**
```
range_per_bucket = (maximum - minimum + 1) / K
for each element:
    bucket_index = (element - minimum) / range_per_bucket
    buckets[bucket_index].push(element)
```
Assigns each element to a bucket based on its value range. Elements in bucket 0 are all smaller than elements in bucket 1, and so on. Time: O(n).

**Phase 3 — Parallel sort:**
```
for each bucket i: (in parallel, each on its own thread)
    std::sort(buckets[i])
```
Each thread independently sorts its bucket using std::sort (introsort, O(m log m) where m = bucket size). Because buckets cover non-overlapping value ranges, they never need to communicate. Time: O((n/K) log(n/K)) per thread, with K threads running simultaneously.

**Phase 4 — Gather (reassemble):**
```
for each bucket i in order:
    copy buckets[i] into the output array
```
Because buckets are ordered by value range, concatenating them produces the final sorted array. Time: O(n).

**Total: O(n + (n/K) log(n/K)). Extra memory: O(n) for buckets. Threads: K (hardware-dependent).**

### 2.3 Key Structural Differences

| Step | DialSort | Parallel Bucket Sort |
|------|----------|----------------------|
| Find range | ✅ O(n) | ✅ O(n) |
| Build histogram / scatter | ✅ O(n) — single pass | ✅ O(n) — single pass |
| Prefix-sum | ❌ Not needed | ❌ Not needed |
| Sort elements | ❌ Not needed (histogram is already sorted) | ✅ O((n/K) log(n/K)) per thread |
| Gather / sweep | ✅ O(U + n) | ✅ O(n) |
| Extra memory | ✅ O(U) — histogram only | ❌ O(n) — full copy in buckets |
| Comparisons | ✅ **0** | ❌ ~(n/K) log(n/K) × K |
| Parallelism | ❌ Sequential | ✅ K threads |

The critical difference: DialSort never sorts the elements — the histogram does it implicitly. Parallel Bucket Sort still needs to sort each bucket with a comparison-based algorithm, which is unavoidable regardless of how many threads are used.

---

## 3. Complexity Analysis

### 3.1 Time Complexity

| Case | DialSort | Parallel Bucket Sort |
|------|----------|----------------------|
| Best | O(n + U) | O(n + (n/K) log(n/K)) |
| Average | O(n + U) | O(n + (n/K) log(n/K)) |
| Worst | O(n + U) | O(n + (n/K)²) if buckets are unbalanced |

**Important note about the worst case of Parallel Bucket Sort:** if all elements land in the same bucket (which happens with highly skewed data), that single bucket has n elements and std::sort runs in O(n²) worst case (though in practice introsort degrades to O(n log n) not O(n²)). DialSort has no such vulnerability — its complexity is always O(n + U), regardless of data distribution.

### 3.2 Space Complexity

| Algorithm | Extra Memory |
|-----------|-------------|
| DialSort | O(U) — histogram only |
| Parallel Bucket Sort | O(n) — all elements copied into bucket vectors |

At n = 10,000,000 with 32-bit integers:
- DialSort: at most 65,536 × 4 bytes = **256 KB** (for U = 65,536)
- Parallel Bucket Sort: 10,000,000 × 4 bytes = **~40 MB** plus bucket structure overhead

### 3.3 The Parallelism Factor

Parallel Bucket Sort uses K threads, where K = `thread::hardware_concurrency()`. On most modern laptops this is 8 to 16 threads. Theoretically, this should give it a K× advantage in the sorting phase.

However, parallelism has overhead:
- **Thread creation:** each `std::thread` launch has a fixed cost (~microseconds)
- **Memory allocation:** each bucket is a separate `vector<int>` that requires a heap allocation
- **Cache pressure:** K threads writing to K different memory regions simultaneously causes cache contention
- **Join synchronization:** the main thread must wait for the slowest thread before gathering

These costs are small but not zero, and they become proportionally larger at small n.

---

## 4. Experimental Setup

### 4.1 Implementation

Both algorithms were implemented in C++17, compiled with `-O2` optimization in Release mode. Time was measured using `std::chrono::high_resolution_clock` with nanosecond precision. Each configuration was run 5 times after 3 warmup rounds (which are discarded). Mean, standard deviation, minimum time, and throughput were computed for each run.

### 4.2 Test Parameters

| Parameter | Values |
|-----------|--------|
| Input size n | 100,000 / 500,000 / 1,000,000 / 5,000,000 / 10,000,000 |
| Universe size U | 256 / 1,024 / 65,536 |
| Distributions | Uniform / Biased / Nearly Uniform |
| Warmup rounds | 3 (discarded) |
| Measurement rounds | 5 (averaged) |
| Independent runs | **5** (benchmark run 5 separate times) |
| Total comparisons | **225** |
| Platform | Apple Silicon arm64, Release mode, `-O2` |

### 4.3 Data Distributions

**Uniform:** each value in [0, U-1] has equal probability of appearing. With large n and small U, every value appears approximately n/U times.

**Biased:** 80% of the data falls in the bottom 5% of the universe (the "hot zone"). This means a small group of values is extremely frequent, while the rest are rare. Represents real-world data like sales frequencies or word counts.

**Nearly Uniform:** the array is fully sorted first, then exactly 5% of elements are randomly changed to new values. Represents an already-ordered dataset with a small number of recent insertions.

---

## 5. Results

### 5.1 Global Summary (225 configurations)

| Metric | Value |
|--------|-------|
| Total configurations measured | 225 |
| DialSort wins | **225 / 225 (100%)** |
| Average speedup | **5.76x** |
| Median speedup | **5.92x** |
| Minimum speedup | **2.45x** (n=100K, U=65536, Nearly Uniform) |
| Maximum speedup | **7.21x** (n=100K, U=256, Uniform) |
| DialSort avg throughput | **173.7 Mkeys/s** |
| Parallel Bucket Sort avg throughput | **30.1 Mkeys/s** |

### 5.2 Speedup by Distribution

| Distribution | Avg Speedup | Min Speedup | Max Speedup |
|---|---|---|---|
| Uniform | 5.57x | 2.64x | 7.21x |
| Biased | **6.19x** | 3.82x | 7.11x |
| Nearly Uniform | 5.52x | 2.45x | 6.56x |

Biased distribution gives DialSort the highest advantage. This makes sense: when 80% of elements are in the bottom 5% of the universe, almost all buckets in Parallel Bucket Sort are nearly empty while one bucket is overloaded with 80% of the data. That overloaded bucket has to sort a massive portion of the array by itself. DialSort is unaffected by this — it simply counts frequencies and sweeps, with the same behavior regardless of how data is distributed.

### 5.3 Speedup by Universe Size U

| U | Avg Speedup |
|---|---|
| 256 | 5.87x |
| 1,024 | **6.16x** |
| 65,536 | 5.25x |

U=1024 gives the highest speedup. At U=65536, DialSort's histogram sweep costs more (65,536 positions to iterate), which narrows the gap slightly. However, even at U=65536, DialSort wins by more than 5x on average.

### 5.4 Speedup by Input Size n

| n | Avg Speedup |
|---|---|
| 100,000 | 4.94x |
| 500,000 | 5.70x |
| 1,000,000 | 5.88x |
| 5,000,000 | 6.12x |
| **10,000,000** | **6.17x** |

The speedup **increases with n**. At small n, Parallel Bucket Sort's parallelism gives it a relative advantage because thread creation cost is amortized over fewer elements. As n grows, the comparison-based sorting in each bucket dominates the total time, and DialSort's O(n) advantage compounds.

### 5.5 Sample Results at n = 10,000,000 (average of 5 runs)

| Algorithm | Distribution | U | Media (ms) | Desv (ms) | Mkeys/s | Speedup |
|-----------|-------------|---|-----------|----------|---------|---------|
| DialSort | Uniform | 256 | ~52.5 | ~0.9 | ~190 | **~6.0x** |
| Parallel Bucket Sort | Uniform | 256 | ~313.6 | ~0.7 | ~31.8 | — |
| DialSort | Biased | 256 | ~52.7 | ~0.9 | ~189 | **~6.3x** |
| Parallel Bucket Sort | Biased | 256 | ~334.1 | ~2.4 | ~29.7 | — |
| DialSort | Uniform | 65,536 | ~53.2 | ~1.1 | ~188 | **~6.1x** |
| Parallel Bucket Sort | Uniform | 65,536 | ~320.6 | ~1.4 | ~31.2 | — |
| DialSort | Biased | 65,536 | ~52.9 | ~1.2 | ~188 | **~6.9x** |
| Parallel Bucket Sort | Biased | 65,536 | ~366.1 | ~1.8 | ~27.3 | — |

### 5.6 Stability of Results

The standard deviation values across all 5 independent benchmark runs are consistently below 2ms even at n = 10,000,000. This indicates that the results are stable and not affected by random OS interruptions or cache effects. The speedup values are reproducible across all 5 independent executions.

---

## 6. Analysis and Discussion

### 6.1 Why DialSort wins despite being sequential

This result might seem counterintuitive: a single-threaded algorithm consistently outperforms a multi-threaded one by 5-7x. The reason lies in what each algorithm actually does.

Parallel Bucket Sort has three sources of overhead that DialSort completely avoids:

**1. Comparison-based internal sort.** Each bucket is sorted with `std::sort`, which performs approximately (n/K) × log(n/K) comparisons per bucket. Even with 8 or 16 threads running in parallel, the total comparison work is O(n log(n/K)), which is still super-linear. DialSort performs zero comparisons — the histogram implicitly sorts.

**2. Memory duplication.** Every element is copied twice in Parallel Bucket Sort: once from the original array into its bucket (scatter), and once from the bucket back into the original array (gather). DialSort overwrites the original array directly in a single sweep.

**3. Memory allocation for buckets.** Each of the K buckets is a `vector<int>` that must be dynamically allocated. At n = 10,000,000, this means allocating ~40MB of bucket storage in addition to the original 40MB array. More memory means more cache misses.

### 6.2 Why the advantage grows with n

At n = 100,000, Parallel Bucket Sort is "only" ~5x slower. At n = 10,000,000, it is ~6.2x slower. This is because the O(n log n) comparison work in the buckets grows faster than the O(n + U) cost of DialSort. As n doubles, DialSort's cost roughly doubles, but Parallel Bucket Sort's cost grows by slightly more than double because of the log factor.

### 6.3 Why Biased data hurts Parallel Bucket Sort most

With biased data, 80% of elements land in the bottom 5% of the universe. This means ~80% of all elements end up in the first one or two buckets. Those buckets become massive and one thread must sort a huge subarray while other threads finish almost instantly. The parallel advantage disappears because the work is unbalanced.

DialSort is completely unaffected by this — it counts frequencies in one pass and sweeps the histogram in another, with exactly the same number of operations regardless of how skewed the data is.

### 6.4 The role of U

At U = 65,536, DialSort's histogram sweep iterates over 65,536 positions. This adds a fixed O(U) cost on top of O(n). At n = 100,000 with U = 65,536, this extra cost is significant relative to n, which explains why the minimum speedup (2.45x) occurs at this configuration. As n grows to 10,000,000, the fixed O(U) cost becomes negligible compared to O(n), and the speedup recovers to ~6x.

---

## 7. Answer to "Which Algorithm Performed Better?"

**DialSort-Counting performed better in every single one of the 225 configurations tested, with an average speedup of 5.76x.**

The result is unambiguous and consistent across all input sizes, universe sizes, and data distributions. The minimum advantage was 2.45x and the maximum was 7.21x — meaning DialSort was never slower, not even once.

This outcome demonstrates a fundamental principle in algorithm design: **asymptotic complexity matters more than parallelism** when the comparison work is not eliminated. Parallel Bucket Sort adds K threads to speed up the sorting phase, but it cannot escape the fact that it still performs O(n log n) comparisons in total. DialSort performs zero comparisons, regardless of n.

The practical implication is clear: for integer sorting over a bounded universe where n ≥ U, a well-implemented sequential non-comparative algorithm is more efficient than a parallel comparison-based one, because the overhead of parallelism does not compensate for the inherent cost of comparisons.

---

## 8. Conclusions

1. **DialSort-Counting outperformed Parallel Bucket Sort in all 225 configurations** tested across 5 independent benchmark runs.

2. The **average speedup was 5.76x**, with values consistently between 2.45x and 7.21x depending on the configuration.

3. The speedup **increases with n**: at n = 10M, DialSort was ~6.2x faster on average. This confirms that the advantage is structural and grows as the dataset scales.

4. **Biased distribution favored DialSort the most** (avg 6.19x speedup) because uneven data causes bucket imbalance in Parallel Bucket Sort, where one thread ends up sorting most of the data.

5. The main reason DialSort wins despite being single-threaded is that it **performs zero comparisons**. Parallel Bucket Sort must still execute O(n log n) comparisons in total across all threads, while DialSort only counts and sweeps.

6. **Parallelism alone does not make an algorithm faster** if the underlying algorithmic complexity is higher. Reducing the number of operations (DialSort: 0 comparisons) consistently beats executing more operations faster (Parallel Bucket Sort: O(n log n) comparisons with K threads).

7. DialSort's limitation remains: **when U >> n**, the histogram becomes wasteful. For example, sorting 2 integers with values [0, 10,000,000] requires a 10-million-position histogram. In those edge cases, Parallel Bucket Sort or any comparison-based sort would be more appropriate.

---

## 9. References

- Narváez Berrío, A. (2026). *DialSort: Non-Comparative Integer Sorting via the Self-Indexing Principle*. Zenodo. https://doi.org/10.5281/zenodo.19169171
- Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2022). *Introduction to Algorithms* (4th ed.). MIT Press. Chapter 8: Sorting in Linear Time.
- Williams, A. (2019). *C++ Concurrency in Action* (2nd ed.). Manning Publications. Chapter 2: Managing threads.
- ISO/IEC 14882:2017. *C++ Standard Library*: `std::thread`, `std::sort`, `std::chrono`.
