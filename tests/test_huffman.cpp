#include <iostream>
#include <cassert>
#include "../include/HuffmanTree.hpp"
#include "../src/DeltaEncoder.cpp"

// ============================================================================
// TEST 1: Differential Delta Encoder Pre-Conditioning Test
// ============================================================================
void testDeltaEncoding() {
    // Instantiate static, zero-heap Delta Encoder
    DeltaEncoder encoder;
    
    // Raw incoming sensor telemetry stream (analog sample values)
    int16_t rawSamples[5] = {100, 102, 101, 105, 100};
    
    // Output buffer pre-allocated on stack to hold computed deltas
    int16_t deltas[5] = {0};

    // ------------------------------------------------------------------------
    // Execute Batch Delta Encoding over Raw Sample Buffer
    // ------------------------------------------------------------------------
    uint8_t count = encoder.encodeBuffer(rawSamples, deltas, 5);

    // Assert that the exact number of processed samples equals input length
    assert(count == 5);

    // ------------------------------------------------------------------------
    // Step-by-Step Differential Arithmetic Verification:
    // ------------------------------------------------------------------------
    assert(deltas[0] == 100);  // Sample 0: First sample passes raw baseline untouched (100 - 0 = 100)
    assert(deltas[1] == 2);    // Sample 1: Small positive drift step (102 - 100 = +2)
    assert(deltas[2] == -1);   // Sample 2: Minor negative step shift (101 - 102 = -1)
    assert(deltas[3] == 4);    // Sample 3: Sudden telemetry upward bump (105 - 101 = +4)
    assert(deltas[4] == -5);   // Sample 4: Signal drops back down to base (100 - 105 = -5)

    std::cout << "[PASS] Delta Encoder Test Passed!\n";
}

// ============================================================================
// TEST 2: Static Min-Heap Huffman Tree Building & Struct Size Assertions
// ============================================================================
void testHuffmanTree() {
    // Instantiate static pool Huffman Tree manager (Zero-STL, zero-malloc)
    HuffmanTree tree;
    
    // Unique delta symbol occurrences (pre-conditioned differential values)
    int16_t symbols[4] = {1, 2, 3, 4};
    
    // Corresponding frequency counts (how often each delta symbol appeared)
    uint16_t frequencies[4] = {50, 25, 15, 10};

    // ------------------------------------------------------------------------
    // Step 1: Construct Huffman Binary Tree via Array-Backed Min-Heap
    // ------------------------------------------------------------------------
    bool success = tree.buildTree(symbols, frequencies, 4);
    assert(success == true); // Confirm build algorithm completes successfully without pool overflow

    // ------------------------------------------------------------------------
    // Step 2: Root Node & Combined Frequency Integrity Check
    // ------------------------------------------------------------------------
    uint16_t rootIdx = tree.getRootIndex();
    assert(rootIdx != NULL_INDEX); // Root pointer index must point to a valid slot in nodePool

    // Fetch static node pool array to inspect root node properties directly
    const HuffmanNode* pool = tree.getNodePool();
    
    // Root frequency must equal the sum total of all input symbol frequencies (50 + 25 + 15 + 10 = 100)
    assert(pool[rootIdx].frequency == 100);

    // ------------------------------------------------------------------------
    // Step 3: Hard Embedded Constraint — Enforce 8-Byte Struct Packing Check
    // ------------------------------------------------------------------------
    // Critical RAM guard: Verifies that #pragma pack(push, 1) successfully prevents compiler byte padding.
    // HuffmanNode layout: symbol (2B) + frequency (2B) + leftIdx (2B) + rightIdx (2B) = EXACTLY 8 BYTES!
    assert(sizeof(HuffmanNode) == 8);

    std::cout << "[PASS] Huffman Tree & Min-Heap Test Passed (Struct Size: 8 Bytes)!\n";
}

// ============================================================================
// MAIN ENTRY POINT: Member 5 Milestone Verification Suite
// ============================================================================
int main() {
    std::cout << "=================================================\n";
    std::cout << "     Milestone 1 Verification Suite   \n";
    std::cout << "=================================================\n";

    // Run telemetry differential pre-conditioner test
    testDeltaEncoding();
    
    // Run static memory pool Huffman Tree & Min-Heap verification
    testHuffmanTree();

    std::cout << "-------------------------------------------------\n";
    std::cout << "     Milestone 1 Test Passes Successfully!   \n";
    
    return 0;
}
