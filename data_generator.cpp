#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

// Generate random data with uniform distribution
void generateRandomData(ofstream& file, size_t size) {
    random_device              rd;
    mt19937                    gen(rd());
    uniform_int_distribution<> dis(0, 255);

    vector<char> buffer(1024 * 1024);   // Use 1MB buffer for efficient I/O
    for (size_t written = 0; written < size;) {
        size_t chunk = min(buffer.size(), size - written);
        for (size_t i = 0; i < chunk; ++i) {
            buffer[i] = static_cast<char>(dis(gen));
        }
        file.write(buffer.data(), chunk);
        written += chunk;
    }
}

// Generate data with repeating pattern for high compression ratio
void generateRepeatingData(ofstream& file, size_t size) {
    const string pattern = "HelloWorldThisIsARepeatingPattern";
    vector<char> buffer(1024 * 1024);   // Use 1MB buffer for efficient I/O

    size_t pattern_pos = 0;
    for (size_t written = 0; written < size;) {
        size_t chunk = min(buffer.size(), size - written);
        for (size_t i = 0; i < chunk; ++i) {
            buffer[i]   = pattern[pattern_pos];
            pattern_pos = (pattern_pos + 1) % pattern.length();
        }
        file.write(buffer.data(), chunk);
        written += chunk;
    }
}

// Generate data with skewed distribution using exponential distribution
void generateSkewedData(ofstream& file, size_t size) {
    random_device              rd;
    mt19937                    gen(rd());
    exponential_distribution<> dis(0.1);   // Lambda = 0.1 for moderate skew

    vector<char> buffer(1024 * 1024);   // Use 1MB buffer for efficient I/O
    for (size_t written = 0; written < size;) {
        size_t chunk = min(buffer.size(), size - written);
        for (size_t i = 0; i < chunk; ++i) {
            buffer[i] = static_cast<char>(static_cast<int>(dis(gen)) % 256);
        }
        file.write(buffer.data(), chunk);
        written += chunk;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <output_file> <size_in_MB> <type>" << endl;
        cerr << "Types: 0=random, 1=repeating, 2=skewed" << endl;
        return 1;
    }

    string output_file = argv[1];
    size_t size        = stoull(argv[2]) * 1024 * 1024;   // Convert MB to bytes
    int    type        = stoi(argv[3]);

    ofstream file(output_file, ios::binary);
    if (!file) {
        cerr << "Cannot create output file" << endl;
        return 1;
    }

    // Generate data based on the specified type
    switch (type) {
    case 0: generateRandomData(file, size); break;      // Uniform random data
    case 1: generateRepeatingData(file, size); break;   // Repeating pattern
    case 2: generateSkewedData(file, size); break;      // Skewed distribution
    default: cerr << "Unknown data type" << endl; return 1;
    }

    cout << "Generated " << argv[2] << "MB of ";
    cout << (type == 0 ? "random" : type == 1 ? "repeating" : "skewed") << " data to " << output_file << endl;

    return 0;
}