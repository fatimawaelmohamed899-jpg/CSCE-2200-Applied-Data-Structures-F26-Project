#ifndef DELTA_ENCODER_HPP
#define DELTA_ENCODER_HPP

#include <cstddef>
#include <cstdint>

class DeltaEncoder {
private:
    int16_t lastValue; 
    bool isFirstSample;

public:
    DeltaEncoder() : lastValue(0), isFirstSample(true) {}

    void reset() {
        lastValue = 0;
        isFirstSample = true;
    }

    int16_t encodeSample(int16_t currentSample) {
        if (isFirstSample) {
            isFirstSample = false;
            lastValue = currentSample;
            return currentSample;
        }
        int16_t delta = currentSample - lastValue;
        lastValue = currentSample;
        return delta;
    }

    uint8_t encodeBuffer(const int16_t* input, int16_t* output, uint8_t length) {
        if (!input || !output || length == 0) return 0;
        
        for (uint8_t i = 0; i < length; ++i) {
            output[i] = encodeSample(input[i]);
        }
        return length;
    }
};

#endif // DELTA_ENCODER_HPP
