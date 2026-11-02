/**
 * @file MinMaxHeap.hpp
 * @brief Static-memory Max-Heap and Min-Heap with lazy deletion, plus the
 *        two-heap balancing logic used by the running median filter.
 *
 * WHY THIS FILE EXISTS
 * --------------------
 * The sensor pipeline needs a sliding-window MEDIAN to remove spikes. A median
 * only needs the middle of the window, so the window is split into two halves:
 *   - lower half -> MaxHeap (its top is the biggest of the small values)
 *   - upper half -> MinHeap (its top is the smallest of the big values)
 * The median is then read from the tops in O(1); insert/remove cost O(log w).
 *
 * DESIGN DECISIONS (summary)
 * --------------------------
 * 1. No dynamic memory. Every heap is a plain array whose size is a template
 *    constant, as on real firmware (no new / malloc / std::vector).
 * 2. One heap class (LazyHeap) + a comparison policy, instead of two copies.
 *    MaxHeap and MinHeap are just LazyHeap with the comparison flipped.
 * 3. Lazy deletion. The sample that leaves the window may be deep inside a
 *    heap, and a heap can only remove its root cheaply. So we mark it stale
 *    and physically remove it only when it reaches the root.
 * 4. Each entry stores {value, id}. The id is the sample's arrival number.
 *    Samples leave in arrival order, so "stale" simply means id < oldestValidId.
 * 5. Sizes used for balancing are VALID sizes (size - staleCount), never the
 *    raw array sizes, because stale entries are still physically present.
 * 6. Stale entries still occupy space, so insert() compacts (drops all stale
 *    entries and re-heapifies in O(n)) if the array is full.
 */
#pragma once

 // ---------------------------------------------------------------------------
 // HeapEntry: what the heap stores for each sample.
 // ---------------------------------------------------------------------------

 /**
  * @brief One sample stored in a heap.
  * @tparam T Sensor value type (int, float, ...).
  *
  * value : the reading itself; this is what the heap orders by.
  * id    : increasing arrival number of the sample. Used only to decide whether
  *         the entry has left the window (stale). It never affects the order.
  */
    template <typename T>
struct HeapEntry {
    T value;
    unsigned long id;
};

// ---------------------------------------------------------------------------
// Ordering policies: the ONLY difference between a MaxHeap and a MinHeap.
// before(a, b) is true when a must sit closer to the root than b.
// ---------------------------------------------------------------------------

/** Largest value closest to the root (MaxHeap). */
template <typename T>
struct MaxOrder {
    static bool before(const T& a, const T& b) { return a > b; }
};

/** Smallest value closest to the root (MinHeap). */
template <typename T>
struct MinOrder {
    static bool before(const T& a, const T& b) { return a < b; }
};

// ---------------------------------------------------------------------------
// LazyHeap: array-backed binary heap with lazy deletion.
// ---------------------------------------------------------------------------

/**
 * @brief Fixed-capacity binary heap (array-backed complete binary tree).
 *
 * @tparam T        Value type.
 * @tparam CAPACITY Maximum number of physical entries (valid + stale).
 * @tparam Order    Ordering policy (MaxOrder or MinOrder).
 *
 * HEAP INVARIANT: for every index i > 0, the entry at parent(i) is NOT
 * ordered after the entry at i. For a MaxHeap: parent >= child.
 * The tree is stored in the array with no pointers:
 *      parent(i) = (i-1)/2     left(i) = 2i+1     right(i) = 2i+2
 *
 * LAZY-DELETION INVARIANT: staleCount == number of entries with
 * id < oldestValidId. validSize() == size() - staleCount.
 */
template <typename T, int CAPACITY, typename Order>
class LazyHeap {
public:
    /**
     * @brief Insert a sample.
     * @param value The reading.
     * @param id    Arrival number; must be >= the heap's oldest valid id.
     * @return false if the sample is already stale or the heap is truly full.
     *
     * If the array is full but contains stale entries, they are compacted away
     * first, so a stale-heavy heap never rejects a valid sample.
     * Complexity: O(log n), or O(n) on the rare compaction.
     */
    bool insert(T value, unsigned long id) {
        if (id < oldestValidId) return false;      // would break the stale counter
        if (count == CAPACITY) {
            if (staleCount == 0) return false;     // really full
            compact();
            if (count == CAPACITY) return false;
        }
        data[count] = HeapEntry<T>{ value, id };     // 1. place at the end
        siftUp(count);                             // 2. restore the heap rule
        count++;
        return true;
    }

