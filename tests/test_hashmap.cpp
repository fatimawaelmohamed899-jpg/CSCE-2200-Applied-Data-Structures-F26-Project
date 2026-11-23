#include <cassert>
#include <iostream>
#include "HashMap.hpp"

int main() {
    SensorHashMap sensors;
    bool alarm = false;
    SensorStats stats = {};

    // Register sensors and reject invalid entries.
    assert(sensors.addSensor("temp", 10, 35));
    assert(sensors.addSensor("humidity", 20, 80));
    assert(!sensors.addSensor("temp", 0, 100)); // Duplicate ID
    assert(!sensors.addSensor("bad", 90, 10)); // Low limit greater than high
    assert(sensors.size() == 2);

    // Normal readings and high/low threshold alarms.
    assert(sensors.addReading("temp", 20, alarm) && !alarm);
    assert(sensors.addReading("temp", 40, alarm) && alarm);
    assert(sensors.get("temp", stats));
    assert(stats.sampleCount == 2);
    assert(stats.minimum == 20);
    assert(stats.maximum == 40);
    assert(stats.mean() == 30);
    assert(stats.alarmActive);

    assert(sensors.addReading("temp", 30, alarm) && !alarm); // Alarm clears
    assert(sensors.addReading("humidity", 10, alarm) && alarm); // Low alarm
    assert(sensors.get("temp", stats));
    assert(stats.sampleCount == 3 && stats.mean() == 30);
    assert(!stats.alarmActive);

    // Change limits and check the next reading.
    assert(sensors.setThresholds("temp", 0, 100));
    assert(sensors.addReading("temp", 50, alarm) && !alarm);
    assert(!sensors.setThresholds("missing", 0, 100));
    assert(!sensors.setThresholds("temp", 100, 0));
    assert(!sensors.addReading("missing", 10, alarm));

    // Readings exactly on the boundaries should not trigger alarms.
    assert(sensors.addReading("temp", 0, alarm) && !alarm);
    assert(sensors.addReading("temp", 100, alarm) && !alarm);
    assert(sensors.addReading("temp", -1, alarm) && alarm);
    assert(sensors.addReading("temp", 101, alarm) && alarm);

    // "a" and "l" have the same DJB2 hash index (mod 11).
    // Both should still be stored and retrieved independently.
    assert(sensors.hash("a") == sensors.hash("l"));
    assert(sensors.addSensor("a", 0, 10));
    assert(sensors.addSensor("l", 0, 10));
    assert(sensors.contains("a") && sensors.contains("l"));
    assert(sensors.addReading("l", 7, alarm) && !alarm);
    assert(sensors.get("l", stats) && stats.mean() == 7);

    // Removing one colliding sensor should not hide the other.
    assert(sensors.remove("a"));
    assert(!sensors.contains("a"));
    assert(sensors.contains("l"));
    assert(!sensors.remove("a"));
    assert(sensors.addSensor("a", 0, 10)); // Reuse deleted slot
    assert(sensors.contains("a"));
    assert(sensors.size() == 4);

    std::cout << "All Milestone 2 tests passed!\n";
    return 0;
}

