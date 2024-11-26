#!/bin/bash

# Color definitions for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'  # No Color

# Check and install dependencies
check_dependencies() {
    echo -e "${YELLOW}Checking dependencies...${NC}"
    
    # Check for gnuplot
    if ! command -v gnuplot &> /dev/null; then
        echo -e "${YELLOW}gnuplot not found. Installing...${NC}"
        if ! sudo apt-get update && sudo apt-get install -y gnuplot; then
            echo -e "${RED}Error: Failed to install gnuplot${NC}"
            exit 1
        fi
        echo -e "${GREEN}gnuplot installed successfully${NC}"
    else
        echo -e "${GREEN}gnuplot is already installed${NC}"
    fi
}

# Run dependency check
check_dependencies

# Create necessary directories if they don't exist
mkdir -p build
mkdir -p test_data

# Compile the data generator program
make build/data_generator

# Define test parameters
# Start with small files to verify functionality
sizes=(1 10)  # Test with 1MB and 10MB files
types=("random" "repeating" "skewed")  # Different data distribution types
type_nums=(0 1 2)  # Corresponding numeric codes for data types

# Generate test data for each combination of size and type
for size in "${sizes[@]}"; do
    for i in "${!types[@]}"; do
        type=${types[$i]}
        type_num=${type_nums[$i]}
        output_file="test_data/${type}_${size}MB.dat"
        
        # Generate the test file
        echo "Generating ${size}MB ${type} data to ${output_file}"
        ./build/data_generator "$output_file" "$size" "$type_num"
        
        # Verify file size using Linux stat command
        actual_size=$(stat -c%s "$output_file")
        expected_size=$((size * 1024 * 1024))  # Convert MB to bytes
        
        # Display size comparison
        echo "Expected size: $expected_size bytes"
        echo "Actual size: $actual_size bytes"
        
        # Check if the file size matches expected size
        if [ "$actual_size" -eq "$expected_size" ]; then
            echo "✓ File size correct"
        else
            echo "✗ File size mismatch!"
        fi
        echo "----------------------------------------"
    done
done

# Display a hexdump preview of generated files for verification
echo "File content preview:"
for size in "${sizes[@]}"; do
    for type in "${types[@]}"; do
        file="test_data/${type}_${size}MB.dat"
        echo "== First 32 bytes of $file =="
        hexdump -C "$file" | head -n 2  # Show first 32 bytes in hex format
        echo
    done
done 