/**
* @file test_heaps.cpp
* @brief Unit tests for MinMaxHeap.hpp(Milestone 1, Member 3).
*
* Plain C++ with a tiny CHECK macro(no test library needed).CHECK is used
* instead of assert() so the tests still fail loudly if someone compiles
* with - DNDEBUG.
*
*Build & run:
*g++ - std = c++17 - Wall - Wextra - Iinclude tests / test_heaps.cpp - o test_heaps && . / test_heaps
* /
#include <cstdio>
#include <cstdlib>
#include "MinMaxHeap.hpp"

#define CHECK(cond)                                                     \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("FAILED line %d: %s\n", __LINE__, #cond);       \
            std::exit(1);                                               \
        }                                                               \
    } while (0)

// ---------------------------------------------------------------------------
// 1. Heap invariant after insertions and extractions, and sorted extraction.
// ---------------------------------------------------------------------------
static void testMaxHeapOrder() {
    MaxHeap<int, 256> h;
    std::srand(12345);                       // fixed seed -> reproducible
    for (int i = 0; i < 200; i++) {
        CHECK(h.insert(std::rand() % 1000, i));
        CHECK(h.isValidHeap());              // invariant after EVERY insert
    }
    CHECK(h.size() == 200);

    int prev = h.extractTop().value;
    CHECK(h.isValidHeap());
    while (!h.empty()) {
        int cur = h.extractTop().value;
        CHECK(h.isValidHeap());              // invariant after EVERY extract
        CHECK(prev >= cur);                  // MaxHeap yields descending order
        prev = cur;
    }
    std::printf("  MaxHeap order/invariant ........ OK\n");
}

static void testMinHeapOrder() {
    MinHeap<int, 256> h;
    std::srand(54321);
    for (int i = 0; i < 200; i++) {
        CHECK(h.insert(std::rand() % 1000, i));
        CHECK(h.isValidHeap());
    }
    int prev = h.extractTop().value;
    CHECK(h.isValidHeap());
    while (!h.empty()) {
        int cur = h.extractTop().value;
        CHECK(h.isValidHeap());
        CHECK(prev <= cur);                  // MinHeap yields ascending order
        prev = cur;
    }
    std::printf("  MinHeap order/invariant ........ OK\n");
}

// ---------------------------------------------------------------------------
// 2. Edge cases: empty, single element, duplicates, full heap.
// ---------------------------------------------------------------------------
static void testEdgeCases() {
    MaxHeap<int, 4> h;
    CHECK(h.empty() && h.size() == 0 && h.validSize() == 0);

    CHECK(h.insert(42, 0));                  // single element
    CHECK(h.top().value == 42);
    CHECK(h.extractTop().value == 42);
    CHECK(h.empty());

    CHECK(h.insert(7, 1));                   // duplicates
    CHECK(h.insert(7, 2));
    CHECK(h.insert(7, 3));
    CHECK(h.isValidHeap());
    CHECK(h.extractTop().value == 7);
    CHECK(h.extractTop().value == 7);
    CHECK(h.extractTop().value == 7);
    CHECK(h.empty());

    CHECK(h.insert(1, 4));                   // full heap rejects insert
    CHECK(h.insert(2, 5));
    CHECK(h.insert(3, 6));
    CHECK(h.insert(4, 7));
    CHECK(!h.insert(5, 8));
    CHECK(h.size() == 4 && h.isValidHeap());
    std::printf("  Edge cases ..................... OK\n");
}

// ---------------------------------------------------------------------------
// 3. Lazy deletion.
// ---------------------------------------------------------------------------
static void testLazyDeletionRoot() {
    MaxHeap<int, 8> h;
    h.insert(50, 0);                         // oldest sample, will be the root
    h.insert(20, 1);
    h.insert(30, 2);
    CHECK(h.top().value == 50);

    h.markStale(1);                          // sample id 0 expired
    CHECK(h.size() == 3);                    // still physically there
    CHECK(h.validSize() == 2);               // but not counted
    CHECK(h.top().value == 30);              // top() pruned the stale root
    CHECK(h.size() == 2 && h.staleSize() == 0);
    CHECK(h.isValidHeap());
    std::printf("  Lazy deletion (root) ........... OK\n");
}

static void testLazyDeletionDeep() {
    MaxHeap<int, 8> h;
    h.insert(1, 0);                          // oldest sample but SMALLEST: sits deep
    h.insert(10, 1);
    h.insert(8, 2);
    h.insert(5, 3);
    CHECK(h.top().value == 10);

    h.markStale(1);                          // id 0 (value 1) expired
    CHECK(h.size() == 4);                    // stays in the array
    CHECK(h.validSize() == 3);               // not counted
    CHECK(h.top().value == 10);              // root is valid: nothing pruned
    CHECK(h.size() == 4);                    // still not removed

    CHECK(h.extractTop().value == 10);       // draining the heap...
    CHECK(h.extractTop().value == 8);
    CHECK(h.extractTop().value == 5);
    CHECK(h.empty());                        // stale 1 never comes out
    std::printf("  Lazy deletion (deep) ........... OK\n");
}

static void testCompactionWhenFull() {
    MaxHeap<int, 4> h;
    h.insert(1, 0);                          // deep stale-to-be
    h.insert(10, 1);
    h.insert(8, 2);
    h.insert(5, 3);
    CHECK(h.size() == 4);
    h.markStale(1);                          // id 0 expired, but not at the root

    CHECK(h.insert(7, 4));                   // full -> compacts instead of failing
    CHECK(h.size() == 4 && h.validSize() == 4 && h.staleSize() == 0);
    CHECK(h.isValidHeap());
    CHECK(!h.insert(6, 5));                  // now truly full
    CHECK(!h.insert(99, 0));                 // an already-stale id is rejected
    std::printf("  Compaction when full ........... OK\n");
}

// ---------------------------------------------------------------------------
// 4. Balance + median against a brute-force sorted-array median.
// ---------------------------------------------------------------------------

/** Reference median of the last min(n, W) values, by sorting a copy. */
static int bruteMedian(const int* history, int n, int W) {
    int k = n < W ? n : W;
    int tmp[64];
    for (int i = 0; i < k; i++) tmp[i] = history[n - k + i];
    for (int i = 1; i < k; i++) {            // insertion sort
        int v = tmp[i], j = i - 1;
        while (j >= 0 && tmp[j] > v) { tmp[j + 1] = tmp[j]; j--; }
        tmp[j + 1] = v;
    }
    if (k % 2 == 1) return tmp[k / 2];
    int a = tmp[k / 2 - 1], b = tmp[k / 2];
    return a + (b - a) / 2;                  // same formula as MedianHeaps
}

