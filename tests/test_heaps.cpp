
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
