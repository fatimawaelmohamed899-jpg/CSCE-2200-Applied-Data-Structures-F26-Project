// Memory minimization is a primary architecture goal for microcontroller target hardware
#ifndef DELTA_ENCODER_HPP
#define DELTA_ENCODER_HPP

#include <cstddef>
#include <cstdint>

// Converts raw telemetry into differential step values (3 bytes total RAM footprint)
class DeltaEncoder {
private:
    int16_t lastValue;    // Stores previous raw sample (2 bytes)
    bool isFirstSample;   // Stream baseline flag (1 byte)

public:
    // Initializes internal registers to baseline state
    DeltaEncoder() : lastValue(0), isFirstSample(true) {}

    // Resets encoder state for new stream transmission
    void reset() {
        lastValue = 0;
        isFirstSample = true;
    }

    // Encodes single sample into differential delta relative to previous sample
    int16_t encodeSample(int16_t currentSample) {
        // Pass first sample raw to establish base reference
        if (isFirstSample) {
            isFirstSample = false;
            lastValue = currentSample;
            return currentSample;
        }
        // Calculate difference and update stored sample register
        int16_t delta = currentSample - lastValue;
        lastValue = currentSample;
        return delta;
    }

    // Encodes array of raw samples into output delta array
    uint8_t encodeBuffer(const int16_t* input, int16_t* output, uint8_t length) {
        // Safety guard against null pointers and zero length
        if (!input || !output || length == 0) return 0;
        
        // Process each sample sequentially through single delta encoder
        for (uint8_t i = 0; i < length; ++i) {
            output[i] = encodeSample(input[i]);
        }
        return length;
    }
};

#endif // DELTA_ENCODER_HPP
