#ifndef TELEMETRY_COMPRESSOR_HPP
#define TELEMETRY_COMPRESSOR_HPP

#include <cstdint>
#include "DeltaEncoder.hpp"
#include "HuffmanTree.hpp"

#define MAX_SAMPLES 128

struct CompressionStats {
    uint8_t sampleCount;
    uint8_t uniqueCount;
    uint16_t rawBytes;
    uint16_t compressedBits;
    uint16_t compressedBytes;
    float compressionRatio;
};

class TelemetryCompressor {
private:
    DeltaEncoder deltaEncoder;
    HuffmanTree huffmanTree;

    int16_t deltas[MAX_SAMPLES];
    int16_t uniqueSymbols[MAX_HUFFMAN_SYMBOLS];
    uint16_t frequencies[MAX_HUFFMAN_SYMBOLS];

    // Walks the Huffman tree to sum (bit_depth * symbol_frequency) for all leaves
    uint16_t calculateTotalBits(uint8_t nodeIdx, uint8_t depth, const HuffmanNode* pool) {
        if (nodeIdx == NULL_INDEX) return 0;

        // If leaf node, bit length is depth (or 1 if there is only 1 unique symbol)
        if (pool[nodeIdx].leftIdx == NULL_INDEX && pool[nodeIdx].rightIdx == NULL_INDEX) {
            uint8_t bitLen = (depth == 0) ? 1 : depth;
            return bitLen * pool[nodeIdx].frequency;
        }

        // Otherwise sum left and right subtrees
        return calculateTotalBits(pool[nodeIdx].leftIdx, depth + 1, pool) +
               calculateTotalBits(pool[nodeIdx].rightIdx, depth + 1, pool);
    }

public:
    CompressionStats compress(const int16_t* rawSamples, uint8_t count) {
        CompressionStats stats = {0, 0, 0, 0, 0, 0.0f};
        if (!rawSamples || count == 0) return stats;
        if (count > MAX_SAMPLES) count = MAX_SAMPLES;

        // Step 1: Delta Encode the raw sensor samples
        deltaEncoder.reset();
        deltaEncoder.encodeBuffer(rawSamples, deltas, count);

        // Step 2: Count frequency of each unique delta value
        uint8_t uniqueCount = 0;
        for (uint8_t i = 0; i < count; ++i) {
            bool found = false;
            for (uint8_t j = 0; j < uniqueCount; ++j) {
                if (uniqueSymbols[j] == deltas[i]) {
                    frequencies[j]++;
                    found = true;
                    break;
                }
            }
            if (!found && uniqueCount < MAX_HUFFMAN_SYMBOLS) {
                uniqueSymbols[uniqueCount] = deltas[i];
                frequencies[uniqueCount] = 1;
                uniqueCount++;
            }
        }

        // Step 3: Build the static Huffman Tree
        huffmanTree.buildTree(uniqueSymbols, frequencies, uniqueCount);

        // Step 4: Calculate compressed size and ratio
        uint16_t totalBits = calculateTotalBits(huffmanTree.getRootIndex(), 0, huffmanTree.getNodePool());
        uint16_t rawBytes = count * sizeof(int16_t);
        uint16_t compressedBytes = (totalBits + 7) / 8; // Round bits up to bytes
        if (compressedBytes == 0) compressedBytes = 1;

        stats.sampleCount = count;
        stats.uniqueCount = uniqueCount;
        stats.rawBytes = rawBytes;
        stats.compressedBits = totalBits;
        stats.compressedBytes = compressedBytes;
        stats.compressionRatio = (float)rawBytes / (float)compressedBytes;

        return stats;
    }

    const int16_t* getDeltas() const { return deltas; }
};

#endif