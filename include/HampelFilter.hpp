// HampelFilter.hpp - Member 3 (Noise & Spike Filtration)
//
// This is my spike remover for one sensor. It uses the running median from my
// two heaps. For every new reading I work out how far it is from the median,
// compared with how far normal readings usually are (the MAD). If it is too far,
// I call it a spike and send out the median instead. Normal readings pass through
// untouched. This score is the same as the modified z-score, so a limit of 3 is
// the usual Hampel filter and 3.5 is the usual modified z-score.
// No printing and no file reading in here, and only fixed arrays, so it runs on the Arduino.

#pragma once
#include "MinMaxHeap.hpp"

class HampelFilter {
public:
    // windowSize: how many recent readings to look at (odd, up to MAX_WINDOW)
    // spikeLimit: score above this counts as a spike (3 = Hampel, 3.5 = modified z-score)
    // madFloor:   the sensor's resolution, so tiny changes don't look like spikes
    //             when most readings in the window are exactly the same
    explicit HampelFilter(int windowSize = 5, float spikeLimit = 3.0f, float madFloor = 0.0f)
        : running(windowSize), limit(spikeLimit), minMad(madFloor) {
    }

    // Takes one raw reading and gives back the clean one.
    float filter(float raw) {
        // Keep a copy of the window's readings, because the MAD needs them.
        int w = running.windowSize();
        recent[next] = raw;
        next = (next + 1) % w;
        if (stored < w) stored++;

        // 1. Median of the window, from the two heaps.
        running.add(raw);
        med = running.median();

        // 2. MAD: the middle of all the distances from the median.
        mad = medianDistance(med);
        if (mad < minMad) mad = minMad;

        // 3. Score: how many "normal distances" away the new reading is.
        float distance = raw - med;
        if (distance < 0) distance = -distance;
        if (mad > 0)           score = distance / (1.4826f * mad);
        else if (distance > 0) score = 1e9f;   // MAD is 0 and the reading is different
        else                   score = 0.0f;

        // 4. Decide.
        spike = score > limit;
        if (spike) {
            spikes++;
            return med;        // spike: replace it with the median
        }
        return raw;            // normal: keep it exactly as it was
    }

    bool  lastWasSpike() const { return spike; }
    float lastMedian()   const { return med; }
    float lastMad()      const { return mad; }
    float lastScore()    const { return score; }
    long  spikesFound()  const { return spikes; }

private:
    RunningMedian running;           // my two heaps
    float limit;                     // spike limit
    float minMad;                    // smallest MAD allowed (sensor resolution)
    float recent[MAX_WINDOW] = {};   // the last readings, for the MAD
    int   stored = 0;                // how many slots of recent[] are filled
    int   next = 0;                  // where the next reading goes in recent[]
    float med = 0;                   // last median
    float mad = 0;                   // last MAD
    float score = 0;                 // last score
    bool  spike = false;             // was the last reading a spike?
    long  spikes = 0;                // how many spikes so far

    // Median of |reading - m| over the window. The window is tiny, so a simple
    // insertion sort is fine here.
    float medianDistance(float m) const {
        float d[MAX_WINDOW];
        for (int i = 0; i < stored; i++) {
            d[i] = recent[i] - m;
            if (d[i] < 0) d[i] = -d[i];
        }
        for (int i = 1; i < stored; i++) {
            float v = d[i];
            int j = i - 1;
            while (j >= 0 && d[j] > v) { d[j + 1] = d[j]; j--; }
            d[j + 1] = v;
        }
        if (stored % 2 == 1) return d[stored / 2];
        return (d[stored / 2 - 1] + d[stored / 2]) / 2.0f;
    }
};