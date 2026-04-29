# DialSort vs Counting Sort — Experimental Benchmark
Course: ST0245 - Data Structures and Algorithms
University: EAFIT University — School of Applied Sciences and Engineering
Lecturer: Alexander Narváez Berrío
April 2026

### Team Members
Sofía Marín Bustamante

### Explanation 

This practice implements and experimentally compares two non-comparative integer sorting algorithms:

* DialSort-Counting — sorts integers by mapping each value directly to its position in a histogram, then sweeping the histogram left to right. No comparisons, no prefix-sum.
* Classic Counting Sort — similar histogram approach, but requires an extra prefix-sum pass to calculate exact output positions before distributing elements.

Both run in O(n + U) time, but DialSort eliminates the prefix-sum step, resulting in consistently faster real-world performance.

This benchmark implements up to 45 configurations to ensure a comprehensive and highly accurate analysis, given the volume and diversity of the data. The configurations are defined based on all possible combinations: the two algorithms, the size n of the input array, the size U of the universe, and the three types of distributions used. In addition, each measurement does not only consist of one run of the two algorithms with the parameters defined for that measurement, but each includes 3 warm-up rounds to ensure that the 5 measurement rounds yield more accurate data without interference from the cache, the operating system, or other factors that typically cause measurements to fluctuate.

The breakdown of the parameters used is as follows:
- **Distributions**
  - Uniform: all data has equally probability of appearing (each number is repeated                 around the same amount of times)
  - Biased: The 5% of the data represents 80% of the total, which means that the                   numbers included in that 5% appear much more frequently than the others. 
  - Nearly-Uniform: the array is almost completely ordered, with only 5% of the                            elements randomly displaced.

These distributions were chosen because they reflect the actual behavior of various types of data that are sorted—or could be sorted—using these algorithms, such as product sales data or sorted lists of customers, which are constantly updated with new entries. 

- **N** (Size of the input array):
  From the smallest accepted volume to the largest
  - 100,000
  - 500,000
  - 1,000,000
  - 5,000,000
  - 10,000,000

- **U** (Universe size):

  Each algorithm behaves differently depending on the size of the universe; in some cases, when the universe size U is very large but the     input size N is too small
  compared to U, DialSort requires much more space and is less efficient.
  Based on this information, these values for U were selected in order to test performance in different scenarios and evaluate the            algorithms.
  - 256
  - 1,024
  - 65,536

- **Warmup rounds**
  These three benchmark runs are not taken into account and are discarded; they serve to warm up the cache, the operating system, and the     data, so that the subsequent runs—which are included in the results—are consistent and yield more reliable values.

- **Measurement rounds**
  These five rounds are used to analyze the results. Three warm-up rounds plus five measurement rounds are performed for each combination     of distribution, array size n, and universe size U.

According to this structure the total amount of configurations is: 5*3*3= 45 configurations. And inside each configuration both algorithms are executed 16 times: (3+5)*2 = 16 executions.
In total, there are 45*16 = 720 executions of the program.

### Execution Instructions
Since the amount of executions of the program, it requieres to be run in a specific mode, otherwise it would take too long to complete.

**Option A — CLion (recommended)**

* Open the project in CLion
* Ensure you add `benchmark_dialsort_vs_bucketsort.cpp` to your `CMakeLists.txt`
* Switch build mode from 'Debug' to Release (top-left dropdown)
* Click the green 'Run' button

**Option B — Terminal**
```
g++ -O2 -std=c++17 -pthread -o benchmark_dialsort_vs_bucketsort.cpp
./benchmark

```
* Important: always compile in Release mode (`-O2)`. Debug mode is 10–50x slower and will take a very long time with n = 10,000,000.
  In release mode the compiler analyzes all your code at once and reorganizes it to make it as fast as possible. You can't easily pause it to inspect variables, but the result is much faster. Essentially, “Release” mode optimizes your code and will take around one minute to complete.

**Interactive Simulator**

Open `simulator.html` in any browser (Chrome recommended). The simulator shows both algorithms side by side in real time:
**Interactive Simulator**

Or you can run the simulator directly in your browser without downloading anything:
**[CLICK HERE TO OPEN THE SIMULATOR](https://Sofia-Marin0109.github.io/FINAL_PRACTICE/Simulator.html)**

* DialSort (Left): Each number falls to the height that matches its value, and a light beam sweeps left to right reading the histogram.
* Parallel Bucket Sort (Right): Elements are routed into discrete buckets based on their range. Each bucket is then sorted independently and simultaneously, simulating parallel threads, before being gathered into the final array.


### Experimental Results and Analysis

After executing 225 configurations across different input sizes ($n$), universe ranges ($U$), and data distributions, the experimental data revealed a clear dominance of the sequential DialSort approach within the tested parameters.

**Key Findings:**

* **Overall Performance:** DialSort was faster in **100%** of the test cases (225/225 configurations).
* **Maximum Speedup:** The highest performance gap occurred with $n = 100,000$, $U = 256$ under a Uniform distribution, where DialSort was **7.21x faster** than Parallel Bucket Sort.
* **Distribution Impact:** DialSort showed its strongest average advantage (**6.19x speedup**) on the **Biased** distribution. This is because DialSort's histogram writing phase heavily benefits from CPU cache locality when a small subset of numbers repeats constantly.

**Technical Conclusions:**

1. **The Overhead of Concurrency:** While Parallel Bucket Sort theoretically benefits from multi-threading, the overhead of dynamic memory allocation (`std::vector::push_back` for buckets) and thread synchronization (`std::thread::join`) proved to be costlier than DialSort's straightforward contiguous array accesses.
2. **Algorithmic Nature:** DialSort avoids key comparisons entirely, operating in strict $\mathcal{O}(n + U)$ time. Parallel Bucket Sort relies on `std::sort` (Introsort) inside each bucket, binding its internal performance to an $\mathcal{O}(n \log n)$ bound.
3. **The Universe ($U$) Bottleneck:** Parallel Bucket Sort's relative performance improved noticeably as $U$ increased. Its best relative showing (reducing DialSort's speedup down to just **2.43x**) occurred at $n = 100,000$ and $U = 65,536$. This trend confirms the theoretical vulnerability of DialSort: if $U$ were exponentially larger than $n$ (e.g., $U = 10^9$), DialSort would suffer from massive memory allocation and inefficient sweeps, which is exactly the scenario where a distributed Parallel Bucket Sort would theoretically dominate.
