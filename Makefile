# Compiler settings
CXX ?= g++
CXXFLAGS ?= -std=c++14 -O2 -w

# OpenMP flags
OMPFLAGS = -fopenmp

# Output directory
BUILD_DIR = build

# Source files
SOURCES = data_generator.cpp Compressor.cpp Compressor_OpenMP.cpp test_compression.cpp

# Target executables
TARGETS = $(BUILD_DIR)/data_generator \
          $(BUILD_DIR)/archive \
          $(BUILD_DIR)/modified_archive \
          $(BUILD_DIR)/test_compression

# Default target: build all executables
all: $(TARGETS)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile data generator (no OpenMP needed)
$(BUILD_DIR)/data_generator: data_generator.cpp | $(BUILD_DIR)
	@echo "Compiling data_generator..."
	@$(CXX) $(CXXFLAGS) $< -o $@

# Compile original compression program (no OpenMP)
$(BUILD_DIR)/archive: Compressor.cpp | $(BUILD_DIR)
	@echo "Compiling archive..."
	@$(CXX) $(CXXFLAGS) $< -o $@

# Compile OpenMP-optimized version
$(BUILD_DIR)/modified_archive: Compressor_OpenMP.cpp | $(BUILD_DIR)
	@echo "Compiling modified_archive with OpenMP..."
	@$(CXX) $(CXXFLAGS) $(OMPFLAGS) $< -o $@

# Compile test program (needs OpenMP for timing)
$(BUILD_DIR)/test_compression: test_compression.cpp | $(BUILD_DIR)
	@echo "Compiling test_compression with OpenMP..."
	@$(CXX) $(CXXFLAGS) $(OMPFLAGS) $< -o $@

# Clean build artifacts and temporary files
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -f *.compressed
	@rm -f temp_input.txt temp_output.txt temp_error.txt
	@rm -rf test_data
	@rm -rf results

# Print compiler and flags information
info:
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "OpenMP flags: $(OMPFLAGS)"
	@echo "Source files: $(SOURCES)"
	@echo "Targets: $(TARGETS)"

# Declare phony targets (targets that don't create files)
.PHONY: all clean info