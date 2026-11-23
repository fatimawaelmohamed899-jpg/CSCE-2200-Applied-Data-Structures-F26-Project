#pragma once

// Member 4: Sensor Monitoring and Alarms
// Milestone 1: Store and find sensors using a fixed size hash map
// Milestone 2: Track readings and check threshold alarms for each sensor
// The application uses this map to access sensor data by its unique ID

// Statistics and alarm settings stored for one sensor
struct SensorStats {
    int sampleCount;
    double sum;
    double minimum;
    double maximum;
    double lowThreshold;
    double highThreshold;
    bool alarmActive;

    // Calculate the average reading return 0 when there are no readings
    double mean() const {
        if (sampleCount == 0)
            return 0.0;

        return sum / sampleCount;
    }
};

// Fixedsize hash map each sensor ID maps to its own statistics
// Linear probing handles collisions without STL containers
class SensorHashMap {
public:
    static const int capacity = 11;   // Maximum number of sensors
    static const int maxLength = 20;  // Sensor ID size including the null character

private:
    // A slot can be unused, occupied, or marked as deleted
    static const int empty = 0;
    static const int occupied = 1;
    static const int deleted = 2;

    struct Entry {
        char sensorId[maxLength];
        SensorStats stats;
        int state;
    };

    // All entries are stored in a fixed array (no dynamic memory)
    Entry table[capacity];
    int used;

    // Reject empty IDs and IDs that do not fit in the array
    bool validKey(const char* id) const {
        if (id == nullptr || id[0] == '\0')
            return false;

        for (int i = 0; i < maxLength; i++) {
            if (id[i] == '\0')
                return true;
        }

        return false;
    }

    // Compare two sensor IDs character by character
    bool sameId(const char* first, const char* second) const {
        int i = 0;

        while (first[i] != '\0' && second[i] != '\0') {
            if (first[i] != second[i])
                return false;

            i++;
        }

        return first[i] == second[i];
    }

    // Look up a sensor by probing from its hashed position
    // Continue past deleted slots and stop at an empty slot
    int findIndex(const char* id) const {
        if (!validKey(id))
            return -1;

        int start = hash(id);

        for (int step = 0; step < capacity; step++) {
            int index = (start + step) % capacity;

            if (table[index].state == empty)
                return -1;

            if (table[index].state == occupied &&
                sameId(table[index].sensorId, id)) {
                return index;
            }
        }

        return -1;
    }

public:
    // Initialize every slot as empty and clear its stored data
    SensorHashMap() : used(0) {
        for (int i = 0; i < capacity; i++) {
            table[i].state = empty;
            table[i].sensorId[0] = '\0';
            table[i].stats = {};
        }
    }

    // DJB2 converts an ID to a starting index in the table
    int hash(const char* id) const {
        if (!validKey(id))
            return -1;

        unsigned long value = 5381;

        for (int i = 0; id[i] != '\0'; i++) {
            value = value * 33 + id[i];
        }

        return static_cast<int>(value % capacity);
    }

    // Insert a new sensor. reject invalid, duplicate, or full table 
    // On collision try the following slots until one is available
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

            if (table[index].state != occupied) {
                int i = 0;

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

    // Copy a sensor's statistics into result and return false if not found
    bool get(const char* id, SensorStats& result) const {
        int index = findIndex(id);

        if (index == -1)
            return false;

        result = table[index].stats;
        return true;
    }

    // Check whether a sensor ID is already in the map.
    bool contains(const char* id) const {
        return findIndex(id) != -1;
    }

    // Replace the statistics stored for an existing sensor
    bool update(const char* id, const SensorStats& stats) {
        int index = findIndex(id);

        if (index == -1)
            return false;

        table[index].stats = stats;
        return true;
    }

    // Mark the sensor as deleted so later lookups can still probe past it
    bool remove(const char* id) {
        int index = findIndex(id);

        if (index == -1)
            return false;

        table[index].state = deleted;
        used--;

        return true;
    }

    // Return the number of currently stored sensors
    int size() const {
        return used;
    }

    // MILESTONE 2: Application logic built on the Milestone 1 hash map
    // These methods use insertion and lookup to manage sensor readings

    // Register a sensor with its allowed low and high readings
    // Start its statistics at zero and insert it into the hash map
    bool addSensor(const char* id, double low, double high) {
        if (low > high)
            return false;

        SensorStats data = {};
        data.lowThreshold = low;
        data.highThreshold = high;

        return insert(id, data);
    }

    // Find an existing sensor and change its allowed reading range
    bool setThresholds(const char* id, double low, double high) {
        if (low > high)
            return false;

        int index = findIndex(id);

        if (index == -1)
            return false;

        SensorStats& data = table[index].stats;

        data.lowThreshold = low;
        data.highThreshold = high;

        // New limits are checked when the next reading arrives.
        return true;
    }

    // Find the sensor and process one new reading
    // Update its minimum, maximum, count, sum, and alarm status
    // alarmNow reports whether this reading is outside the limits
    bool addReading(const char* id, double value, bool& alarmNow) {
        int index = findIndex(id);

        if (index == -1)
            return false;

        SensorStats& data = table[index].stats;

        // The first reading sets both the minimum and maximum
        if (data.sampleCount == 0) {
            data.minimum = value;
            data.maximum = value;
        }
        else {
            if (value < data.minimum)
                data.minimum = value;

            if (value > data.maximum)
                data.maximum = value;
        }

        data.sampleCount++;
        data.sum += value;

        // Trigger an alarm only when the reading is outside the range
        data.alarmActive =
            (value < data.lowThreshold ||
             value > data.highThreshold);

        alarmNow = data.alarmActive;

        return true;
    }
};


