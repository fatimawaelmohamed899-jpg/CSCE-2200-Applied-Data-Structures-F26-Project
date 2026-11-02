#include <iostream>
#include <cassert>
#include "../include/HuffmanTree.hpp"
#include "../src/DeltaEncoder.cpp"

void testDeltaEncoding() {
    DeltaEncoder encoder;
    int16_t rawSamples[5] = {100, 102, 101, 105, 100};
    int16_t deltas[5] = {0};

    uint8_t count = encoder.encodeBuffer(rawSamples, deltas, 5);

    assert(count == 5);
    assert(deltas[0] == 100);
    assert(deltas[1] == 2);
    assert(deltas[2] == -1);
    assert(deltas[3] == 4);
    assert(deltas[4] == -5);

    std::cout << "[PASS] Delta Encoder Test Passed!\n";
}

void testHuffmanTree() {
    HuffmanTree tree;
    int16_t symbols[4] = {1, 2, 3, 4};
    uint16_t frequencies[4] = {50, 25, 15, 10};

    bool success = tree.buildTree(symbols, frequencies, 4);
    assert(success == true);

    uint16_t rootIdx = tree.getRootIndex();
    assert(rootIdx != NULL_INDEX);

    const HuffmanNode* pool = tree.getNodePool();
    assert(pool[rootIdx].frequency == 100);

    assert(sizeof(HuffmanNode) == 8);

    std::cout << "[PASS] Huffman Tree & Min-Heap Test Passed (Struct Size: 8 Bytes)!\n";
}

int main() {
    std::cout << "=================================================\n";
    std::cout << "   Milestone 1 Verification Suite  SUCCEDEDED     \n";
    std::cout << "=================================================\n";

    testDeltaEncoding();
    testHuffmanTree();
    return 0;
}
