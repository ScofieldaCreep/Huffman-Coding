#include "progress_bar.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <omp.h>
#include <string>
#include <vector>

using namespace std;

// Function declarations for file I/O operations
void write_from_uChar(unsigned char, unsigned char&, int&, FILE*);
void write_from_uChar(unsigned char, unsigned char&, int&, vector<unsigned char>&);

// Utility functions for file and folder operations
int      this_is_not_a_folder(char*);
long int size_of_the_file(char*);
void     count_in_folder(string, long int*, long int&, long int&);

// File writing operations
void write_file_count(int, unsigned char&, int&, FILE*);
void write_file_size(long int, unsigned char&, int&, vector<unsigned char>&);
void write_file_name(char*, string*, unsigned char&, int&, vector<unsigned char>&);
void write_the_file_content(FILE*, long int, string*, unsigned char&, int&, vector<unsigned char>&);
void write_the_folder(string, string*, unsigned char&, int&, vector<unsigned char>&);

progress PROGRESS;

// Node structure for Huffman tree construction
struct ersel {
    ersel *       left, *right;
    long int      number;      // Frequency count
    unsigned char character;   // The byte value
    string        bit;         // Huffman code
};

// Comparison function for sorting nodes by frequency
bool erselcompare0(ersel a, ersel b) {
    return a.number < b.number;
}

// Recursively assign binary codes to the Huffman tree nodes
void assign_codes(ersel* node, const string& code) {
    if (!node) return;
    node->bit = code;
    assign_codes(node->left, code + "1");
    assign_codes(node->right, code + "0");
}

