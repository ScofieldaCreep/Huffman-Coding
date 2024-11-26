#!/bin/bash

# Default configuration parameters
DEFAULT_SIZES=(1 10 100 500)  # Default file sizes (MB)
DEFAULT_TYPES=("repeating" "skewed")  # Default data types
DEFAULT_TYPE_NUMS=(0 1 2)  # Numbers corresponding to data types
DEFAULT_THREAD_COUNTS=(1 2 8 $(nproc))  # Default thread counts
DEFAULT_KEEP_DATA=false  # Default: don't keep test data
DEFAULT_RESULTS_FILE="benchmark_results.md"  # Default results file

# Color definitions for terminal output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'  # No Color

# Display usage information
usage() {
    cat << EOF
Usage: $0 [options]

Options:
    -s, --sizes "size1 size2 ..."       Specify test file sizes (MB), default: ${DEFAULT_SIZES[@]}
    -t, --threads "t1 t2 ..."          Specify thread counts, default: ${DEFAULT_THREAD_COUNTS[@]}
    -d, --datatypes "type1 type2 ..."  Specify data types, default: ${DEFAULT_TYPES[@]}
    -k, --keep-data                    Keep test data files
    -o, --output filename              Specify output filename, default: $DEFAULT_RESULTS_FILE
    -h, --help                         Display this help message

Examples:
    $0 -s "1 5 10" -t "1 4 8" -k
    $0 --sizes "50 100" --threads "1 2 4 8" --datatypes "random repeating"
EOF
}

# Parse command line arguments
parse_args() {
    SIZES=("${DEFAULT_SIZES[@]}")
    TYPES=("${DEFAULT_TYPES[@]}")
    TYPE_NUMS=("${DEFAULT_TYPE_NUMS[@]}")
    THREAD_COUNTS=("${DEFAULT_THREAD_COUNTS[@]}")
    KEEP_DATA=$DEFAULT_KEEP_DATA
    RESULTS_FILE=$DEFAULT_RESULTS_FILE

    while [[ $# -gt 0 ]]; do
        case $1 in
            -s|--sizes)
                IFS=' ' read -r -a SIZES <<< "$2"
                shift 2
                ;;
            -t|--threads)
                IFS=' ' read -r -a THREAD_COUNTS <<< "$2"
                shift 2
                ;;
            -d|--datatypes)
                IFS=' ' read -r -a TYPES <<< "$2"
                shift 2
                ;;
            -k|--keep-data)
                KEEP_DATA=true
                shift
                ;;
            -o|--output)
                RESULTS_FILE="$2"
                shift 2
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                echo "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done
}

# Error handling function
handle_error() {
    echo -e "${RED}Error: $1${NC}" >&2
    exit 1
}

# Check prerequisites
check_prerequisites() {
    local required_files=(
        "build/archive"
        "build/modified_archive"
        "build/test_compression"
        "build/data_generator"
    )
    
    for file in "${required_files[@]}"; do
        if [[ ! -x "$file" ]]; then
            handle_error "Executable not found: $file"
        fi
    done
    
    if [[ ! -d "data" ]]; then
        echo -e "${YELLOW}Creating data directory...${NC}"
        mkdir -p data
    fi
}

