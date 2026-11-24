// NoiseFilter.hpp - Member 3 (Noise & Spike Filtration)
//
// This is the part the rest of the pipeline talks to. main.cpp reads the CSV
// line by line and hands me one reading at a time: (sensorId, timestamp, value).
// I find that sensor's own filter, remove the spike if there is one, normalize
// the clean value to 0..1, and fill in a result struct. I never print anything
// and never touch files; I only take numbers in and give numbers and true/false back.
// Each sensor gets its own window, so a spike on one sensor can't affect another.

#pragma once
#include "HampelFilter.hpp"
#include "Normalizer.hpp"

// How many sensors I can handle at once (temperature, voltage, accelerometer + 1 spare).
const int MAX_SENSORS = 4;

// Everything I work out for one reading.
struct FilterResult {
    int           sensorId;
    unsigned long timestamp;    // passed straight through so the next stages keep the timing
    float         raw;          // the reading as it came in
    float         clean;        // after spike removal (same as raw if it was normal)
    float         normalized;   // clean value on the 0..1 scale
    float         median;       // median of this sensor's window
    float         score;        // modified z-score of the raw reading
    bool          isSpike;      // true if I replaced the reading
    bool          outOfRange;   // true if the raw reading is impossible for this sensor
};

class NoiseFilter {
public:
    // Window size and spike limit are shared by all sensors.
    explicit NoiseFilter(int windowSize = 5, float spikeLimit = 3.0f)
        : window(windowSize), limit(spikeLimit) {
    }

    // Registers a sensor before its readings arrive.
    // resolution is the smallest step the sensor reports (for example 0.1 degrees).
    // Returns false if the table is full, the id is already used, or the range is wrong.
    bool addSensor(int sensorId, float physMin, float physMax, float resolution) {
        if (count == MAX_SENSORS) return false;
        if (find(sensorId) != -1) return false;
        Normalizer range = { physMin, physMax };
        if (!range.valid() || resolution < 0) return false;

        channels[count].sensorId = sensorId;
        channels[count].range = range;
        channels[count].filter = HampelFilter(window, limit, resolution);
        count++;
        return true;
    }

    // Processes one reading. Returns false if the sensor was never registered.
    bool process(int sensorId, unsigned long timestamp, float value, FilterResult& out) {
        int i = find(sensorId);
        if (i == -1) return false;

        Channel& ch = channels[i];
        float clean = ch.filter.filter(value);

        out.sensorId = sensorId;
        out.timestamp = timestamp;
        out.raw = value;
        out.clean = clean;
        out.normalized = ch.range.normalize(clean);
        out.median = ch.filter.lastMedian();
        out.score = ch.filter.lastScore();
        out.isSpike = ch.filter.lastWasSpike();
        out.outOfRange = ch.range.outOfRange(value);
        return true;
    }

    // How many spikes I have removed for one sensor (-1 if the sensor is unknown).
    long spikesFound(int sensorId) const {
        int i = find(sensorId);
        return i == -1 ? -1 : channels[i].filter.spikesFound();
    }

    int sensorCount() const { return count; }

private:
    // Everything one sensor needs.
    struct Channel {
        int          sensorId = -1;
        Normalizer   range = { 0.0f, 1.0f };
        HampelFilter filter;
    };

    Channel channels[MAX_SENSORS];   // fixed table, one slot per sensor
    int     count = 0;               // how many sensors are registered
    int     window;                  // window size for every sensor
    float   limit;                   // spike limit for every sensor

    // Finds a sensor's slot. With only a few sensors a simple loop is enough.
    // (Member 4's hash map does the fast id lookup for the alarm statistics.)
    int find(int sensorId) const {
        for (int i = 0; i < count; i++)
            if (channels[i].sensorId == sensorId) return i;
        return -1;
    }
};