int main(int argc, char* argv[]) {
    long int number[256]  = {0};   // Array to store byte frequencies
    long int total_bits   = 0;     // Total bits in compressed output
    int      letter_count = 0;     // Number of unique bytes

    // Input validation
    if (argc == 1) {
        cout << "Missing file name" << endl << "try './archive {{file_name}}'" << endl;
        return 0;
    }

    string scompressed;
    FILE * original_fp, *compressed_fp;

    // Validate input files exist
    for (int i = 1; i < argc; i++) {
        if (this_is_not_a_folder(argv[i])) {
            original_fp = fopen(argv[i], "rb");
            if (!original_fp) {
                cout << argv[i] << " file does not exist" << endl << "Process has been terminated" << endl;
                return 0;
            }
            fclose(original_fp);
        }
    }

    scompressed = argv[1];
    scompressed += ".compressed";

    unsigned char x;
    long int      total_size = 0, size;
    total_bits += 16 + 9 * (argc - 1);

    // Initialize global counters for parallel reduction
    long int total_number[256] = {0};
    long int global_total_size = 0;
    long int global_total_bits = 0;

// Parallel region for counting byte frequencies across all input files
#pragma omp parallel
    {
        // Thread-local buffer and counters
        vector<char> local_buffer(8 * 1024 * 1024);   // 8MB buffer per thread
        long int     local_number[256] = {0};         // Local byte frequency counter
        long int     local_total_size  = 0;           // Local size accumulator
        long int     local_total_bits  = 0;           // Local bit count

// Parallel file processing using guided scheduling for better load balancing
// nowait allows threads to proceed without synchronization at loop end
#pragma omp for schedule(guided) nowait
        for (int current_file = 1; current_file < argc; current_file++) {
            // Count bytes in filename
            for (char* c = argv[current_file]; *c; c++) {
                local_number[(unsigned char)(*c)]++;
            }

            if (this_is_not_a_folder(argv[current_file])) {
                FILE* original_fp = fopen(argv[current_file], "rb");
                if (!original_fp) {
#pragma omp critical
                    { cerr << "Error: Cannot open file " << argv[current_file] << endl; }
                    continue;
                }

                // Process file in chunks using the thread-local buffer
                size_t size = size_of_the_file(argv[current_file]);
                local_total_size += size;
                local_total_bits += 64;

                size_t bytes_read;
                while ((bytes_read = fread(local_buffer.data(), 1, local_buffer.size(), original_fp)) > 0) {
// Vectorized byte counting
#pragma omp simd
                    for (size_t j = 0; j < bytes_read; j++) {
                        local_number[(unsigned char)local_buffer[j]]++;
                    }
                }
                fclose(original_fp);
            } else {
                string temp = argv[current_file];
                count_in_folder(temp, local_number, local_total_size, local_total_bits);
            }
        }

// Merge thread-local counters into global counters using critical section
#pragma omp critical
        {
            for (int i = 0; i < 256; i++) {
                total_number[i] += local_number[i];
            }
            global_total_size += local_total_size;
            global_total_bits += local_total_bits;
        }
    }

    // Copy final counts to the main array
    memcpy(number, total_number, sizeof(number));
    total_size += global_total_size;
    total_bits += global_total_bits;

    // Count unique bytes for Huffman tree construction
    for (long int* i = number; i < number + 256; i++) {
        if (*i) {
            letter_count++;
        }
    }

    // Initialize Huffman tree nodes array
    ersel  array[512];   // Maximum size for Huffman tree (2n-1 nodes for n unique bytes)
    int    array_size = 0;
    ersel* e          = array;
    for (long int* i = number; i < number + 256; i++) {
        if (*i) {
            e->right     = NULL;
            e->left      = NULL;
            e->number    = *i;
            e->character = i - number;
            e++;
            array_size++;
        }
    }
    sort(array, array + array_size, erselcompare0);

    // Build Huffman tree by repeatedly combining lowest frequency nodes
    ersel *min1 = array, *min2 = array + 1, *current = array + array_size;
    ersel *notleaf = array + array_size, *isleaf = array + 2;
    int    total_nodes = array_size;
    for (int i = 0; i < array_size - 1; i++) {
        current->number = min1->number + min2->number;
        current->left   = min1;
        current->right  = min2;
        min1->bit       = "1";
        min2->bit       = "0";
        current++;
        total_nodes++;

        // Select next minimum frequency nodes
        if (isleaf >= array + array_size) {
            min1 = notleaf;
            notleaf++;
        } else {
            if (isleaf->number < notleaf->number) {
                min1 = isleaf;
                isleaf++;
            } else {
                min1 = notleaf;
                notleaf++;
            }
        }

        if (isleaf >= array + array_size) {
            min2 = notleaf;
            notleaf++;
        } else if (notleaf >= current) {
            min2 = isleaf;
            isleaf++;
        } else {
            if (isleaf->number < notleaf->number) {
                min2 = isleaf;
                isleaf++;
            } else {
                min2 = notleaf;
                notleaf++;
            }
        }
    }

    // Assign Huffman codes starting from root
    ersel* root = current - 1;
    assign_codes(root, "");

    // Open output file and initialize bit buffer
    compressed_fp                   = fopen(&scompressed[0], "wb");
    int           current_bit_count = 0;
    unsigned char current_byte      = 0;

    // Write header information
    fwrite(&letter_count, 1, 1, compressed_fp);
    total_bits += 8;

    // Handle password protection
    {
        cout << "If you want a password write any number other than 0" << endl << "If you do not, write 0" << endl;
        int check_password;
        cin >> check_password;
        if (check_password) {
            string password;
            cout << "Enter your password (Do not use whitespaces): ";
            cin >> password;
            int password_length = password.length();
            if (password_length == 0) {
                cout << "You did not enter a password" << endl << "Process has been terminated" << endl;
                fclose(compressed_fp);
                remove(&scompressed[0]);
                return 0;
            }
            if (password_length > 100) {
                cout << "Password cannot contain more than 100 characters" << endl << "Process has been terminated" << endl;
                fclose(compressed_fp);
                remove(&scompressed[0]);
                return 0;
            }
            unsigned char password_length_unsigned = password_length;
            fwrite(&password_length_unsigned, 1, 1, compressed_fp);
            fwrite(&password[0], 1, password_length, compressed_fp);
            total_bits += 8 + 8 * password_length;
        } else {
            fwrite(&check_password, 1, 1, compressed_fp);
            total_bits += 8;
        }
    }

    // Write Huffman coding table
    char*         str_pointer;
    unsigned char len, current_character;
    string        str_arr[256];
    for (e = array; e < array + array_size; e++) {
        str_arr[(e->character)] = e->bit;   // Store Huffman codes for each byte
        len                     = e->bit.length();
        current_character       = e->character;

        write_from_uChar(current_character, current_byte, current_bit_count, compressed_fp);
        write_from_uChar(len, current_byte, current_bit_count, compressed_fp);
        total_bits += len + 16;

        str_pointer = &e->bit[0];
        while (*str_pointer) {
            if (current_bit_count == 8) {
                fwrite(&current_byte, 1, 1, compressed_fp);
                current_byte      = 0;
                current_bit_count = 0;
            }
            switch (*str_pointer) {
            case '1':
                current_byte <<= 1;
                current_byte |= 1;
                current_bit_count++;
                break;
            case '0':
                current_byte <<= 1;
                current_bit_count++;
                break;
            default:
                cout << "An error has occurred" << endl << "Compression process aborted" << endl;
                fclose(compressed_fp);
                remove(&scompressed[0]);
                return 1;
            }
            str_pointer++;
        }
    }

    // Pad last byte with zeros if needed
    if (total_bits % 8) {
        total_bits = (total_bits / 8 + 1) * 8;
    }

    // Display compression statistics
    cout << "The size of the sum of ORIGINAL files is: " << total_size << " bytes" << endl;
    cout << "The size of the COMPRESSED file will be: " << total_bits / 8 << " bytes" << endl;
    cout << "Compressed file's size will be [%" << 100 * ((float)total_bits / 8 / total_size) << "] of the original file" << endl;
    if (total_bits / 8 > total_size) {
        cout << endl << "COMPRESSED FILE'S SIZE WILL BE HIGHER THAN THE SUM OF ORIGINALS" << endl << endl;
    }
    cout << "If you wish to abort this process write 0 and press enter" << endl
         << "If you want to continue write any other number and press enter" << endl;
    int check;
    cin >> check;
    if (!check) {
        cout << endl << "Process has been aborted" << endl;
        fclose(compressed_fp);
        remove(&scompressed[0]);
        return 0;
    }

    // Set progress bar maximum
    PROGRESS.MAX = root->number;

    // Write file count to output
    write_file_count(argc - 1, current_byte, current_bit_count, compressed_fp);

    // Prepare thread-local buffers for parallel compression
    const int                     num_threads = omp_get_max_threads();
    vector<vector<unsigned char>> thread_buffers(num_threads);
    vector<int>                   buffer_bit_counts(num_threads, 0);
    vector<unsigned char>         current_bytes(num_threads, 0);

// Parallel compression of input files
#pragma omp parallel
    {
        const int thread_id = omp_get_thread_num();
        thread_buffers[thread_id].reserve(8 * 1024 * 1024);   // 8MB initial buffer per thread

// Process files in parallel using guided scheduling
#pragma omp for schedule(guided)
        for (int current_file = 1; current_file < argc; current_file++) {
            if (this_is_not_a_folder(argv[current_file])) {
                FILE* local_original_fp = fopen(argv[current_file], "rb");
                if (!local_original_fp) {
#pragma omp critical
                    { cerr << "Error: Cannot open file " << argv[current_file] << " for compression" << endl; }
                    continue;
                }

                long int size = size_of_the_file(argv[current_file]);

                // Pre-allocate buffer space based on estimated compression ratio
                thread_buffers[thread_id].reserve(thread_buffers[thread_id].size() + size / 4);

                // Write file marker and size
                current_bytes[thread_id] <<= 1;
                current_bytes[thread_id] |= 1;
                buffer_bit_counts[thread_id]++;
                write_file_size(size, current_bytes[thread_id], buffer_bit_counts[thread_id], thread_buffers[thread_id]);

                // Compress file content using Huffman codes
                write_the_file_content(
                    local_original_fp, size, str_arr, current_bytes[thread_id], buffer_bit_counts[thread_id], thread_buffers[thread_id]);

                fclose(local_original_fp);

                // Flush remaining bits in thread buffer
                if (buffer_bit_counts[thread_id] > 0) {
                    current_bytes[thread_id] <<= (8 - buffer_bit_counts[thread_id]);
                    thread_buffers[thread_id].push_back(current_bytes[thread_id]);
                    buffer_bit_counts[thread_id] = 0;
                    current_bytes[thread_id]     = 0;
                }
            }
        }
    }

    // Write compressed data using large buffer for efficiency
    const size_t          WRITE_BUFFER_SIZE = 8 * 1024 * 1024;   // 8MB write buffer
    vector<unsigned char> write_buffer;
    write_buffer.reserve(WRITE_BUFFER_SIZE);

    // Merge and write thread buffers sequentially
    for (int i = 0; i < num_threads; i++) {
        if (thread_buffers[i].empty()) continue;

        // Flush write buffer if it would overflow
        if (write_buffer.size() + thread_buffers[i].size() > WRITE_BUFFER_SIZE) {
            fwrite(write_buffer.data(), 1, write_buffer.size(), compressed_fp);
            write_buffer.clear();
        }
        write_buffer.insert(write_buffer.end(), thread_buffers[i].begin(), thread_buffers[i].end());
    }

    // Write any remaining data
    if (!write_buffer.empty()) {
        fwrite(write_buffer.data(), 1, write_buffer.size(), compressed_fp);
    }

    // Cleanup and finish
    fclose(compressed_fp);
    cout << endl << "Created compressed file: " << scompressed << endl;
    cout << "Compression is complete" << endl;

    return 0;
}

