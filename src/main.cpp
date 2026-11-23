#include <iostream>
#include <fstream>
#include <cstdint>
#include "../include/TelemetryCompressor.hpp"

int main() {
    TelemetryCompressor compressor;
    int16_t samples[MAX_SAMPLES] = {0};
    uint8_t count = 0;
    int choice = -1;

    while (choice != 0) {
        std::cout << "\n=== Telemetry Compression CLI ===\n";
        std::cout << "1. Load data from sensors.csv\n";
        std::cout << "2. Generate sample sensor data\n";
        std::cout << "3. Run Delta + Huffman Compression\n";
        std::cout << "4. Show Raw vs. Delta values\n";
        std::cout << "0. Exit\n";
        std::cout << "Choice: ";
        std::cin >> choice;

        if (choice == 1) {
            std::ifstream file("data/sample_telemetry.csv");
            if (!file.is_open()) {
                std::cout << "Could not open data/sample_telemetry.csv\n";
            } else {
                count = 0;
                // Skip header line if the first character is a letter (not a digit or '-')
                if (std::isalpha(file.peek())) {
                    file.ignore(1000, '\n');
                }
                double val;
                while (count < MAX_SAMPLES && (file >> val)) {
                    samples[count++] = (int16_t)val;
                    while (file.peek() == ',' || file.peek() == ' ') {
                        file.ignore();
                    }
                }
                std::cout << "Loaded " << (int)count << " samples.\n";
            }
        }
        else if (choice == 2) {
            // Simulate slow-changing temperature readings (e.g., 250 = 25.0 C)
            count = 20;
            int16_t temp = 250;
            for (uint8_t i = 0; i < count; ++i) {
                if (i % 3 == 0) temp += 1;
                samples[i] = temp;
            }
            std::cout << "Generated " << (int)count << " simulated samples.\n";
        }
        else if (choice == 3) {
            if (count == 0) {
                std::cout << "Load or generate data first!\n";
                continue;
            }
            CompressionStats stats = compressor.compress(samples, count);
            std::cout << "\n--- Compression Results ---\n";
            std::cout << "Samples         : " << (int)stats.sampleCount << "\n";
            std::cout << "Unique Deltas   : " << (int)stats.uniqueCount << "\n";
            std::cout << "Raw Size        : " << stats.rawBytes << " bytes\n";
            std::cout << "Compressed Size : " << stats.compressedBytes << " bytes (" << stats.compressedBits << " bits)\n";
            std::cout << "Ratio           : " << stats.compressionRatio << "x\n";
        }
        else if (choice == 4) {
            if (count == 0) {
                std::cout << "Load or generate data first!\n";
                continue;
            }
            compressor.compress(samples, count);
            const int16_t* deltas = compressor.getDeltas();
            std::cout << "\nRaw -> Delta:\n";
            for (uint8_t i = 0; i < count; ++i) {
                std::cout << samples[i] << " -> " << deltas[i] << "\n";
            }
        }
    }

    return 0;
}