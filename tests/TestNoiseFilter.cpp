// test_noise_filter.cpp - Member 3 (Noise & Spike Filtration)
//
// My tests for the heaps, the running median, the Hampel filter, the normalizer
// and the NoiseFilter module. Printing is fine here because this is a test
// program, not one of the data structure files.
//
//   g++ -std=c++11 -Wall -Wextra -Iinclude tests/test_noise_filter.cpp -o test_noise_filter
//   ./test_noise_filter

#include <cstdio>
#include <cstdlib>
#include "NoiseFilter.hpp"

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) { std::printf("FAILED line %d: %s\n", __LINE__, #cond); \
                       std::exit(1); }                                       \
    } while (0)

static bool near(float a, float b) { float d = a - b; return d < 1e-3f && d > -1e-3f; }

// Heap: max-heap comes out largest first, min-heap smallest first,
// and the heap rule holds after every insert and every removal.
static void testHeapOrder() {
    Heap mx(true), mn(false);
    std::srand(1);
    for (int i = 0; i < HEAP_CAPACITY; i++) {
        float v = (float)(std::rand() % 1000);
        CHECK(mx.insert(v, i));
        CHECK(mn.insert(v, i));
        CHECK(mx.isValidHeap() && mn.isValidHeap());
    }
    float prevMax = mx.extractTop().value, prevMin = mn.extractTop().value;
    while (!mx.empty()) {
        float a = mx.extractTop().value, b = mn.extractTop().value;
        CHECK(a <= prevMax && b >= prevMin);
        CHECK(mx.isValidHeap() && mn.isValidHeap());
        prevMax = a; prevMin = b;
    }
    std::printf("  Heap order (max and min) ........ OK\n");
}

// Lazy deletion: a gone reading stays stored but isn't counted or returned.
static void testLazyDeletion() {
    Heap h(true);
    h.insert(1, 0); h.insert(10, 1); h.insert(8, 2); h.insert(5, 3);
    h.markGone(1);                                  // reading 0 (value 1) left the window
    CHECK(h.storedSize() == 4 && h.validSize() == 3);
    CHECK(h.extractTop().value == 10);
    CHECK(h.extractTop().value == 8);
    CHECK(h.extractTop().value == 5);
    CHECK(h.empty());                               // the gone 1 never comes out

    Heap g(true);
    g.insert(50, 0); g.insert(20, 1);
    g.markGone(1);                                  // the gone reading is the top
    CHECK(g.top().value == 20);                     // so it gets removed for real
    CHECK(g.storedSize() == 1);
    std::printf("  Lazy deletion ................... OK\n");
}

// A full heap with gone readings cleans them out instead of refusing the insert.
static void testFullHeapCleansUp() {
    Heap f(true);
    for (int i = 0; i < HEAP_CAPACITY; i++) CHECK(f.insert((float)i, i));
    f.markGone(1);                                  // reading 0 is gone, deep in the heap
    CHECK(f.insert(7, HEAP_CAPACITY));
    CHECK(f.validSize() == HEAP_CAPACITY && f.isValidHeap());
    CHECK(!f.insert(8, HEAP_CAPACITY + 1));         // now it really is full
    std::printf("  Full heap cleans up gone ........ OK\n");
}

// Running median must equal "sort the window and take the middle" every time,
// and the two halves must never differ by more than one.
static float bruteMedian(const float* h, int n, int W) {
    int k = n < W ? n : W;
    float t[MAX_WINDOW];
    for (int i = 0; i < k; i++) t[i] = h[n - k + i];
    for (int i = 1; i < k; i++) {
        float v = t[i]; int j = i - 1;
        while (j >= 0 && t[j] > v) { t[j + 1] = t[j]; j--; } t[j + 1] = v;
    }
    return k % 2 ? t[k / 2] : (t[k / 2 - 1] + t[k / 2]) / 2.0f;
}

static void testRunningMedian() {
    static float hist[3000];
    for (int W = 1; W <= MAX_WINDOW; W++) {
        RunningMedian rm(W);
        std::srand(100 + W);
        for (int i = 0; i < 3000; i++) {
            float v = 20.0f + (std::rand() % 100) / 10.0f;
            if (std::rand() % 12 == 0) v = 85.0f;
            hist[i] = v;
            rm.add(v);
            int d = rm.smallSize() - rm.bigSize();
            CHECK(d >= -1 && d <= 1);
            CHECK(rm.heapsValid());
            CHECK(near(rm.median(), bruteMedian(hist, i + 1, W)));
        }
    }
    CHECK(RunningMedian(100).windowSize() == MAX_WINDOW);   // too big gets capped
    std::printf("  Running median = sorted median .. OK (windows 1..%d)\n", MAX_WINDOW);
}