// Modified functions to support thread-local buffers

void write_from_uChar(unsigned char uChar, unsigned char& current_byte, int& current_bit_count, vector<unsigned char>& buffer) {
    for (int i = 0; i < 8; i++) {
        if (current_bit_count == 8) {
            buffer.push_back(current_byte);
            current_byte      = 0;
            current_bit_count = 0;
        }
        current_byte <<= 1;
        current_byte |= (uChar >> (7 - i)) & 1;
        current_bit_count++;
    }
}

void write_from_uChar(unsigned char uChar, unsigned char& current_byte, int& current_bit_count, FILE* fp_write) {
    for (int i = 0; i < 8; i++) {
        if (current_bit_count == 8) {
            fwrite(&current_byte, 1, 1, fp_write);
            current_byte      = 0;
            current_bit_count = 0;
        }
        current_byte <<= 1;
        current_byte |= (uChar >> (7 - i)) & 1;
        current_bit_count++;
    }
}

void write_file_count(int file_count, unsigned char& current_byte, int& current_bit_count, FILE* compressed_fp) {
    unsigned char temp = file_count % 256;
    write_from_uChar(temp, current_byte, current_bit_count, compressed_fp);
    temp = file_count / 256;
    write_from_uChar(temp, current_byte, current_bit_count, compressed_fp);
}

