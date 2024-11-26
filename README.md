# Huffman Coding Compression and Performance Analysis

A comprehensive C++ implementation of Huffman coding with both sequential and OpenMP-optimized parallel versions, along with automated benchmarking tools developed by Chi Zhang.

## Table of Contents

- [Features](#features)
- [How It Works](#how-it-works)
- [Experimental Setup](#experimental-setup)
- [Compilation and Usage](#compilation-and-usage)
- [Benchmarking Tools](#benchmarking-tools)
- [Performance Analysis](#performance-analysis)
- [Troubleshooting](#troubleshooting)
- [Test Data Generation](#test-data-generation)

## Features

- Lossless compression using Huffman coding algorithm
- Both sequential and parallel (OpenMP) implementations
- Password protection for compressed files
- Automated benchmarking suite with configurable parameters
- Three types of test data generation (random, repeating, skewed)
- Comprehensive performance metrics and analysis tools

## How It Works

### Compression Process

The compressor operates in two passes:

1. **First Pass:**
   - Count byte frequencies
   - Build Huffman tree
   - Generate translation table
   - Write header information

2. **Second Pass:**
   - Encode data using Huffman codes
   - Write compressed data

### OpenMP Parallelization

The parallel version (`Compressor_OpenMP.cpp`) optimizes:
- Parallel byte frequency counting
- Concurrent Huffman tree construction
- Parallel file compression
- Thread-safe variable handling

## Experimental Setup

### Hardware Requirements
- CPU: x86_64 processor with multi-core support
- Memory: 4GB+ RAM
- Storage: SSD recommended for consistent I/O

### Software Requirements
- Operating System: Linux (Ubuntu/CentOS recommended)
- Compiler: GCC 7.0+ with OpenMP support
- Build System: GNU Make
- Shell: Bash 4.0+

## Compilation and Usage

### Building the Project

1. **Compile all components:**
   ```bash
   make clean
   make all
   ```

2. **Compiler configuration options:**
   ```makefile
   # Default configuration in Makefile
   CXX ?= g++
   CXXFLAGS ?= -std=c++14 -O3 -w -Wfatal-errors
   OMPFLAGS = -fopenmp
   ```

3. **Platform-specific compilation:**
   - Linux:
     ```bash
     make all CXX=g++ CXXFLAGS='-std=c++14'
     ```
   - macOS:
     ```bash
     brew install gcc
     make all CXX=g++-11 CXXFLAGS='-std=c++14'
     ```

### Basic Usage

1. **Compression:**
   ```bash
   # Original sequential version
   ./build/archive <input_file_or_directory>
   
   # OpenMP parallel version
   ./build/modified_archive <input_file_or_directory>
   ```

2. **Decompression:**
   ```bash
   ./build/extract <compressed_file>
   ```

## Benchmarking Tools

### Data Generator (`data_generator.cpp`)

Generates test data with different characteristics:

1. **Random Data:**
   ```cpp
   // Uniform distribution (0-255)
   void generateRandomData(ofstream& file, size_t size) {
       random_device rd;
       mt19937 gen(rd());
       uniform_int_distribution<> dis(0, 255);
       // ... buffered writing
   }
   ```

2. **Repeating Pattern:**
   ```cpp
   // Fixed pattern repetition
   void generateRepeatingData(ofstream& file, size_t size) {
       const string pattern = "HelloWorldThisIsARepeatingPattern";
       // ... buffered writing with pattern
   }
   ```

3. **Skewed Distribution:**
   ```cpp
   // Exponential distribution
   void generateSkewedData(ofstream& file, size_t size) {
       exponential_distribution<> dis(0.1);
       // ... buffered writing
   }
   ```

Usage:
```bash
./build/data_generator <output_file> <size_in_MB> <type>
# type: 0=random, 1=repeating, 2=skewed
```

### Benchmark Script (`run_benchmarks.sh`)

Automated testing with configurable parameters:

```bash
# Basic usage
./run_benchmarks.sh

# Custom configuration
./run_benchmarks.sh -s "1 5 10" -t "1 4 8" -d "random repeating" -k
```

Options:
- `-s, --sizes`: Test file sizes (MB)
- `-t, --threads`: Thread counts
- `-d, --datatypes`: Data types to test
- `-k, --keep-data`: Preserve test files
- `-o, --output`: Custom output file

## Performance Analysis

### Metrics Collected

1. **Compression Metrics:**
   - Input/output file sizes
   - Compression ratios
   - Space efficiency

2. **Performance Metrics:**
   - Execution times (sequential vs parallel)
   - CPU utilization
   - Thread scaling efficiency
   - Speedup ratios

3. **Resource Usage:**
   - CPU time (user + system)
   - Memory utilization
   - I/O statistics

### Results Presentation

Results are presented in multiple formats:

1. **Tabular Data:**
   ```
   | Size | Input(MB) | Original(MB) | Parallel(MB) | Original(s) | Optimized(s) | Speedup |
   |------|-----------|--------------|--------------|-------------|--------------|---------|
   | 100  |    100.00 |       45.20  |      45.20   |      1.500  |       0.300  |   5.00  |
   ```

2. **ASCII Visualizations:**
   ```
   Speedup vs Thread Count:
   Threads 1: ##### 2.50
   Threads 2: ########## 5.00
   Threads 4: ############### 7.50
   ```

## Troubleshooting

### Common Issues

1. **Compiler Errors:**
   - OpenMP not supported: Install GCC with OpenMP
   - Undefined OpenMP references: Add `-fopenmp` flag

2. **Runtime Issues:**
   - File permissions: Ensure write access
   - Memory errors: Check available RAM
   - Thread count: Verify CPU core availability

### Best Practices

1. **For Reproducible Results:**
   - Run on idle system
   - Disable CPU frequency scaling
   - Document system configuration
   - Multiple test iterations

2. **Performance Optimization:**
   - Match thread count to CPU cores
   - Consider NUMA architecture
   - Monitor system resources

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Author

Chi Zhang

## Acknowledgments

Special thanks to:
- OpenMP community
- Project contributors
- Open source community

For detailed implementation information, bug reports, or feature requests, please refer to the project repository.

## Test Data Generation

## Test Generator Script (`test_generator.sh`)

The `test_generator.sh` script is designed to generate and verify test data for the Huffman coding performance analysis. It provides a systematic way to create test files with different characteristics and sizes.

### Features

1. **Automated Data Generation**
   - Creates test files for each data type (random, repeating, skewed)
   - Verifies file sizes and content integrity
   - Provides hexdump previews for verification

2. **Size Verification**
   - Checks actual file sizes against expected sizes
   - Reports any size mismatches
   - Ensures data generation accuracy

3. **Content Preview**
   - Shows hexdump of first 32 bytes
   - Helps verify data patterns
   - Useful for debugging

### Usage

```bash
# Basic usage
./test_generator.sh

# The script will:
# 1. Create test_data directory if it doesn't exist
# 2. Generate test files for each combination of:
#    - Sizes: 1MB, 10MB (configurable)
#    - Types: random, repeating, skewed
# 3. Verify file sizes and content
# 4. Display hexdump previews
```

### Example Output

```
Generating 1MB random data to test_data/random_1MB.dat
Expected size: 1048576 bytes
Actual size: 1048576 bytes
✓ File size correct
----------------------------------------
Generating 1MB repeating data to test_data/repeating_1MB.dat
Expected size: 1048576 bytes
Actual size: 1048576 bytes
✓ File size correct
----------------------------------------
File content preview:
== First 32 bytes of test_data/random_1MB.dat ==
00000000  7f 2a 15 9c 4e b3 a1 d8  f6 0c 92 5b e4 3d 76 c8  |..*..N.....[.=v.|
00000010  1b 8f 50 e9 d2 67 3c a5  b8 41 fc 0d 95 6e 23 7a  |..P..g<..A...n#z|

== First 32 bytes of test_data/repeating_1MB.dat ==
00000000  48 65 6c 6c 6f 57 6f 72  6c 64 54 68 69 73 49 73  |HelloWorldThisIs|
00000010  41 52 65 70 65 61 74 69  6e 67 50 61 74 74 65 72  |ARepeatingPatter|
```

### Configuration

The script can be configured by modifying the following parameters at the beginning of the file:

```bash
# Test parameters
sizes=(1 10)  # Test with 1MB and 10MB files
types=("random" "repeating" "skewed")  # Data distribution types
type_nums=(0 1 2)  # Corresponding numeric codes for data types
```

### Data Types

1. **Random Data (type 0)**
   - Uniform distribution of bytes
   - Simulates encrypted or compressed data
   - Useful for worst-case compression testing

2. **Repeating Data (type 1)**
   - Fixed pattern repetition
   - Tests compression efficiency
   - Demonstrates best-case compression scenarios

3. **Skewed Data (type 2)**
   - Exponential distribution
   - Simulates real-world text and data
   - Tests practical compression performance

### Integration with Benchmark Suite

The test generator is designed to work seamlessly with the benchmark suite:

1. **Pre-benchmark Testing**
   ```bash
   # Generate test data before running benchmarks
   ./test_generator.sh
   ./run_benchmarks.sh
   ```

2. **Custom Test Data**
   ```bash
   # Modify test_generator.sh parameters
   # Then generate custom test data
   ./test_generator.sh
   ```

3. **Verification**
   ```bash
   # Verify existing test data
   ./test_generator.sh
   # Check hexdump output for data patterns
   ```

### Best Practices

1. **Before Running Benchmarks**
   - Run test generator to ensure fresh test data
   - Verify file sizes and content patterns
   - Check disk space availability

2. **Data Verification**
   - Review hexdump previews
   - Confirm expected patterns
   - Check for any generation errors

3. **Custom Testing**
   - Modify size parameters for specific needs
   - Add new data types if required
   - Adjust verification methods as needed
