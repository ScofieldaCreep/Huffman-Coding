#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <omp.h>
#include <string>
#include <vector>

void        compress_original(const char* input_file, const char* output_file, double& time_taken);
void        compress_modified(const char* input_file, const char* output_file, double& time_taken);
bool        compare_files(const char* file1, const char* file2);
std::string get_base_name(const char* file_path);
long        get_file_size(const char* file_path);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file1> [<input_file2> ...]" << std::endl;
        return 1;
    }

    // Vectors to store test data
    std::vector<std::string> input_files;
    std::vector<double>      original_times;
    std::vector<double>      modified_times;
    std::vector<long>        original_sizes;
    std::vector<long>        modified_sizes;
    std::vector<long>        input_sizes;

    for (int i = 1; i < argc; ++i) {
        const char* input_file = argv[i];
        input_files.push_back(input_file);

        std::string base_name       = get_base_name(input_file);
        std::string output_original = "original_" + base_name + ".compressed";
        std::string output_modified = "modified_" + base_name + ".compressed";

        double time_original = 0.0;
        double time_modified = 0.0;

        // Get input file size
        long input_size = get_file_size(input_file);

        // Compress using the original method
        compress_original(input_file, output_original.c_str(), time_original);

        // Get original compressed file size
        long original_size = get_file_size(output_original.c_str());

        // Compress using the modified method
        compress_modified(input_file, output_modified.c_str(), time_modified);

        // Get modified compressed file size
        long modified_size = get_file_size(output_modified.c_str());

        // Store the data
        original_times.push_back(time_original);
        modified_times.push_back(time_modified);
        original_sizes.push_back(original_size);
        modified_sizes.push_back(modified_size);
        input_sizes.push_back(input_size);

        // Compare the two output files
        if (compare_files(output_original.c_str(), output_modified.c_str())) {
            std::cout << "Test file: " << input_file << " - Compressed files are identical." << std::endl;
        } else {
            std::cout << "Test file: " << input_file << " - Compressed files differ." << std::endl;
        }

        // Output detailed report for this file only
        double speedup                    = original_times[i - 1] / modified_times[i - 1];
        double compression_ratio_original = (double)original_sizes[i - 1] / input_sizes[i - 1];
        double compression_ratio_modified = (double)modified_sizes[i - 1] / input_sizes[i - 1];

        std::cout << "\nDetailed Report:\n";
        std::cout << std::left << std::setw(20) << get_base_name(input_files[i - 1].c_str()) << std::right << std::setw(15)
                  << input_sizes[i - 1] << std::setw(15) << original_sizes[i - 1] << std::setw(15) << modified_sizes[i - 1] << std::setw(20)
                  << original_times[i - 1] << std::setw(20) << modified_times[i - 1] << std::setw(10) << std::fixed << std::setprecision(2)
                  << speedup << std::setw(15) << std::setprecision(4) << compression_ratio_original << std::setw(15)
                  << compression_ratio_modified << std::endl;
    }

    return 0;
}

void compress_original(const char* input_file, const char* output_file, double& time_taken) {
    double start_time = omp_get_wtime();

    // Write the inputs to a temporary file
    std::string   temp_input_file = "temp_input.txt";
    std::ofstream temp_input(temp_input_file);
    temp_input << "0\n1\n";
    temp_input.close();

    // Use input redirection to supply inputs to the compressor
    std::string command = "./build/archive \"" + std::string(input_file) + "\" < " + temp_input_file;
    int         ret     = system(command.c_str());

    // Remove the temporary input file
    std::remove(temp_input_file.c_str());

    double end_time = omp_get_wtime();
    time_taken      = end_time - start_time;
    if (ret != 0) {
        std::cerr << "Error running original compressor on " << input_file << std::endl;
    } else {
        // The output file is input_file + ".compressed"
        std::string compressed_file = std::string(input_file) + ".original.compressed";
        // Rename or copy the compressed file to output_file
        std::string copy_command = "mv \"" + std::string(input_file) + ".compressed\" \"" + compressed_file + "\" && mv \"" +
                                   compressed_file + "\" \"" + output_file + "\"";
        system(copy_command.c_str());
    }
}

void compress_modified(const char* input_file, const char* output_file, double& time_taken) {
    double start_time = omp_get_wtime();

    // Write the inputs to a temporary file
    std::string   temp_input_file = "temp_input.txt";
    std::ofstream temp_input(temp_input_file);
    temp_input << "0\n1\n";
    temp_input.close();

    // Use input redirection to supply inputs to the compressor
    std::string command = "./build/modified_archive \"" + std::string(input_file) + "\" < " + temp_input_file;
    int         ret     = system(command.c_str());

    // Remove the temporary input file
    std::remove(temp_input_file.c_str());

    double end_time = omp_get_wtime();
    time_taken      = end_time - start_time;
    if (ret != 0) {
        std::cerr << "Error running modified compressor on " << input_file << std::endl;
    } else {
        // The output file is input_file + ".compressed"
        std::string compressed_file = std::string(input_file) + ".modified.compressed";
        // Rename or copy the compressed file to output_file
        std::string copy_command = "mv \"" + std::string(input_file) + ".compressed\" \"" + compressed_file + "\" && mv \"" +
                                   compressed_file + "\" \"" + output_file + "\"";
        system(copy_command.c_str());
    }
}

bool compare_files(const char* file1, const char* file2) {
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);

    if (!f1.is_open() || !f2.is_open()) {
        std::cerr << "Error opening files for comparison." << std::endl;
        return false;
    }

    return std::equal(std::istreambuf_iterator<char>(f1), std::istreambuf_iterator<char>(), std::istreambuf_iterator<char>(f2));
}

std::string get_base_name(const char* file_path) {
    std::string path(file_path);
    size_t      last_slash = path.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        path = path.substr(last_slash + 1);
    }
    return path;
}

long get_file_size(const char* file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << file_path << " for size calculation." << std::endl;
        return -1;
    }
    long size = file.tellg();
    file.close();
    return size;
}