void write_file_size(long int size, unsigned char& current_byte, int& current_bit_count, vector<unsigned char>& buffer) {
    for (int i = 0; i < 8; i++) {
        unsigned char temp = (size >> ((7 - i) * 8)) & 0xFF;
        write_from_uChar(temp, current_byte, current_bit_count, buffer);
    }
}

void write_file_name(char* file_name, string* str_arr, unsigned char& current_byte, int& current_bit_count, vector<unsigned char>& buffer) {
    write_from_uChar(strlen(file_name), current_byte, current_bit_count, buffer);
    char* str_pointer;
    for (char* c = file_name; *c; c++) {
        str_pointer = &str_arr[(unsigned char)(*c)][0];
        while (*str_pointer) {
            if (current_bit_count == 8) {
                buffer.push_back(current_byte);
                current_byte      = 0;
                current_bit_count = 0;
            }
            switch (*str_pointer) {
            case '1':
                current_byte <<= 1;
                current_byte |= 1;
                current_bit_count++;
                break;
            case '0':
                current_byte <<= 1;
                current_bit_count++;
                break;
            default: cout << "An error has occurred" << endl << "Process has been aborted"; exit(2);
            }
            str_pointer++;
        }
    }
}

void write_the_file_content(FILE* original_fp, long int size, string* str_arr, unsigned char& current_byte, int& current_bit_count,
                            vector<unsigned char>& buffer) {
    unsigned char x;
    char*         str_pointer;
    for (long int i = 0; i < size; i++) {
        fread(&x, 1, 1, original_fp);
        str_pointer = &str_arr[x][0];
        while (*str_pointer) {
            if (current_bit_count == 8) {
                buffer.push_back(current_byte);
                current_byte      = 0;
                current_bit_count = 0;
            }
            switch (*str_pointer) {
            case '1':
                current_byte <<= 1;
                current_byte |= 1;
                current_bit_count++;
                break;
            case '0':
                current_byte <<= 1;
                current_bit_count++;
                break;
            default: cout << "An error has occurred" << endl << "Process has been aborted"; exit(2);
            }
            str_pointer++;
        }
    }
}

