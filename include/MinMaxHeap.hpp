// MinMaxHeap.hpp - Member 3 (Noise & Spike Filtration)
//
// I built a max-heap and a min-heap here, plus a running median that uses both.
// The smaller half of the window goes in the max-heap and the bigger half goes in
// the min-heap, so the median is always sitting at the two tops.
// When the window slides I don't dig the old reading out of the heap. I just mark
// it as gone and remove it later, once it reaches the top (lazy deletion).
// Everything uses fixed-size arrays, and there is no printing or file reading in
// here, so this same file can go straight onto the Arduino in the final phase.

#pragma once

// Biggest window we support. I kept it small because the Arduino has very
// little RAM. Changing this one number changes how much memory each filter uses.
const int MAX_WINDOW = 7;

// Each heap gets twice the window so there is room for readings that are
// marked gone but not removed yet.
const int HEAP_CAPACITY = 2 * MAX_WINDOW;

// One reading inside a heap.
struct Entry {
    float value;   // the sensor reading
    long  id;      // arrival number (0, 1, 2, ...) so I can tell when it has left the window
};

// A binary heap stored in an array. isMax = true gives a max-heap (largest on top),
// false gives a min-heap (smallest on top). The tree is read from the array with
// parent = (i-1)/2 and children = 2i+1 and 2i+2.
class Heap {
public:
    explicit Heap(bool isMaxHeap = true) : isMax(isMaxHeap) {}

    // Adds a reading. Returns false only if the heap is really full. O(log n).
    bool insert(float value, long id) {
        if (size == HEAP_CAPACITY) {        // full: first clear out the gone readings
            removeAllGone();
            if (size == HEAP_CAPACITY) return false;
        }
        data[size].value = value;           // put it at the end of the array
        data[size].id = id;
        siftUp(size);                       // then move it up to where it belongs
        size++;
        return true;
    }

    // Returns the top reading. Only call this when the heap is not empty.
    Entry top() {
        removeGoneFromTop();                // a gone reading must never be returned
        return data[0];
    }

    // Removes and returns the top reading. Only call when not empty. O(log n).
    Entry extractTop() {
        removeGoneFromTop();
        Entry t = data[0];
        removeRoot();
        return t;
    }

    // Lazy deletion: one reading stored in this heap has left the window.
    // I don't search for it, I just move the cut-off and count it. O(1).
    void markGone(long newOldestValidId) {
        oldestValidId = newOldestValidId;
        goneCount++;
    }

    int  validSize()  const { return size - goneCount; }   // readings still in the window
    int  storedSize() const { return size; }               // includes the gone ones
    bool empty()      const { return validSize() == 0; }

    // Used by the tests: checks every parent really belongs above its children.
    bool isValidHeap() const {
        for (int i = 1; i < size; i++)
            if (belongsAbove(i, parent(i))) return false;
        return true;
    }

private:
    Entry data[HEAP_CAPACITY];   // the heap itself
    int  size = 0;               // how many entries are stored (valid + gone)
    int  goneCount = 0;          // how many of those are gone
    long oldestValidId = 0;      // any entry with a smaller id is gone
    bool isMax;                  // max-heap or min-heap

    static int parent(int i) { return (i - 1) / 2; }
    static int leftChild(int i) { return 2 * i + 1; }
    static int rightChild(int i) { return 2 * i + 2; }

    bool isGone(int i) const { return data[i].id < oldestValidId; }

    // Should data[a] sit higher in the tree than data[b]?
    // This is the only line that differs between a max-heap and a min-heap.
    bool belongsAbove(int a, int b) const {
        return isMax ? data[a].value > data[b].value
            : data[a].value < data[b].value;
    }

    void swap(int a, int b) {
        Entry t = data[a];
        data[a] = data[b];
        data[b] = t;
    }

    // Keep swapping with the parent while this entry belongs above it.
    void siftUp(int i) {
        while (i > 0 && belongsAbove(i, parent(i))) {
            swap(i, parent(i));
            i = parent(i);
        }
    }

