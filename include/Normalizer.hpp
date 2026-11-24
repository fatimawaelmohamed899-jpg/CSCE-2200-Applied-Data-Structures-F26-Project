// Normalizer.hpp - Member 3 (Noise & Spike Filtration)
//
// After the spikes are gone I put every sensor on the same 0 to 1 scale.
// I use the sensor's physical range from its datasheet, not the min and max of
// the data, because a spike could become the max and squash everything else.
// The physical range never changes, so the same reading always gives the same number.
// It is O(1) and needs no memory besides the two limits.

#pragma once

struct Normalizer {
    float physMin;   // lowest value the sensor can measure
    float physMax;   // highest value the sensor can measure

    // The range only makes sense if max is bigger than min.
    bool valid() const { return physMax > physMin; }

    // Maps a reading to 0..1. Anything outside the range is clamped to 0 or 1.
    float normalize(float x) const {
        float n = (x - physMin) / (physMax - physMin);
        if (n < 0.0f) n = 0.0f;
        if (n > 1.0f) n = 1.0f;
        return n;
    }

    // Goes back from 0..1 to real units.
    float denormalize(float n) const { return physMin + n * (physMax - physMin); }

    // True if a working sensor could never give this reading.
    bool outOfRange(float x) const { return x < physMin || x > physMax; }
};