void write_the_folder(string path, string* str_arr, unsigned char& current_byte, int& current_bit_count, vector<unsigned char>& buffer) {
    FILE* original_fp;
    path += '/';
    DIR *          dir = opendir(&path[0]), *next_dir;
    string         next_path;
    struct dirent* current;
    int            file_count = 0;
    long int       size;
    while ((current = readdir(dir))) {
        if (current->d_name[0] == '.') {
            if (current->d_name[1] == 0) continue;
            if (current->d_name[1] == '.' && current->d_name[2] == 0) continue;
        }
        file_count++;
    }
    rewinddir(dir);
    // Writes fourth
    unsigned char temp = file_count % 256;
    write_from_uChar(temp, current_byte, current_bit_count, buffer);
    temp = file_count / 256;
    write_from_uChar(temp, current_byte, current_bit_count, buffer);

    while ((current = readdir(dir))) {   // if current is a file
        if (current->d_name[0] == '.') {
            if (current->d_name[1] == 0) continue;
            if (current->d_name[1] == '.' && current->d_name[2] == 0) continue;
        }

        next_path = path + current->d_name;
        if (this_is_not_a_folder(&next_path[0])) {
            original_fp = fopen(&next_path[0], "rb");
            fseek(original_fp, 0, SEEK_END);
            size = ftell(original_fp);
            rewind(original_fp);

            // Writing fifth
            current_byte <<= 1;
            current_byte |= 1;
            current_bit_count++;

            write_file_size(size, current_byte, current_bit_count, buffer);                                // writes sixth
            write_file_name(current->d_name, str_arr, current_byte, current_bit_count, buffer);            // writes seventh
            write_the_file_content(original_fp, size, str_arr, current_byte, current_bit_count, buffer);   // writes eighth
            fclose(original_fp);
        } else {   // if current is a folder
            // Writing fifth
            current_byte <<= 1;
            current_bit_count++;

            write_file_name(current->d_name, str_arr, current_byte, current_bit_count, buffer);   // writes seventh

            write_the_folder(next_path, str_arr, current_byte, current_bit_count, buffer);
        }
    }
    closedir(dir);
}

int this_is_not_a_folder(char* path) {
    DIR* temp = opendir(path);
    if (temp) {
        closedir(temp);
        return 0;
    }
    return 1;
}

long int size_of_the_file(char* path) {
    long int size;
    FILE*    fp = fopen(path, "rb");
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
    return size;
}

void count_in_folder(string path, long int* local_number, long int& local_total_size, long int& local_total_bits) {
    FILE* original_fp;
    path += '/';
    DIR *  dir = opendir(&path[0]), *next_dir;
    string next_path;
    local_total_size += 4096;
    local_total_bits += 16;   // for file_count
    struct dirent* current;
    while ((current = readdir(dir))) {
        if (current->d_name[0] == '.') {
            if (current->d_name[1] == 0) continue;
            if (current->d_name[1] == '.' && current->d_name[2] == 0) continue;
        }
        local_total_bits += 9;

        for (char* c = current->d_name; *c; c++) {   // counting usage frequency of bytes on the file name (or folder name)
            local_number[(unsigned char)(*c)]++;
        }

        next_path = path + current->d_name;

        if ((next_dir = opendir(&next_path[0]))) {
            closedir(next_dir);
            count_in_folder(next_path, local_number, local_total_size, local_total_bits);
        } else {
            long int      size;
            unsigned char x;
            local_total_size += size = size_of_the_file(&next_path[0]);
            local_total_bits += 64;

            original_fp = fopen(&next_path[0], "rb");

            for (long int j = 0; j < size; j++) {   // counting usage frequency of bytes inside the file
                fread(&x, 1, 1, original_fp);
                local_number[x]++;
            }
            fclose(original_fp);
        }
    }
    closedir(dir);
}