    /**
     * @brief Remove and return the valid top (max for MaxHeap, min for MinHeap).
     * @pre validSize() > 0.
     * Stale roots are pruned first so a deleted sample is never returned.
     * Complexity: O(log n) amortized.
     */
    HeapEntry<T> extractTop() {
        pruneTop();
        HeapEntry<T> t = data[0];
        removeRoot();
        return t;
    }

    /**
     * @brief Look at the valid top without removing it.
     * @pre validSize() > 0.
     * Not const because it prunes stale roots first (that is the whole point
     * of lazy deletion: clean up only when the entry becomes visible).
     */
    const HeapEntry<T>& top() {
        pruneTop();
        return data[0];
    }

    /**
     * @brief Declare that ONE entry stored in THIS heap has left the window.
     * @param newOldestValidId id of the expired sample + 1.
     *
     * Nothing is moved or removed: O(1). The entry stays in the array and is
     * simply no longer counted. The caller must call this on the heap that
     * actually holds the expiring sample (MedianHeaps tracks that).
     */
    void markStale(unsigned long newOldestValidId) {
        oldestValidId = newOldestValidId;
        staleCount++;
    }

    /**
     * @brief Physically remove stale entries from the root until a valid root
     *        (or an empty heap) is reached.
     * Each removal is O(log n); each stale entry is removed at most once.
     */
    void pruneTop() {
        while (count > 0 && isStale(data[0])) {
            removeRoot();
            staleCount--;
        }
    }

    /** Physical entries in the array (valid + stale). */
    int size() const { return count; }

    /** Number of stale entries still physically in the array. */
    int staleSize() const { return staleCount; }

    /** Entries that are still inside the window. Use THIS for balancing. */
    int validSize() const { return count - staleCount; }

    /** True when no valid entry remains. */
    bool empty() const { return validSize() == 0; }

    /**
     * @brief Check the heap invariant over the whole array (used by tests).
     * @return true if no child is ordered before its parent.
     */
    bool isValidHeap() const {
        for (int i = 1; i < count; i++)
            if (higher(i, parent(i))) return false;
        return true;
    }

private:
    HeapEntry<T> data[CAPACITY];  ///< Fixed storage; the only memory the heap uses.
    int count = 0;                ///< Physical entries currently in data[].
    int staleCount = 0;           ///< How many of them are stale.
    unsigned long oldestValidId = 0;  ///< Entries with id below this are stale.

    // --- Navigation helpers (array <-> tree) -------------------------------
    static int parent(int i) { return (i - 1) / 2; }
    static int leftChild(int i) { return 2 * i + 1; }
    static int rightChild(int i) { return 2 * i + 2; }

    /** True if data[a] must sit closer to the root than data[b]. */
    bool higher(int a, int b) const {
        return Order::before(data[a].value, data[b].value);
    }

    /** An entry is stale once its sample has left the window. */
    bool isStale(const HeapEntry<T>& e) const { return e.id < oldestValidId; }

    void swapAt(int a, int b) {
        HeapEntry<T> tmp = data[a];
        data[a] = data[b];
        data[b] = tmp;
    }

    /**
     * Move data[i] up while it belongs above its parent.
     * Used after inserting at the end. O(log n): the tree height.
     */
    void siftUp(int i) {
        while (i > 0 && higher(i, parent(i))) {
            swapAt(i, parent(i));
            i = parent(i);
        }
    }

    /**
     * Move data[i] down, swapping with its better child, until the heap rule
     * holds. Used after the last element is moved to the root. O(log n).
     */
    void siftDown(int i) {
        while (true) {
            int l = leftChild(i);
            int r = rightChild(i);
            int best = i;                              // best = who belongs highest
            if (l < count && higher(l, best)) best = l;
            if (r < count && higher(r, best)) best = r;
            if (best == i) break;                      // rule holds, stop
            swapAt(i, best);
            i = best;
        }
    }