// The worked example from class: only the 90 gets replaced.
static void testHampelExample() {
    HampelFilter h(3);
    const float raw[] = { 20, 22, 21, 90, 22, 23, 21, 22 };
    const float out[] = { 20, 22, 21, 22, 22, 23, 21, 22 };
    for (int i = 0; i < 8; i++) {
        CHECK(h.filter(raw[i]) == out[i]);
        CHECK(h.lastWasSpike() == (i == 3));
    }
    std::printf("  Hampel example (only 90 replaced)  OK\n");
}

// Lots of high and low spikes: every one is caught and nothing normal is flagged.
static void testManySpikes() {
    HampelFilter h(7, 3.0f, 1.0f);
    long injected = 0;
    for (int i = 0; i < 2000; i++) {
        float raw = (float)(20 + (i / 10) % 3);
        bool isSpike = false;
        if (i % 10 == 7) { raw = 1000; isSpike = true; }
        if (i % 13 == 3) { raw = -500; isSpike = true; }
        if (isSpike) injected++;
        float out = h.filter(raw);
        CHECK(h.lastWasSpike() == isSpike);
        CHECK(out >= 20 && out <= 22);
    }
    CHECK(h.spikesFound() == injected);
    std::printf("  Many spikes, all caught ......... OK (%ld)\n", injected);
}

// A real jump in temperature must get through (after about half a window).
static void testRealChange() {
    HampelFilter h(5);
    for (int i = 0; i < 20; i++) {
        float out = h.filter(i < 10 ? 20.0f : 30.0f);
        CHECK(out == (i < 12 ? 20.0f : 30.0f));
    }
    std::printf("  Real change passes through ...... OK\n");
}

static void testNormalizer() {
    Normalizer t = { -40.0f, 125.0f };
    CHECK(t.valid());
    CHECK(near(t.normalize(-40), 0) && near(t.normalize(125), 1) && near(t.normalize(42.5f), 0.5f));
    CHECK(near(t.denormalize(0.5f), 42.5f));
    CHECK(t.outOfRange(200) && near(t.normalize(200), 1));
    CHECK(!t.outOfRange(25));
    std::printf("  Normalizer ...................... OK\n");
}

// The module main.cpp uses: registering sensors and processing one reading at a time.
static void testNoiseFilterModule() {
    NoiseFilter nf(5, 3.0f);
    CHECK(nf.addSensor(1, -40.0f, 125.0f, 0.1f));    // temperature
    CHECK(nf.addSensor(2, 0.0f, 5.0f, 0.01f));       // voltage
    CHECK(!nf.addSensor(1, 0.0f, 1.0f, 0.1f));       // same id twice
    CHECK(!nf.addSensor(9, 5.0f, 5.0f, 0.1f));       // bad range
    CHECK(nf.addSensor(3, -2.0f, 2.0f, 0.01f));
    CHECK(nf.addSensor(4, 0.0f, 100.0f, 1.0f));
    CHECK(!nf.addSensor(5, 0.0f, 1.0f, 0.1f));       // table full
    CHECK(nf.sensorCount() == MAX_SENSORS);

    FilterResult r;
    CHECK(!nf.process(42, 0, 1.0f, r));              // unknown sensor

    const float temp[] = { 22.0f, 22.2f, 22.1f, 22.3f, 85.0f, 22.4f };
    for (int i = 0; i < 6; i++) {
        CHECK(nf.process(1, 1000UL * i, temp[i], r));
        CHECK(nf.process(2, 1000UL * i, 3.30f, r));  // voltage stays steady
        CHECK(!r.isSpike && near(r.clean, 3.30f));   // temperature spike doesn't touch it
    }
    CHECK(nf.spikesFound(1) == 1 && nf.spikesFound(2) == 0 && nf.spikesFound(42) == -1);

    NoiseFilter one(5, 3.0f);
    one.addSensor(1, -40.0f, 125.0f, 0.1f);
    for (int i = 0; i < 5; i++) one.process(1, 500UL * i, temp[i], r);
    CHECK(r.isSpike && near(r.clean, 22.2f) && near(r.normalized, 62.2f / 165.0f));
    CHECK(r.timestamp == 2000UL && r.sensorId == 1 && near(r.raw, 85.0f));
    CHECK(!r.outOfRange);
    one.process(1, 2500UL, 500.0f, r);               // impossible for this sensor
    CHECK(r.outOfRange && r.isSpike);
    std::printf("  NoiseFilter module .............. OK\n");
}

int main() {
    std::printf("Running Member 3 tests...\n");
    testHeapOrder();
    testLazyDeletion();
    testFullHeapCleansUp();
    testRunningMedian();
    testHampelExample();
    testManySpikes();
    testRealChange();
    testNormalizer();
    testNoiseFilterModule();
    std::printf("Memory here (64-bit PC): HampelFilter %u bytes, NoiseFilter %u bytes\n",
        (unsigned)sizeof(HampelFilter), (unsigned)sizeof(NoiseFilter));
    std::printf("ALL TESTS PASSED\n");
    return 0;
}