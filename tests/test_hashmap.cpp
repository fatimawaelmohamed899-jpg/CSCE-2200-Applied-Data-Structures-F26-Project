#include <iostream>
#include <cassert>
#include "../include/HashMap.hpp"

// Check adding and finding a sensor
void testInsertAndGet() {
    static SensorHashMap sensors;

    SensorStats initial = {
        0, 0.0, 0.0, 0.0, 10.0, 50.0, false
    };

    SensorStats result = {};

    assert(sensors.size() == 0);
    assert(!sensors.get("TEMP_01", result));

    assert(sensors.insert("TEMP_01", initial));
    assert(sensors.contains("TEMP_01"));
    assert(sensors.get("TEMP_01", result));

    assert(result.sampleCount == 0);
    assert(result.lowThreshold == 10.0);
    assert(result.highThreshold == 50.0);
    assert(sensors.size() == 1);

    // Reject duplicate, empty, null, and oversized IDs
    assert(!sensors.insert("TEMP_01", initial));
    assert(!sensors.insert("", initial));
    assert(!sensors.insert(nullptr, initial));
    assert(!sensors.insert("THIS_ID_IS_FAR_TOO_LONG", initial));

    assert(sensors.size() == 1);

    std::cout << "[PASS] Insert and Get Test\n";
}

// Check replacing a sensor's statistics
void testUpdate() {
    static SensorHashMap sensors;

    SensorStats initial = {
        0, 0.0, 0.0, 0.0, 10.0, 50.0, false
    };

    SensorStats result = {};

    assert(initial.mean() == 0.0);
    assert(sensors.insert("TEMP_01", initial));

    SensorStats changed = initial;
    changed.sampleCount = 3;
    changed.sum = 75.0;
    changed.minimum = 20.0;
    changed.maximum = 30.0;

    assert(sensors.update("TEMP_01", changed));
    assert(sensors.get("TEMP_01", result));

    assert(result.sampleCount == 3);
    assert(result.sum == 75.0);
    assert(result.minimum == 20.0);
    assert(result.maximum == 30.0);
    assert(result.mean() == 25.0);

    assert(!sensors.update("MISSING", changed));

    std::cout << "[PASS] Update and Mean Test\n";
}

// These IDs collide with the current hash and capacity of 11
void testCollisionsAndRemoval() {
    static SensorHashMap sensors;

    SensorStats initial = {
        0, 0.0, 0.0, 0.0, 10.0, 50.0, false
    };

    SensorStats result = {};

    assert(sensors.hash("A") == 10);
    assert(sensors.hash("A") == sensors.hash("L"));
    assert(sensors.hash("L") == sensors.hash("W"));

    // A occupies slot 10; L and W wrap to slots 0 and 1
    assert(sensors.insert("A", initial));
    assert(sensors.insert("L", initial));
    assert(sensors.insert("W", initial));

    assert(sensors.get("A", result));
    assert(sensors.get("L", result));
    assert(sensors.get("W", result));

    assert(sensors.remove("A"));
    assert(!sensors.contains("A"));
    assert(sensors.size() == 2);

    // Searching must continue past the deleted slot
    assert(sensors.get("W", result));

    // A deleted slot must not allow duplicate keys
    assert(!sensors.insert("W", initial));

    // Reuse the deleted slot
    assert(sensors.insert("A", initial));
    assert(sensors.size() == 3);

    assert(!sensors.remove("MISSING"));

    std::cout << "[PASS] Collisions and Removal Test\n";
}

// Check full-table handling and reuse after deletion
void testFullTable() {
    static SensorHashMap sensors;

    SensorStats initial = {
        0, 0.0, 0.0, 0.0, 10.0, 50.0, false
    };

    SensorStats result = {};

    const char ids[SensorHashMap::capacity][SensorHashMap::maxLength] = {
        "S0", "S1", "S2", "S3", "S4", "S5",
        "S6", "S7", "S8", "S9", "S10"
    };

    for (int i = 0; i < SensorHashMap::capacity; i++) {
        assert(sensors.insert(ids[i], initial));
    }

    assert(sensors.size() == SensorHashMap::capacity);
    assert(!sensors.insert("EXTRA", initial));
    assert(!sensors.get("MISSING", result));

    assert(sensors.remove("S5"));
    assert(sensors.insert("EXTRA", initial));

    // Remove every remaining sensor
    for (int i = 0; i < SensorHashMap::capacity; i++) {
        if (i != 5)
            assert(sensors.remove(ids[i]));
    }

    assert(sensors.remove("EXTRA"));
    assert(sensors.size() == 0);

    // All slots are deleted; searches must still terminate
    assert(!sensors.get("MISSING", result));
    assert(sensors.insert("NEW", initial));

    std::cout << "[PASS] Full Table and Reuse Test\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Member 4: Hash Map Verification Suite\n";
    std::cout << "========================================\n";

    testInsertAndGet();
    testUpdate();
    testCollisionsAndRemoval();
    testFullTable();

    std::cout << "All Hash Map Tests Passed!\n";

    return 0;
}