    /**
     * Physically delete the root regardless of staleness: move the last entry
     * to the root and sift it down. Does NOT touch staleCount; callers do.
     */
    void removeRoot() {
        count--;
        if (count > 0) {
            data[0] = data[count];
            siftDown(0);
        }
    }

    /**
     * Drop every stale entry and rebuild the heap bottom-up (Floyd's method,
     * O(n)). Keeps the heap usable when stale entries have filled the array.
     */
    void compact() {
        int w = 0;
        for (int r = 0; r < count; r++)
            if (!isStale(data[r])) data[w++] = data[r];
        count = w;
        staleCount = 0;
        for (int i = count / 2 - 1; i >= 0; i--) siftDown(i);
    }
};

/** Max-Heap: root is the LARGEST valid value. Holds the lower half. */
template <typename T, int CAPACITY>
using MaxHeap = LazyHeap<T, CAPACITY, MaxOrder<T>>;

/** Min-Heap: root is the SMALLEST valid value. Holds the upper half. */
template <typename T, int CAPACITY>
using MinHeap = LazyHeap<T, CAPACITY, MinOrder<T>>;

// ---------------------------------------------------------------------------
// MedianHeaps: the two-heap balancing logic (foundation of the median filter).
// ---------------------------------------------------------------------------

/**
 * @brief Sliding-window median built from a MaxHeap (lower half) and a
 *        MinHeap (upper half), kept balanced with lazy deletion.
 *
 * @tparam T      Value type.
 * @tparam WINDOW Number of most recent samples the median covers.
 *
 * INVARIANTS (checked by selfCheck()):
 *  A. |lower.validSize() - upper.validSize()| <= 1   (balance)
 *  B. every valid value in lower <= every valid value in upper (ordering)
 *  C. lower.validSize() + upper.validSize() == samples currently in window
 *
 * Each heap gets capacity 2*WINDOW: at most WINDOW entries are valid, the rest
 * may be stale entries waiting to reach the root. Compaction covers the rest.
 */
template <typename T, int WINDOW>
class MedianHeaps {
public:
    /**
     * @brief Add a new sample, expiring the oldest one if the window is full.
     *
     * Steps:
     *  1. If the window is full, mark the oldest sample stale in whichever
     *     heap holds it (O(1), nothing is moved).
     *  2. Insert the new sample into the half it belongs to (see routing rule
     *     below). The routing rule alone guarantees invariant B.
     *  3. Rebalance so the valid sizes differ by at most 1.
     *
     * ROUTING RULE (design decision, found by testing):
     *  - lower has valid entries: value <= lower top -> lower, else -> upper.
     *  - lower has NO valid entries: compare with the UPPER top instead.
     *    Why: lazy deletion means "lower has no valid entries" can happen while
     *    the upper heap is full of values. If we blindly put the sample into
     *    lower, it could be bigger than everything in upper and break B.
     */
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

    /**
     * @brief Median of the current window.
     * @pre hasMedian().
     * Bigger heap -> its top. Equal sizes -> average of the two tops, written
     * as a + (b-a)/2 so integer types cannot overflow.
     */
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

    /** Verify invariants A, B, C and both heap rules (used by tests). */
    bool selfCheck() {
        if (!lower.isValidHeap() || !upper.isValidHeap()) return false;
        int d = lower.validSize() - upper.validSize();
        if (d > 1 || d < -1) return false;
        if (lower.validSize() + upper.validSize() != windowCount) return false;
        if (lower.validSize() > 0 && upper.validSize() > 0 &&
            lower.top().value > upper.top().value) return false;
        return true;
    }

private:
    MaxHeap<T, 2 * WINDOW> lower;       ///< Smaller half; top = biggest of the small.
    MinHeap<T, 2 * WINDOW> upper;       ///< Bigger half; top = smallest of the big.
    unsigned char inUpper[WINDOW] = {}; ///< Which heap holds sample id: slot id % WINDOW.
    unsigned long nextId = 0;           ///< id given to the next sample.
    unsigned long oldestId = 0;         ///< id of the oldest sample in the window.
    int windowCount = 0;                ///< Samples currently in the window.

    /**
     * Move tops between heaps until valid sizes differ by at most 1.
     * Moving the top of one heap keeps invariant B: that top is the extreme
     * value on the border between the two halves. At most one move is needed
     * per insert, but a loop is safe and clear.
     */
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
