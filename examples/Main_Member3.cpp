// member3_main.cpp - Member 3 (Noise & Spike Filtration)
//
// A small main I use to run my part on a CSV file. This is the only place where
// I read files and print, because the data structure files must stay free of I/O.
// I read the CSV one line at a time and pass each reading into NoiseFilter as
// plain numbers (sensorId, timestamp, value), the same way the team's main.cpp will.
//
// CSV format: sensorId,timestamp,value   (first line is a header)
//
//   g++ -std=c++11 -Wall -Wextra -Iinclude examples/member3_main.cpp -o member3_demo
//   ./member3_demo data/member3_sample.csv

#include <cstdio>
#include <fstream>
#include <iostream>
#include "NoiseFilter.hpp"

int main(int argc, char* argv[]) {
    const char* path = argc > 1 ? argv[1] : "data/member3_sample.csv";
    std::ifstream file(path);
    if (!file) {
        std::cout << "Could not open " << path << "\n";
        return 1;
    }

    // One filter per sensor. The ranges are examples; we swap in the real
    // datasheet ranges once we know the exact sensors.
    NoiseFilter noise(5, 3.0f);
    noise.addSensor(1, -40.0f, 125.0f, 0.1f);   // temperature (C)
    noise.addSensor(2, 0.0f, 5.0f, 0.01f);      // voltage (V)
    noise.addSensor(3, -2.0f, 2.0f, 0.01f);     // accelerometer (g)

    std::cout << "sensor,timestamp,raw,median,score,spike,clean,normalized\n";

    char line[128];
    file.getline(line, sizeof(line));            // skip the header
    while (file.getline(line, sizeof(line))) {
        int sensorId;
        unsigned long timestamp;
        float value;
        if (std::sscanf(line, "%d,%lu,%f", &sensorId, &timestamp, &value) != 3) continue;

        FilterResult r;
        if (!noise.process(sensorId, timestamp, value, r)) {
            std::cout << "# unknown sensor " << sensorId << ", skipped\n";
            continue;
        }

        char out[160];
        std::snprintf(out, sizeof(out), "%d,%lu,%.2f,%.2f,%.2f,%s,%.2f,%.4f",
            r.sensorId, r.timestamp, r.raw, r.median, r.score,
            r.isSpike ? "yes" : "no", r.clean, r.normalized);
        std::cout << out << "\n";
    }

    std::cout << "# spikes removed: temperature=" << noise.spikesFound(1)
        << " voltage=" << noise.spikesFound(2)
        << " accelerometer=" << noise.spikesFound(3) << "\n";
    return 0;
}