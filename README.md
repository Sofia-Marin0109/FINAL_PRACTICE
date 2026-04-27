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
  
  Each algorithm behaves differently depending on the size of the universe; in some      cases, when the universe size U is very large but the input size N is too small
  compared to U, DialSort requires much more space and is less efficient.
  Based on this information, these values for U were selected in order to test
  performance in different scenarios and evaluate the algorithms. 
  - 256
  - 1,024
  - 65,536 
hdeifjiwejw