template <int W>
static void slidingMedianTest(int samples, unsigned seed) {
    static int history[5000];
    MedianHeaps<int, W> mh;
    std::srand(seed);
    for (int i = 0; i < samples; i++) {
        // mostly small noise, with an occasional huge spike
        int v = std::rand() % 201 - 100;
        if (std::rand() % 20 == 0) v = (std::rand() % 2 ? 100000 : -100000);
        history[i] = v;
        mh.insert(v);

        CHECK(mh.selfCheck());               // balance, ordering, heap rules
        int d = mh.lowerValidSize() - mh.upperValidSize();
        CHECK(d >= -1 && d <= 1);            // |difference| <= 1 after EVERY step
        CHECK(mh.median() == bruteMedian(history, i + 1, W));
    }
}

static void testSlidingMedian() {
    slidingMedianTest<1>(1000, 1);
    slidingMedianTest<3>(1000, 2);
    slidingMedianTest<4>(1000, 3);           // even window: averaging path
    slidingMedianTest<5>(1000, 4);
    slidingMedianTest<7>(1000, 5);
    slidingMedianTest<16>(3000, 6);
    slidingMedianTest<31>(3000, 7);
    std::printf("  Balance + sliding median ....... OK (1000s of steps, 7 window sizes)\n");
}

static void testSpikeRemoved() {
    MedianHeaps<int, 3> mh;
    const int in[] = { 25, 25, 26, 500, 25 };
    const int expected[] = { 25, 25, 25,  26, 26 };   // the 500 glitch never appears
    for (int i = 0; i < 5; i++) {
        mh.insert(in[i]);
        CHECK(mh.median() == expected[i]);
    }
    std::printf("  Spike (500) filtered out ....... OK\n");
}

/**
 * Regression test for a real bug found during development: when the expiring
 * sample was the ONLY valid entry of the lower heap, the lower heap became
 * "empty" (only a stale entry left) while the upper heap still held values.
 * The new sample was then wrongly routed into the lower heap even though it
 * was bigger than everything in the upper heap, which broke the ordering rule
 * (every lower value <= every upper value) and gave a wrong median.
 */
static void testLowerEmptiedByExpiry() {
    MedianHeaps<int, 3> mh;
    mh.insert(-40);                          // window [-40]
    mh.insert(-2);                           // window [-40, -2]
    mh.insert(53);                           // window [-40, -2, 53]
    CHECK(mh.median() == -2);
    mh.insert(44);                           // -40 expires; window [-2, 53, 44]
    CHECK(mh.selfCheck());
    CHECK(mh.median() == 44);                // sorted [-2, 44, 53] -> 44
    std::printf("  Lower emptied by expiry ........ OK\n");
}

int main() {
    std::printf("Running heap tests...\n");
    testMaxHeapOrder();
    testMinHeapOrder();
    testEdgeCases();
    testLazyDeletionRoot();
    testLazyDeletionDeep();
    testCompactionWhenFull();
    testSlidingMedian();
    testSpikeRemoved();
    testLowerEmptiedByExpiry();
    std::printf("Memory: MedianHeaps<float,16> uses %zu bytes (all static)\n",
        sizeof(MedianHeaps<float, 16>));
    std::printf("ALL TESTS PASSED\n");
    return 0;
}