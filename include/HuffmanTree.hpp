// BESIDE MY DATA STRUCTURE IMPLEMENTATION, I AM LOOKING FORWARD TO MINIMIZE MY MEMORY USUAGE AS MUCH AS I CAN

#ifndef HUFFMAN_TREE_HPP
#define HUFFMAN_TREE_HPP

#include <cstddef>
#include <cstdint>

// Telemetry delta symbols fit inside 64 max unique symbol slots
#define MAX_HUFFMAN_SYMBOLS 64
#define NULL_INDEX 0xFFFF

// Compact Node: 8 bytes total per node 
#pragma pack(push, 1)
struct HuffmanNode {
    int16_t symbol;       // 2 bytes (Delta value)
    uint16_t frequency;   // 2 bytes (Sample count)
    uint16_t leftIdx;     // 2 bytes (Index in static pool instead of 8-byte pointer)
    uint16_t rightIdx;    // 2 bytes
};
#pragma pack(pop)

class HuffmanMinHeap {
private:
    uint16_t heapArray[MAX_HUFFMAN_SYMBOLS]; // Stores pool indices (128 bytes total)
    uint8_t heapSize;

    void heapifyDown(uint8_t idx, const HuffmanNode* pool) {
        uint8_t smallest = idx;
        uint8_t left = 2 * idx + 1;
        uint8_t right = 2 * idx + 2;

        if (left < heapSize && pool[heapArray[left]].frequency < pool[heapArray[smallest]].frequency)
            smallest = left;
        if (right < heapSize && pool[heapArray[right]].frequency < pool[heapArray[smallest]].frequency)
            smallest = right;

        if (smallest != idx) {
            uint16_t temp = heapArray[idx];
            heapArray[idx] = heapArray[smallest];
            heapArray[smallest] = temp;
            heapifyDown(smallest, pool);
        }
    }

    void heapifyUp(uint8_t idx, const HuffmanNode* pool) {
        while (idx > 0) {
            uint8_t parent = (idx - 1) / 2;
            if (pool[heapArray[parent]].frequency <= pool[heapArray[idx]].frequency) break;
            uint16_t temp = heapArray[idx];
            heapArray[idx] = heapArray[parent];
            heapArray[parent] = temp;
            idx = parent;
        }
    }

public:
    HuffmanMinHeap() : heapSize(0) {}

    bool push(uint16_t poolIdx, const HuffmanNode* pool) {
        if (heapSize >= MAX_HUFFMAN_SYMBOLS) return false;
        heapArray[heapSize] = poolIdx;
        heapifyUp(heapSize, pool);
        heapSize++;
        return true;
    }

    uint16_t pop(const HuffmanNode* pool) {
        if (heapSize == 0) return NULL_INDEX;
        uint16_t rootIdx = heapArray[0];
        heapArray[0] = heapArray[heapSize - 1];
        heapSize--;
        if (heapSize > 0) heapifyDown(0, pool);
        return rootIdx;
    }

    uint8_t size() const { return heapSize; }
};

class HuffmanTree {
private:
    HuffmanNode nodePool[MAX_HUFFMAN_SYMBOLS * 2]; // 1,024 bytes static RAM pool
    uint16_t poolIndex;
    uint16_t rootIndex;

    uint16_t createNode(int16_t symbol, uint16_t freq, uint16_t left = NULL_INDEX, uint16_t right = NULL_INDEX) {
        if (poolIndex >= (MAX_HUFFMAN_SYMBOLS * 2)) return NULL_INDEX;
        uint16_t idx = poolIndex++;
        nodePool[idx].symbol = symbol;
        nodePool[idx].frequency = freq;
        nodePool[idx].leftIdx = left;
        nodePool[idx].rightIdx = right;
        return idx;
    }

public:
    HuffmanTree() : poolIndex(0), rootIndex(NULL_INDEX) {}

    void reset() {
        poolIndex = 0;
        rootIndex = NULL_INDEX;
    }

    bool buildTree(const int16_t* symbols, const uint16_t* frequencies, uint8_t uniqueCount) {
        reset();
        if (uniqueCount == 0 || uniqueCount > MAX_HUFFMAN_SYMBOLS) return false;

        HuffmanMinHeap minHeap;

        for (uint8_t i = 0; i < uniqueCount; ++i) {
            uint16_t nodeIdx = createNode(symbols[i], frequencies[i]);
            if (nodeIdx == NULL_INDEX || !minHeap.push(nodeIdx, nodePool)) return false;
        }

        while (minHeap.size() > 1) {
            uint16_t left = minHeap.pop(nodePool);
            uint16_t right = minHeap.pop(nodePool);

            uint16_t parent = createNode(-1, nodePool[left].frequency + nodePool[right].frequency, left, right);
            if (parent == NULL_INDEX || !minHeap.push(parent, nodePool)) return false;
        }

        rootIndex = minHeap.pop(nodePool);
        return rootIndex != NULL_INDEX;
    }

    uint16_t getRootIndex() const { return rootIndex; }
    const HuffmanNode* getNodePool() const { return nodePool; }
};

#endif // HUFFMAN_TREE_HPP