    // Keep swapping with the better child until the entry fits.
    void siftDown(int i) {
        while (true) {
            int best = i;
            int l = leftChild(i);
            int r = rightChild(i);
            if (l < size && belongsAbove(l, best)) best = l;
            if (r < size && belongsAbove(r, best)) best = r;
            if (best == i) return;          // it fits here
            swap(i, best);
            i = best;
        }
    }

    // Delete the root: the last entry jumps to the top, then sinks into place.
    void removeRoot() {
        size--;
        if (size > 0) {
            data[0] = data[size];
            siftDown(0);
        }
    }

    // Throw away gone readings that have floated up to the top.
    void removeGoneFromTop() {
        while (size > 0 && isGone(0)) {
            removeRoot();
            goneCount--;
        }
    }

    // Only used when the array fills up: keep the valid readings and rebuild
    // the heap from the bottom up. O(n), but it rarely happens.
    void removeAllGone() {
        int kept = 0;
        for (int i = 0; i < size; i++)
            if (!isGone(i)) data[kept++] = data[i];
        size = kept;
        goneCount = 0;
        for (int i = size / 2 - 1; i >= 0; i--) siftDown(i);
    }
};

// Median of the last `window` readings, using the two heaps.
// It also works on its own as a plain median filter: add(x) then median().
class RunningMedian {
public:
    // The window is kept between 1 and MAX_WINDOW. An odd size (3, 5, 7) works best.
    explicit RunningMedian(int windowSize = 5)
        : window(windowSize < 1 ? 1 : (windowSize > MAX_WINDOW ? MAX_WINDOW : windowSize)) {
    }

    // Adds one reading. O(log window) on average.
    void add(float value) {
        // Step 1: if the window is full, the oldest reading leaves. I only mark it gone.
        if (count == window) {
            if (inBig[oldestId % window]) big.markGone(oldestId + 1);
            else                          small.markGone(oldestId + 1);
            oldestId++;
            count--;
        }

        // Step 2: put the new reading in the right half.
        // If the small half has nothing valid left (this can happen because of lazy
        // deletion) I compare with the big half instead, so every small value stays
        // below every big value.
        bool goesSmall;
        if (!small.empty())    goesSmall = value <= small.top().value;
        else if (!big.empty()) goesSmall = value <= big.top().value;
        else                   goesSmall = true;   // very first reading

        if (goesSmall) small.insert(value, nextId);
        else           big.insert(value, nextId);
        inBig[nextId % window] = !goesSmall;
        nextId++;
        count++;

        // Step 3: keep both halves the same size (they can differ by at most 1).
        rebalance();
    }

    // Median of the current window. Only call after at least one add().
    float median() {
        if (small.validSize() > big.validSize()) return small.top().value;
        if (big.validSize() > small.validSize()) return big.top().value;
        return (small.top().value + big.top().value) / 2.0f;
    }

    int  windowSize()       const { return window; }
    int  readingsInWindow() const { return count; }
    int  smallSize()        const { return small.validSize(); }
    int  bigSize()          const { return big.validSize(); }
    bool heapsValid()       const { return small.isValidHeap() && big.isValidHeap(); }

private:
    Heap small{ true };              // max-heap holding the smaller half
    Heap big{ false };               // min-heap holding the bigger half
    bool inBig[MAX_WINDOW] = {};   // which half each window reading is in (slot = id % window)
    int  window;                   // window size
    long nextId = 0;               // id for the next reading
    long oldestId = 0;             // id of the oldest reading still in the window
    int  count = 0;                // how many readings are in the window right now

    // Move tops across until the halves differ by at most one.
    void rebalance() {
        while (small.validSize() > big.validSize() + 1) {
            Entry e = small.extractTop();
            big.insert(e.value, e.id);
            inBig[e.id % window] = true;
        }
        while (big.validSize() > small.validSize() + 1) {
            Entry e = big.extractTop();
            small.insert(e.value, e.id);
            inBig[e.id % window] = false;
        }
    }
};