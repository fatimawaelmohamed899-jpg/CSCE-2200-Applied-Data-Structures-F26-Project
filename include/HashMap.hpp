#pragma once
#include <cstdio>

// Information stored for each sensor
// Statistics and alarm calculations are added in Milestone 2
struct SensorStats {
    int sampleCount;
    double sum;
    double minimum;
    double maximum;
    double lowThreshold;
    double highThreshold;
    bool alarmActive;

    // Avoid division by zero when no readings exist
    double mean() const {
        if (sampleCount == 0)
            return 0.0;

        return sum / sampleCount;
    }
};

// Fixed-size hash map: sensor ID -> sensor information
// Uses linear probing to handle collisions
class SensorHashMap {
public:
    static const int capacity = 11;
    static const int maxLength = 20;

private:
    static const int empty = 0;
    static const int occupied = 1;
    static const int deleted = 2;

    // One slot in the hash map.
    struct Entry {
        char sensorId[maxLength];
        SensorStats stats;
        int state;
    };

    Entry table[capacity];
    int used;

    // Check that the ID exists, is not empty, and fits
    static bool validKey(const char* id) {
        if (id == nullptr || id[0] == '\0')
            return false;

        for (int i = 0; i < maxLength; i++) {
            if (id[i] == '\0')
                return true;
        }

        return false;
    }

    // Compare two IDs character by character
    static bool sameId(const char* first, const char* second) {
        int i = 0;

        while (first[i] != '\0' && second[i] != '\0') {
            if (first[i] != second[i])
                return false;

            i++;
        }

        // Both IDs must end at the same position
        return first[i] == second[i];
    }

    // Find the slot containing an ID return -1 when the ID is invalid or missing
    int findIndex(const char* id) const {
        if (!validKey(id))
            return -1;

        int start = hash(id);

        for (int step = 0; step < capacity; step++) {
            // go to zero after the last slot.
            int index = (start + step) % capacity;

            if (table[index].state == empty)
                return -1;

            if (table[index].state == occupied &&
                sameId(table[index].sensorId, id)) {
                return index;
            }

            // Keep searching past deleted slots or other IDs
        }

        return -1;
    }

public:
    // Prepare an empty map.
    SensorHashMap() : used(0) {
        for (int i = 0; i < capacity; i++) {
            table[i].state = empty;
            table[i].sensorId[0] = '\0';
            table[i].stats = {};
        }
    }

    // DJB2 string hash: combine character values into a starting slot
    // Public so tests can identify colliding IDs
    // Return -1 for invalid IDs
    int hash(const char* id) const {
        if (!validKey(id))
            return -1;

        unsigned long value = 5381;

        for (int i = 0; id[i] != '\0'; i++) {
            value = value * 33 + static_cast<unsigned char>(id[i]);
        }

        return static_cast<int>(value % capacity);
    }

    // Add a new sensor and reject invalid IDs, duplicates, and a full table.
    bool insert(const char* id, const SensorStats& stats) {
        if (!validKey(id))
            return false;

        if (findIndex(id) != -1)
            return false;

        if (used == capacity)
            return false;

        int start = hash(id);

        for (int step = 0; step < capacity; step++) {
            int index = (start + step) % capacity;

            // Empty and deleted slots can both be reused.
            if (table[index].state != occupied) {
                int i = 0;

                // Copy the ID into the map's own fixed array
                while (id[i] != '\0') {
                    table[index].sensorId[i] = id[i];
                    i++;
                }

                table[index].sensorId[i] = '\0';
                table[index].stats = stats;
                table[index].state = occupied;

                used++;
                return true;
            }
        }

        return false;
    }

    // Copy the sensor's information into result leave result unchanged if the sensor is missing
    bool get(const char* id, SensorStats& result) const {
        int index = findIndex(id);

        if (index == -1)
            return false;

        result = table[index].stats;
        return true;
    }

    // Check whether a sensor exists
    bool contains(const char* id) const {
        return findIndex(id) != -1;
    }

    // Replace an existing sensor's information
    bool update(const char* id, const SensorStats& stats) {
        int index = findIndex(id);

        if (index == -1)
            return false;

        table[index].stats = stats;
        return true;
    }

    // Remove a sensor while preserving collision searches
    bool remove(const char* id) {
        int index = findIndex(id);

        if (index == -1)
            return false;

        table[index].state = deleted;
        used--;

        return true;
    }

    // Return the number of sensors currently stored.
    int size() const {
        return used;
    }

    // Display the table for demonstration.
    void display() const {
        for (int i = 0; i < capacity; i++) {
            std::printf("Slot %2d: ", i);

            if (table[i].state == empty) {
                std::printf("empty\n");
            }
            else if (table[i].state == deleted) {
                std::printf("deleted\n");
            }
            else {
                std::printf(
                    "Sensor %s, samples %d\n",
                    table[i].sensorId,
                    table[i].stats.sampleCount
                );
            }
        }
    }
};
