# Embedded Telemetry Compression & Processing Framework

An ultra-low memory, zero-heap, zero-STL C++ data processing and lossless compression system tailored for microcontrollers (e.g., Arduino ATmega328P, ESP32) with severe RAM constraints.

## 📌 System Architecture

+------------------+     +-------------------+     +-------------------+     +--------------------+
|  Sensor Input    | --> |  Delta Encoder    | --> | Huffman Min-Heap  | --> | Bit-Stream Output  |
| (Analog Sampling)|     | (Pre-Conditioner) |     | (Static Pool Tree)|     | (Arduino)  |
+------------------+     +-------------------+     +-------------------+     +--------------------+


## 👥 Milestone 1 Team Work Breakdown

| Team Member | Scope / Assigned Component | Responsibilities & Data Structures |
| :--- | :--- | :--- |
| **Member 1** | Packet Queue & Circular Buffer | Implements `CircularBuffer.hpp` and `Queue.hpp` for telemetry packet queueing with zero dynamic allocation. |
| **Member 2** | Monotonic Deque | Implements `MonotonicDeque.hpp` for $O(1)$ sliding-window min/max tracking over sensor streams. |
| **Member 3** | Min-Max Heap | Implements `MinMaxHeap.hpp` for double-ended priority management without dynamic memory. |
| **Member 4** | Static HashMap | Implements `HashMap.hpp` using open addressing for telemetry symbol and lookup tables. |
| **Member 5** | Delta Encoder & Huffman Tree | Implements header-only `HuffmanTree.hpp` (8-byte packed nodes, array-backed min-heap), `DeltaEncoder.cpp`.
## 🛠️ RAM Optimization Constraints

1. **Zero Dynamic Allocation:** Absolute avoidance of `malloc`, `free`, `new`, and `delete`.
2. **Zero STL Usage:** Hand-crafted data structures replacing standard C++ library headers (`<vector>`, `<queue>`, `<map>`).
3. **Struct Memory Packing:** Enforced `#pragma pack(push, 1)` ensuring `HuffmanNode` occupies exactly **8 bytes**.
4. **Pointer Elimination:** Replaces 64-bit pointers with 16-bit static array indices (`uint16_t`).

## 📂 Complete Project Directory Structure

CSCE2200_Telemetry_Framework/
├── include/
│   ├── CircularBuffer.hpp      # Member 1: Static ring buffer for raw sensor data
│   ├── HashMap.hpp             # Member 4: Static hash table (Open Addressing)
│   ├── HuffmanTree.hpp         # Member 5: Header-only Huffman Tree & Min-Heap
│   ├── MinMaxHeap.hpp          # Member 3: Double-ended priority queue
│   ├── MonotonicDeque.hpp      # Member 2: Sliding-window O(1) min/max tracking
│   └── Queue.hpp               # Member 1: Packet queue for serial transmission
├── src/
│   ├── DeltaEncoder.cpp        # Member 5: Differential Delta Encoder logic
│   ├── SensorSimulator.cpp     # Sensor data generation logic
│   ├── TelemetryCompressor.cpp # Compression pipeline wrapper
│   ├── TelemetryPipeline.cpp   # Complete end-to-end telemetry system
│   └── main.cpp                # Application entry point
├── tests/
│   ├── test_circular_buffer.cpp# Member 1 Unit Tests
│   ├── test_deque.cpp          # Member 2 Unit Tests
│   ├── test_hashmap.cpp        # Member 4 Unit Tests
│   ├── test_heaps.cpp          # Member 3 Unit Tests
│   └── test_huffman.cpp        # Member 5 Unit Tests 
├── CMakeLists.txt              # CMake build configuration
└── README.md                   # System documentation & build guidelines
