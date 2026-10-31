# Sensor Stream Processing and Telemetry Compression

> **CSCE 2200: Applied Data Structures — B2 Project**  
> **American University in Cairo (AUC) — Fall 2026**


## 📌 Project Overview

This project simulates a resource-constrained microcontroller system that samples real-time telemetry data (temperature, accelerometer, voltage) at fixed rates into fixed-size buffers, filters noisy sensor streams in real time, monitors operational thresholds to trigger alarms, compresses telemetry payloads before transmission across a slow radio link, and manages outbound packet queues under bandwidth limits.

To comply with real firmware constraints, **no STL containers or external libraries were used** for core data structure operations. All memory is statically allocated without dynamic heap memory allocation (`malloc` or `new`).

The repository includes both a C++ core simulator engine and an **interactive Web Application Dashboard (`index.html`)** to visually demonstrate real-time ring buffering, sliding-window min/max tracking, running median filtering, hash table key-value lookups, and delta/Huffman compression ratios.

---

## 📂 Repository Structure

```text
.
├── index.html                      # Interactive Web Dashboard (Live visual simulator & analytics)
├── .github/
│   └── workflows/
│       └── build.yml               # CI build & test pipeline
├── docs/
│   ├── Architecture.md             # System architecture & component interaction
│   ├── Memory_Footprint_Report.md  # Detailed RAM/Flash usage breakdown (Zero dynamic allocation)
│   ├── Project_Report.pdf          # Final CSCE 2200 project report
│   └── plots/                      # Generated plots (Raw vs Filtered, Compression Ratios)
├── include/                        # Custom Data Structures (No STL / Static Allocations)
│   ├── CircularBuffer.hpp          # Member 1: Ring buffer with overwrite/drop policies
│   ├── MonotonicDeque.hpp          # Member 2: Sliding-window min/max queue (O(1) amortized)
│   ├── MinMaxHeap.hpp              # Member 3: Dual heaps for running median spike filtering
│   ├── HashMap.hpp                 # Member 4: Static hash map (Sensor ID -> stats & alarms)
│   ├── HuffmanTree.hpp             # Member 5: Min-heap Huffman encoder & Delta encoder
│   └── Queue.hpp                   # Member 1/5: Outgoing packet queue with bandwidth limits
├── src/                            # Application Logic & Modules
│   ├── main.cpp                    # CLI interactive menu entry point
│   ├── SensorSimulator.cpp         # CSV reader / Synthetic signal generator
│   ├── TelemetryPipeline.cpp       # Stream processing & threshold alarm dispatcher
│   ├── DeltaEncoder.cpp            # Delta differential encoding module
│   └── TelemetryCompressor.cpp     # Huffman compression & packetizer
├── data/                           # Sample Data & Test Signals
│   ├── sample_telemetry.csv        # Input CSV test dataset
│   └── expected_output.txt
├── tests/                          # Unit Tests for Custom Data Structures
│   ├── test_circular_buffer.cpp
│   ├── test_deque.cpp
│   ├── test_heaps.cpp
│   ├── test_hashmap.cpp
│   └── test_huffman.cpp
├── hardware/                       # Embedded Board Deployment (ESP32 / Arduino)
│   └── embedded_firmware.ino       # Hardware test sketch & RAM telemetry logger
├── benchmarks/                     # Performance Benchmarks
│   └── deque_vs_naive.cpp          # Benchmark: Monotonic Deque O(1) vs Naive O(w) scan
├── CMakeLists.txt                  # CMake build configuration
├── LICENSE
└── README.md                       # Main repository documentation
