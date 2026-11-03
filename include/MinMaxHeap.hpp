/*
* Static-memory, Max-Heap and Min-Heap with lazy deletion,
* used to implement the running median filter.
* The window is divided into two heaps :
*MaxHeap stores the lower half.
*MinHeap stores the upper half.
*The median is obtained from the tops of the two heaps.
* Lazy deletion marks outdated samples instead of removing
* them immediately.Each entry has a value and an ID to
* identify outdated samples.
*The heaps use fixed-size arrays and valid sizes for balancing.
* If the array becomes full, stale entries are removed and
*the heap is rebuilt.
*/
#pragma once

// HeapEntry stores the reading's value and its ID to track its arrival order and to track the outdated reading
template <typename T>
struct HeapEntry {
    T value;
    unsigned long id;
};

//Largest value closest to the root (MaxHeap).
template <typename T>
struct MaxOrder {
    static bool before(const T& a, const T& b) { 
        return a > b; }
};

//Smallest value closest to the root (MinHeap).
template <typename T>
struct MinOrder {
    static bool before(const T& a, const T& b) { 
        return a < b; }
};
template <typename T, int CAPACITY, typename Order>
class LazyHeap {
private:
    HeapEntry<T> data[CAPACITY];  //Array that the heap uses of type HeapEntry ex:{50,1}
    int count = 0;                // Physical entries currently in data[].
    int staleCount = 0;           //How many of them are stale.
    unsigned long oldestValidId = 0;  // Entries with id below this are stale.

    // --- Navigation helpers (array <-> tree) -------------------------------
    static int parent(int i) { return (i - 1) / 2; }
    static int leftChild(int i) { return 2 * i + 1; }
    static int rightChild(int i) { return 2 * i + 2; }

    //True if data[a] must sit closer to the root than data[b]. 
    bool higher(int a, int b) const {
        return Order::before(data[a].value, data[b].value);
    }

    //An entry is stale once its sample has left the window.
    bool isStale(const HeapEntry<T>& e) const { return e.id < oldestValidId; }

    void swapAt(int a, int b) {
        HeapEntry<T> temp = data[a];
        data[a] = data[b];
        data[b] = temp;
    }

    //Move data[i] up if it is larger than its parent.Used after inserting at the end. O(log n): the tree height.
     
    void siftUp(int i) {
        while (i > 0 && higher(i, parent(i))) {
            swapAt(i, parent(i));
            i = parent(i);
        }
    }

 //Move data[i] down, swapping with its better child, until the heap rule holds. Used after the last element is moved to the root. O(log n).
    void siftDown(int i) {
        while (true) {
            int l = leftChild(i);
            int r = rightChild(i);
            int best = i;                // best = index of element with highest priority
            if (l < count && higher(l, best)) best = l;
            if (r < count && higher(r, best)) best = r;
            if (best == i) break;                      // rule holds, stop
            swapAt(i, best);
            i = best;
        }
    }

// Physically delete the root
    void removeRoot() {
        count--;
        if (count > 0) {
            data[0] = data[count];
            siftDown(0);
        }
    }

   // Drop every stale entry and rebuild the heap bottom-up. Keeps the heap usable when stale entries have filled the array.
  
    void compact() {
        int w = 0;
        for (int r = 0; r < count; r++) {
            if (!isStale(data[r])) data[w++] = data[r];
        }
        count = w;
        staleCount = 0;
        for (int i = count / 2 - 1; i >= 0; i--) siftDown(i);
    }
public:
    //Complexity: O(log n), or O(n) on the rare compaction.
    bool insert(T value, unsigned long id) {
        if (id < oldestValidId) return false;      // would break the stale counter
        if (count == CAPACITY) {
            if (staleCount == 0) return false;     // It is really full
            compact();
            if (count == CAPACITY) return false;
        }
        data[count] = HeapEntry<T>{ value, id };     // 1. place at the end
        siftUp(count);                             // 2. restore the heap rule
        count++;
        return true;
    }

    //This function returns the top of a heap and remove it ,Complexity: O(log n) amortized.
    HeapEntry<T> extractTop() {
        pruneTop();// function that removes stale value if in the root first
        HeapEntry<T> t = data[0];
        removeRoot();
        return t;
    } 
    //Returns the top element of the heap. Return the refernce without the ability of the caller to modify the refernce. 
    const HeapEntry<T>& top() {
        pruneTop();
        return data[0];
    }

