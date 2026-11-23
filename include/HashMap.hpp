
#pragma once

// Information stored for each sensor
struct SensorStats {
    int sampleCount;
    double sum;
    double minimum;
    double maximum;
    double lowThreshold;
    double highThreshold;
    bool alarmActive;

    double mean() const {
        if (sampleCount == 0)
            return 0.0;

        return sum / sampleCount;
    }
};

// Fixed size hash map using linear probing
class SensorHashMap {
public:
    static const int capacity = 11;
    static const int maxLength = 20;

private:
    static const int empty = 0;
    static const int occupied = 1;
    static const int deleted = 2;

    struct Entry {
        char sensorId[maxLength];
        SensorStats stats;
        int state;
    };

    Entry table[capacity];
    int used;

    // Check if Sensor ID is valid
    bool validKey(const char* id) const {
        if (id == nullptr || id[0] == '\0')
            return false;

        for (int i = 0; i < maxLength; i++) {
            if (id[i] == '\0')
                return true;
        }

        return false;
    }

    // Compare two Sensor IDs
    bool sameId(const char* first, const char* second) const {
        int i = 0;

        while (first[i] != '\0' && second[i] != '\0') {
            if (first[i] != second[i])
                return false;

            i++;
        }

        return first[i] == second[i];
    }

    // Find the position of a sensor
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
    // Constructor
    SensorHashMap() : used(0) {
        for (int i = 0; i < capacity; i++) {
            table[i].state = empty;
            table[i].sensorId[0] = '\0';
            table[i].stats = {};
        }
    }

    // DJB2 hash function
    int hash(const char* id) const {
        if (!validKey(id))
            return -1;

        unsigned long value = 5381;

        for (int i = 0; id[i] != '\0'; i++) {
            value = value * 33 +
                static_cast<unsigned char>(id[i]);
        }

        return static_cast<int>(value % capacity);
    }

    // Insert a sensor
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

    // Get sensor statistics
    bool get(const char* id, SensorStats& result) const {
        int index = findIndex(id);

        if (index == -1)
            return false;

        result = table[index].stats;
        return true;
    }

    // Check if sensor exists
    bool contains(const char* id) const {
        return findIndex(id) != -1;
    }

    // Update sensor information
    bool update(const char* id, const SensorStats& stats) {
        int index = findIndex(id);

        if (index == -1)
            return false;

        table[index].stats = stats;
        return true;
    }

    // Remove a sensor
    bool remove(const char* id) {
        int index = findIndex(id);

        if (index == -1)
            return false;

        table[index].state = deleted;
        used--;

        return true;
    }

    // Number of stored sensors
    int size() const {
        return used;
    }

    // MILESTONE 2

    // Register a sensor with threshold limits
    bool addSensor(const char* id, double low, double high) {
        if (low > high)
            return false;

        SensorStats data = {};
        data.lowThreshold = low;
        data.highThreshold = high;

        return insert(id, data);
    }

    // Change sensor threshold limits
    bool setThresholds(const char* id, double low, double high) {
        if (low > high)
            return false;

        int index = findIndex(id);

        if (index == -1)
            return false;

        SensorStats& data = table[index].stats;

        data.lowThreshold = low;
        data.highThreshold = high;

        // New thresholds apply to future readings
        return true;
    }

    // Process a reading and update statistics and alarm
    bool addReading(const char* id, double value, bool& alarmNow) {
        int index = findIndex(id);

        if (index == -1)
            return false;

        SensorStats& data = table[index].stats;

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

        data.alarmActive =
            (value < data.lowThreshold ||
             value > data.highThreshold);

        alarmNow = data.alarmActive;

        return true;
    }
};
