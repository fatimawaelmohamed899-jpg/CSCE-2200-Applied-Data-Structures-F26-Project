// Memory minimization is a primary architecture goal for microcontroller target hardware
#ifndef HUFFMAN_TREE_HPP
#define HUFFMAN_TREE_HPP

#include <cstddef>
#include <cstdint>

// Maximum 64 unique symbols fits within 128 total node pool limit
#define MAX_HUFFMAN_SYMBOLS 64
#define NULL_INDEX 0xFF

// Ultra-Compact Node: 6 bytes total per node (uint8_t indices save 256B RAM overall)
#pragma pack(push, 1)
struct HuffmanNode {
    int16_t symbol;       // Delta telemetry sample value (2 bytes)
    uint16_t frequency;   // Symbol occurrence count (2 bytes)
    uint8_t leftIdx;      // Static node pool array index for left child (1 byte)
    uint8_t rightIdx;     // Static node pool array index for right child (1 byte)
};
#pragma pack(pop)

// Static min-heap priority queue using pool indices
class HuffmanMinHeap {
private:
    uint8_t heapArray[MAX_HUFFMAN_SYMBOLS]; // Stores pool indices (64 bytes RAM footprint)
    uint8_t heapSize;                       // Current heap size counter

    // Bubble down element to maintain min-heap property
    void heapifyDown(uint8_t idx, const HuffmanNode* pool) {
        uint8_t smallest = idx;
        uint8_t left = 2 * idx + 1;
        uint8_t right = 2 * idx + 2;

        if (left < heapSize && pool[heapArray[left]].frequency < pool[heapArray[smallest]].frequency)
            smallest = left;
        if (right < heapSize && pool[heapArray[right]].frequency < pool[heapArray[smallest]].frequency)
            smallest = right;

        if (smallest != idx) {
            uint8_t temp = heapArray[idx];
            heapArray[idx] = heapArray[smallest];
            heapArray[smallest] = temp;
            heapifyDown(smallest, pool);
        }
    }

    // Bubble up element to maintain min-heap property
    void heapifyUp(uint8_t idx, const HuffmanNode* pool) {
        while (idx > 0) {
            uint8_t parent = (idx - 1) / 2;
            if (pool[heapArray[parent]].frequency <= pool[heapArray[idx]].frequency) break;
            uint8_t temp = heapArray[idx];
            heapArray[idx] = heapArray[parent];
            heapArray[parent] = temp;
            idx = parent;
        }
    }

public:
    // Initializes empty heap
    HuffmanMinHeap() : heapSize(0) {}

    // Pushes node pool index into min-heap
    bool push(uint8_t poolIdx, const HuffmanNode* pool) {
        if (heapSize >= MAX_HUFFMAN_SYMBOLS) return false;
        heapArray[heapSize] = poolIdx;
        heapifyUp(heapSize, pool);
        heapSize++;
        return true;
    }

    // Pops smallest frequency node index from min-heap
    uint8_t pop(const HuffmanNode* pool) {
        if (heapSize == 0) return NULL_INDEX;
        uint8_t rootIdx = heapArray[0];
        heapArray[0] = heapArray[heapSize - 1];
        heapSize--;
        if (heapSize > 0) heapifyDown(0, pool);
        return rootIdx;
    }

    // Returns active heap element count
    uint8_t size() const { return heapSize; }
};

// Zero-STL Huffman Tree manager built on static pre-allocated pool
class HuffmanTree {
private:
    HuffmanNode nodePool[MAX_HUFFMAN_SYMBOLS * 2]; // 768 bytes static pool (6 bytes * 128 nodes)
    uint8_t poolIndex;                             // Free pool allocation index pointer
    uint8_t rootIndex;                             // Tree root node index

    // Allocates next free node from static node pool
    uint8_t createNode(int16_t symbol, uint16_t freq, uint8_t left = NULL_INDEX, uint8_t right = NULL_INDEX) {
        if (poolIndex >= (MAX_HUFFMAN_SYMBOLS * 2)) return NULL_INDEX;
        uint8_t idx = poolIndex++;
        nodePool[idx].symbol = symbol;
        nodePool[idx].frequency = freq;
        nodePool[idx].leftIdx = left;
        nodePool[idx].rightIdx = right;
        return idx;
    }

public:
    // Initializes static tree state registers
    HuffmanTree() : poolIndex(0), rootIndex(NULL_INDEX) {}

    // Resets pool allocation counters for fresh tree construction
    void reset() {
        poolIndex = 0;
        rootIndex = NULL_INDEX;
    }

    // Builds optimal Huffman tree from unique delta symbol frequency tables
    bool buildTree(const int16_t* symbols, const uint16_t* frequencies, uint8_t uniqueCount) {
        reset();
        if (uniqueCount == 0 || uniqueCount > MAX_HUFFMAN_SYMBOLS) return false;

        HuffmanMinHeap minHeap;

        // Insert leaf nodes into static node pool and heap
        for (uint8_t i = 0; i < uniqueCount; ++i) {
            uint8_t nodeIdx = createNode(symbols[i], frequencies[i]);
            if (nodeIdx == NULL_INDEX || !minHeap.push(nodeIdx, nodePool)) return false;
        }

        // Combine lowest-frequency nodes iteratively until single root remains
        while (minHeap.size() > 1) {
            uint8_t left = minHeap.pop(nodePool);
            uint8_t right = minHeap.pop(nodePool);

            uint8_t parent = createNode(-1, nodePool[left].frequency + nodePool[right].frequency, left, right);
            if (parent == NULL_INDEX || !minHeap.push(parent, nodePool)) return false;
        }

        rootIndex = minHeap.pop(nodePool);
        return rootIndex != NULL_INDEX;
    }

    // Returns array index pointing to tree root node
    uint8_t getRootIndex() const { return rootIndex; }

    // Returns pointer to static node pool storage array
    const HuffmanNode* getNodePool() const { return nodePool; }
};

#endif // HUFFMAN_TREE_HPP