    // Marks one expired entry as stale without removing it. o(1)
    void markStale(unsigned long newOldestValidId) {
        oldestValidId = newOldestValidId;
        staleCount++;
    }

   //Removes stale entries from the root until the root is valid. O(logw)
    void pruneTop() {
        while (count > 0 && isStale(data[0])) {
            removeRoot();
            staleCount--;
        }
    }
    // Returns the total number of physical entries.
    int size() const { return count; }

    // Returns the number of stale entries.
    int staleSize() const { return staleCount; }

    // Returns the number of valid entries.
    int validSize() const { return count - staleCount; }


    // Checks whether the heap has any valid entries.
    bool empty() const { return validSize() == 0; }

    // Checks whether every child follows the heap ordering rule. O(W), since it checks the entire heap.
    bool isValidHeap() const {
        for (int i = 1; i < count; i++)
            if (higher(i, parent(i))) return false;
        return true;
    }
};

// MaxHeap stores the lower half; root is its largest value.
template <typename T, int CAPACITY>
using MaxHeap = LazyHeap<T, CAPACITY, MaxOrder<T>>;

// MinHeap stores the upper half; root is its smallest value.
template <typename T, int CAPACITY>
using MinHeap = LazyHeap<T, CAPACITY, MinOrder<T>>;

/**
*Maintains a sliding - window median using two heaps.
* Invariants:
*1. Valid heap sizes differ by at most 1.
* 2. Every lower value is <= every upper value.
* 3. Total valid entries equal the window size.
*/

template <typename T, int WINDOW>
class MedianHeaps {
private:
    MaxHeap<T, 2 * WINDOW> lower;       ///< Smaller half; top = biggest of the small.
    MinHeap<T, 2 * WINDOW> upper;       ///< Bigger half; top = smallest of the big.
    unsigned char inUpper[WINDOW] = {}; ///< Which heap holds sample id: slot id % WINDOW.
    unsigned long nextId = 0;           ///< id given to the next sample.
    unsigned long oldestId = 0;         ///< id of the oldest sample in the window.
    int windowCount = 0;                ///< Samples currently in the window.
    
public:
    
    void insert(T value) {
        if (windowCount == WINDOW) {
            int slot = static_cast<int>(oldestId % WINDOW);
            if (inUpper[slot]) upper.markStale(oldestId + 1);
            else               lower.markStale(oldestId + 1);
            oldestId++;
            windowCount--;
        }

        int slot = static_cast<int>(nextId % WINDOW);
        bool toLower;
        if (!lower.empty())      toLower = (value <= lower.top().value);
        else if (!upper.empty()) toLower = (value <= upper.top().value);
        else                     toLower = true;   // both empty: first sample
        if (toLower) {
            lower.insert(value, nextId);
            inUpper[slot] = 0;
        }
        else {
            upper.insert(value, nextId);
            inUpper[slot] = 1;
        }
        nextId++;
        windowCount++;

        rebalance();
    }

    /** True once at least one sample has been inserted. */
    bool hasMedian() const { return windowCount > 0; }

    
    T median() {
        int lv = lower.validSize();
        int uv = upper.validSize();
        if (lv > uv) return lower.top().value;
        if (uv > lv) return upper.top().value;
        T a = lower.top().value;
        T b = upper.top().value;
        return a + (b - a) / 2;
    }

    int lowerValidSize() const { return lower.validSize(); }
    int upperValidSize() const { return upper.validSize(); }
    int windowSize() const { return windowCount; }

    // Checks both heap properties and the three median invariants.
    bool selfCheck() {
        if (!lower.isValidHeap() || !upper.isValidHeap()) return false;
        int d = lower.validSize() - upper.validSize();
        if (d > 1 || d < -1) return false;
        if (lower.validSize() + upper.validSize() != windowCount) return false;
        if (lower.validSize() > 0 && upper.validSize() > 0 &&
            lower.top().value > upper.top().value) return false;
        return true;
    }
    // Moves heap roots until their valid sizes differ by at most one.
    void rebalance() {
        while (lower.validSize() > upper.validSize() + 1) {
            HeapEntry<T> e = lower.extractTop();
            upper.insert(e.value, e.id);
            inUpper[e.id % WINDOW] = 1;
        }
        while (upper.validSize() > lower.validSize() + 1) {
            HeapEntry<T> e = upper.extractTop();
            lower.insert(e.value, e.id);
            inUpper[e.id % WINDOW] = 0;
        }
    }

};