# Cleanup temporary files
cleanup() {
    if [ "$KEEP_DATA" = false ]; then
        echo -e "${YELLOW}Cleaning up temporary files...${NC}"
        rm -f temp_output.txt temp_error.txt
        rm -f data/*.compressed
        rm -f data/*.original.compressed
        rm -f data/*.modified.compressed
        rm -f data/*.bin
    else
        echo -e "${YELLOW}Keeping test data files...${NC}"
        rm -f temp_output.txt temp_error.txt
    fi
}

# Get numeric code for data type
get_type_num() {
    local type=$1
    case "$type" in
        "random") echo 0 ;;
        "repeating") echo 1 ;;
        "skewed") echo 2 ;;
        *) handle_error "Unknown data type: $type" ;;
    esac
}

# Run single test and collect data
run_test() {
    local size=$1
    local type=$2
    local thread_count=$3
    local input_file="data/${type}_${size}.bin"
    
    if [[ ! -f "$input_file" ]]; then
        echo -e "${YELLOW}Generating test data: ${type}_${size}MB...${NC}"
        local type_num=$(get_type_num "$type")
        ./build/data_generator "$input_file" "$size" "$type_num"
        if [[ $? -ne 0 ]]; then
            echo -e "${RED}Warning: Failed to generate test data${NC}" >&2
            return 1
        fi
    fi
    
    export OMP_NUM_THREADS=$thread_count
    
    echo -e "${GREEN}Testing $type data, size: ${size}MB, threads: $thread_count${NC}"
    
    # Run original compression to get size
    rm -f "${input_file}.compressed" 2>/dev/null
    if ! ./build/archive "$input_file" < <(echo -e "0\n1") > /dev/null 2>&1; then
        echo -e "${RED}Warning: Original compression failed${NC}" >&2
        return 1
    fi
    local original_compressed_size=$(stat -f%z "${input_file}.compressed" 2>/dev/null || stat -c%s "${input_file}.compressed")
    rm -f "${input_file}.compressed" 2>/dev/null

    # Run parallel compression to get size
    if ! ./build/modified_archive "$input_file" < <(echo -e "0\n1") > /dev/null 2>&1; then
        echo -e "${RED}Warning: Parallel compression failed${NC}" >&2
        return 1
    fi
    local parallel_compressed_size=$(stat -f%z "${input_file}.compressed" 2>/dev/null || stat -c%s "${input_file}.compressed")
    rm -f "${input_file}.compressed" 2>/dev/null

    # Run test_compression for timing
    if ! ./build/test_compression "$input_file" > temp_output.txt 2>temp_error.txt; then
        echo -e "${RED}Warning: test_compression failed${NC}" >&2
        cat temp_error.txt >&2
        return 1
    fi

    # Extract results and calculate metrics
    local original_time=$(grep "Detailed Report" -A 1 temp_output.txt | tail -n 1 | awk '{print $5}')
    local modified_time=$(grep "Detailed Report" -A 1 temp_output.txt | tail -n 1 | awk '{print $6}')
    local input_size_bytes=$(grep "Detailed Report" -A 1 temp_output.txt | tail -n 1 | awk '{print $2}')
    
    # Measure system-level performance
    local time_output
    time_output=$(TIMEFORMAT='%R %U %S'; { time ./build/modified_archive "$input_file" < <(echo -e "0\n1") ; } 2>&1 >/dev/null)
    
    # Parse time command output
    local real_time=$(echo "$time_output" | awk '{print $1}')
    local user_time=$(echo "$time_output" | awk '{print $2}')
    local sys_time=$(echo "$time_output" | awk '{print $3}')
    local cpu_time=$(echo "$user_time + $sys_time" | bc)
    local cpu_usage=$(echo "scale=2; 100 * $cpu_time / $real_time" | bc)
    
    # Calculate sizes and ratios
    local input_size_mb=$(echo "scale=2; $input_size_bytes / 1048576" | bc)
    local original_compressed_mb=$(echo "scale=2; $original_compressed_size / 1048576" | bc)
    local parallel_compressed_mb=$(echo "scale=2; $parallel_compressed_size / 1048576" | bc)
    local speedup=$(echo "scale=2; $original_time / $modified_time" | bc)
    local compression_ratio=$(echo "scale=2; $parallel_compressed_size / $input_size_bytes" | bc)
    
    # Output results to file
    printf "| %sMB | %.2f | %.2f | %.2f | %.3f | %.3f | %.3f | %.3f (%.0f%%) | %.2f | %.2f |\n" \
        "$size" "$input_size_mb" "$original_compressed_mb" "$parallel_compressed_mb" \
        "$original_time" "$modified_time" "$real_time" \
        "$cpu_time" "$cpu_usage" "$speedup" "$compression_ratio" >> "$RESULTS_FILE"
    
    # Save data for plotting
    echo "$thread_count $speedup $cpu_usage $real_time" >> "data/${type}_results.txt"
}

# Generate report header
generate_report_header() {
    echo "# Huffman Coding Performance Test Report" > "$RESULTS_FILE"
    echo "Generated at: $(date '+%Y-%m-%d %H:%M:%S')" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    echo "## Test Environment" >> "$RESULTS_FILE"
    echo "- CPU: $(lscpu | grep 'Model name' | cut -d ':' -f2 | xargs)" >> "$RESULTS_FILE"
    echo "- Memory: $(free -h | awk '/^Mem:/{print $2}')" >> "$RESULTS_FILE"
    echo "- OS: $(uname -s) $(uname -r)" >> "$RESULTS_FILE"
    echo "- Compiler: $(g++ --version | head -n1)" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
}

# Generate type report section
generate_type_report() {
    local type=$1
    echo "## ${type^} Data Test Results" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    echo "### Time Metrics Explanation" >> "$RESULTS_FILE"
    echo "1. Program Internal Measurement (test_compression):" >> "$RESULTS_FILE"
    echo "   - Original Time: Compression time using unoptimized version (archive)" >> "$RESULTS_FILE"
    echo "   - Optimized Time: Compression time using OpenMP optimized version (modified_archive)" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    echo "2. System-level Measurement (time command):" >> "$RESULTS_FILE"
    echo "   - System Time (real): Actual elapsed time, includes all IO and system calls" >> "$RESULTS_FILE"
    echo "   - CPU Time (user+sys): Total CPU time used by all threads" >> "$RESULTS_FILE"
    echo "   - CPU Usage: CPU Time/System Time * 100%" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
}

# Generate thread report section
generate_thread_report() {
    local thread_count=$1
    echo "#### Thread Count: $thread_count" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    echo "| Size | Input(MB) | Original(MB) | Parallel(MB) | Original(s) | Optimized(s) | System(s) | CPU(s) (Usage) | Speedup | Ratio |" >> "$RESULTS_FILE"
    echo "|------|-----------|--------------|--------------|-------------|--------------|-----------|----------------|---------|--------|" >> "$RESULTS_FILE"
}

# Generate plots using gnuplot
generate_plots() {
    local type=$1
    
    # Create gnuplot script for speedup
    cat > "data/${type}_speedup.gnuplot" << EOL
set terminal dumb 80 25
set title "${type} Data: Speedup vs Thread Count"
set xlabel "Thread Count"
set ylabel "Speedup"
plot "data/${type}_results.txt" using 1:2 with linespoints title "Speedup"
EOL

    # Create gnuplot script for CPU usage
    cat > "data/${type}_cpu.gnuplot" << EOL
set terminal dumb 80 25
set title "${type} Data: CPU Usage vs Thread Count"
set xlabel "Thread Count"
set ylabel "CPU Usage (%)"
plot "data/${type}_results.txt" using 1:3 with linespoints title "CPU Usage"
EOL

    # Create gnuplot script for processing time
    cat > "data/${type}_time.gnuplot" << EOL
set terminal dumb 80 25
set title "${type} Data: Processing Time vs Thread Count"
set xlabel "Thread Count"
set ylabel "Time (s)"
plot "data/${type}_results.txt" using 1:4 with linespoints title "Processing Time"
EOL

    # Generate plots
    echo "### ${type} Data Results" >> "$RESULTS_FILE"
    echo "\`\`\`" >> "$RESULTS_FILE"
    echo "Speedup vs Thread Count:" >> "$RESULTS_FILE"
    gnuplot "data/${type}_speedup.gnuplot" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    echo "CPU Usage vs Thread Count:" >> "$RESULTS_FILE"
    gnuplot "data/${type}_cpu.gnuplot" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    echo "Processing Time vs Thread Count:" >> "$RESULTS_FILE"
    gnuplot "data/${type}_time.gnuplot" >> "$RESULTS_FILE"
    echo "\`\`\`" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"

    # Clean up gnuplot scripts
    rm -f "data/${type}_speedup.gnuplot" "data/${type}_cpu.gnuplot" "data/${type}_time.gnuplot"
}

# Generate ASCII plots for visualization
generate_ascii_plots() {
    echo "## Performance Visualization" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    
    for type in "${TYPES[@]}"; do
        generate_plots "$type"
    done
    
    # Clean up temporary data files
    rm -f data/*_results.txt
}

# Compile all programs
compile_programs() {
    echo -e "${YELLOW}Compiling programs...${NC}"
    if ! make clean; then
        handle_error "make clean failed"
    fi
    if ! make all; then
        handle_error "make all failed"
    fi
    echo -e "${GREEN}Compilation complete${NC}"
}

# Main function
main() {
    # Parse command line arguments
    parse_args "$@"
    
    # Compile programs
    compile_programs
    
    # Check prerequisites
    check_prerequisites
    
    # Clean up previous temporary files
    cleanup
    
    # Generate report header
    generate_report_header
    
    # Run tests
    local total_tests=$((${#TYPES[@]} * ${#SIZES[@]} * ${#THREAD_COUNTS[@]}))
    local current_test=0
    
    for type in "${TYPES[@]}"; do
        generate_type_report "$type"
        > "data/${type}_results.txt"  # Create/clear results file
        
        for thread_count in "${THREAD_COUNTS[@]}"; do
            generate_thread_report "$thread_count"
            
            for size in "${SIZES[@]}"; do
                current_test=$((current_test + 1))
                echo -e "${YELLOW}Progress: $current_test / $total_tests${NC}"
                
                if ! run_test "$size" "$type" "$thread_count"; then
                    echo -e "${RED}Warning: Test failed - $type ${size}MB Threads $thread_count${NC}" >&2
                    continue
                fi
            done
            echo "" >> "$RESULTS_FILE"
        done
    done
    
    # Generate plots
    generate_ascii_plots
    
    # Final cleanup
    cleanup
    
    echo -e "${GREEN}Test completed! Results saved to $RESULTS_FILE${NC}"
}

# Capture Ctrl+C
trap 'echo -e "\n${RED}Test interrupted${NC}"; cleanup; exit 1' INT

# Run main function
main "$@"
