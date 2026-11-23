#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "../include/SlidingWindow.hpp"

// Runs one test and prints its name before and after.
// If an assert fails, the program aborts and the last [RUN] line shows which test failed.
void run(const char* name, void (*test)()) {
    std::cout << "[RUN ] " << name << "\n";
    test();
    std::cout << "[PASS] " << name << "\n\n";
}

// ---------------- Brute-force references ----------------
// (these also become the naive baseline for the Final Phase)

double brute_max(const std::vector<double>& v, int i, int W) {
    int start = (i - W + 1 > 0) ? i - W + 1 : 0;
    double m = v[start];
    for (int j = start; j <= i; j++) if (v[j] > m) m = v[j];
    return m;
}
double brute_min(const std::vector<double>& v, int i, int W) {
    int start = (i - W + 1 > 0) ? i - W + 1 : 0;
    double m = v[start];
    for (int j = start; j <= i; j++) if (v[j] < m) m = v[j];
    return m;
}
double brute_avg(const std::vector<double>& v, int i, int W) {
    int start = (i - W + 1 > 0) ? i - W + 1 : 0;
    double s = 0;
    for (int j = start; j <= i; j++) s += v[j];
    return s / (i - start + 1);
}

// Floating-point comparison (the running sum is not bit-identical to a fresh sum)
bool close(double a, double b, double eps = 1e-9) {
    return std::fabs(a - b) <= eps * (1.0 + std::fabs(b));
}

// ---------------- SlidingWindow tests ----------------

void test_single_sample() {
    SlidingWindow<3> sw;
    sw.add(42.0);
    std::cout << "  added 42: max=" << sw.getMax() << " min=" << sw.getMin()
              << " avg=" << sw.getAverage() << "\n";
    assert(sw.getMax() == 42.0);
    assert(sw.getMin() == 42.0);
    assert(close(sw.getAverage(), 42.0));
}

void test_empty_average_is_zero() {
    SlidingWindow<3> sw;
    std::cout << "  empty window: avg=" << sw.getAverage()
              << " ready=" << sw.ready() << "\n";
    assert(sw.getAverage() == 0.0);
    assert(!sw.ready());
}

void test_ready_flips_at_w() {
    SlidingWindow<4> sw;
    for (int i = 1; i <= 8; i++) {
        sw.add(i);
        bool expected = (i >= 4);
        std::cout << "  after " << i << " samples: ready=" << sw.ready()
                  << " (expected " << expected << ")\n";
        assert(sw.ready() == expected);
    }
}

void test_window_not_yet_full() {
    SlidingWindow<5> sw;
    sw.add(3);
    sw.add(1);
    std::cout << "  added 3,1 (W=5): max=" << sw.getMax() << " min=" << sw.getMin()
              << " avg=" << sw.getAverage() << "\n";
    assert(sw.getMax() == 3);
    assert(sw.getMin() == 1);
    assert(close(sw.getAverage(), 2.0));   // average over the 2 samples seen, not over W
}

void test_known_sequence() {
    // W = 3, samples 1 3 -1 -3 5 3 6 7
    SlidingWindow<3> sw;
    double in[]     = {1, 3, -1, -3, 5, 3, 6, 7};
    double expMax[] = {1, 3,  3,  3, 5, 5, 6, 7};
    double expMin[] = {1, 1, -1, -3, -3, -3, 3, 3};
    double expAvg[] = {1.0, 2.0, 1.0, -1.0 / 3, 1.0 / 3, 5.0 / 3, 14.0 / 3, 16.0 / 3};
    for (int i = 0; i < 8; i++) {
        sw.add(in[i]);
        std::cout << "  add " << in[i]
                  << " -> max=" << sw.getMax() << " (expected " << expMax[i] << ")"
                  << ", min=" << sw.getMin() << " (expected " << expMin[i] << ")"
                  << ", avg=" << sw.getAverage() << " (expected " << expAvg[i] << ")\n";
        assert(sw.getMax() == expMax[i]);
        assert(sw.getMin() == expMin[i]);
        assert(close(sw.getAverage(), expAvg[i]));
    }
}

void test_old_max_expires() {
    SlidingWindow<3> sw;
    sw.add(100);
    sw.add(1);
    sw.add(2);
    std::cout << "  window {100,1,2}: max=" << sw.getMax() << "\n";
    assert(sw.getMax() == 100);
    sw.add(3);
    std::cout << "  window {1,2,3}: max=" << sw.getMax() << " (100 expired)\n";
    assert(sw.getMax() == 3);
}

void test_old_min_expires() {
    SlidingWindow<3> sw;
    sw.add(-100);
    sw.add(1);
    sw.add(2);
    std::cout << "  window {-100,1,2}: min=" << sw.getMin() << "\n";
    assert(sw.getMin() == -100);
    sw.add(3);
    std::cout << "  window {1,2,3}: min=" << sw.getMin() << " (-100 expired)\n";
    assert(sw.getMin() == 1);
}

void test_average_drops_expired_value() {
    SlidingWindow<3> sw;
    sw.add(30);
    sw.add(0);
    sw.add(0);
    std::cout << "  window {30,0,0}: avg=" << sw.getAverage() << "\n";
    assert(close(sw.getAverage(), 10.0));
    sw.add(0);
    std::cout << "  window {0,0,0}: avg=" << sw.getAverage() << " (30 expired)\n";
    assert(close(sw.getAverage(), 0.0));
}

void test_constant_values() {
    SlidingWindow<4> sw;
    for (int i = 0; i < 20; i++) {
        sw.add(7.5);
        assert(sw.getMax() == 7.5);
        assert(sw.getMin() == 7.5);
        assert(close(sw.getAverage(), 7.5));
    }
    std::cout << "  20 adds of 7.5: max, min and avg stayed 7.5\n";
}

void test_strictly_increasing() {
    const int W = 3;
    SlidingWindow<W> sw;
    for (int i = 0; i < 20; i++) {
        sw.add(i);
        int start = (i - W + 1 > 0) ? i - W + 1 : 0;
        assert(sw.getMax() == i);
        assert(sw.getMin() == start);
        assert(close(sw.getAverage(), (start + i) / 2.0));   // mean of consecutive ints
    }
    std::cout << "  0..19 increasing: max=newest, min=window start, avg=midpoint\n";
}

void test_strictly_decreasing() {
    const int W = 3;
    SlidingWindow<W> sw;
    for (int i = 0; i < 20; i++) {
        sw.add(-i);
        int start = (i - W + 1 > 0) ? i - W + 1 : 0;
        assert(sw.getMin() == -i);
        assert(sw.getMax() == -start);
        assert(close(sw.getAverage(), -(start + i) / 2.0));
    }
    std::cout << "  0..-19 decreasing: min=newest, max=window start, avg=midpoint\n";
}

void test_window_size_one() {
    SlidingWindow<1> sw;
    double in[] = {4, 9, 2, 7};
    for (double x : in) {
        sw.add(x);
        std::cout << "  W=1 add " << x << " -> max=" << sw.getMax()
                  << " min=" << sw.getMin() << " avg=" << sw.getAverage() << "\n";
        assert(sw.getMax() == x);
        assert(sw.getMin() == x);
        assert(close(sw.getAverage(), x));
        assert(sw.ready());
    }
}

template <int W>
void check_against_brute_force(int n, unsigned seed) {
    std::srand(seed);
    SlidingWindow<W> sw;
    std::vector<double> v;
    for (int i = 0; i < n; i++) {
        double x = (std::rand() % 2001) - 1000;   // -1000 .. 1000
        v.push_back(x);
        sw.add(x);
        assert(sw.getMax() == brute_max(v, i, W));
        assert(sw.getMin() == brute_min(v, i, W));
        assert(close(sw.getAverage(), brute_avg(v, i, W)));
    }
    std::cout << "  W=" << W << ", " << n << " random samples (seed " << seed
              << "): matches brute force\n";
}

void test_random_vs_brute_force() {
    check_against_brute_force<1>(500, 1);
    check_against_brute_force<2>(500, 2);
    check_against_brute_force<5>(2000, 3);
    check_against_brute_force<16>(2000, 4);
    check_against_brute_force<100>(5000, 5);
}

void test_long_stream_sum_drift() {
    // Long stream of non-integer values: checks the running sum doesn't drift noticeably.
    const int W = 50;
    const int N = 1000000;
    std::srand(99);
    SlidingWindow<W> sw;
    std::vector<double> v;
    v.reserve(N);
    double worst = 0;
    for (int i = 0; i < N; i++) {
        double x = (std::rand() % 100000) / 7.0;
        v.push_back(x);
        sw.add(x);
        // Only compare occasionally so the brute force doesn't dominate the runtime
        if (i % 10000 == 0 || i == N - 1) {
            double err = std::fabs(sw.getAverage() - brute_avg(v, i, W));
            if (err > worst) worst = err;
            assert(close(sw.getAverage(), brute_avg(v, i, W), 1e-6));
        }
    }
    std::cout << "  " << N << " samples, W=" << W
              << ": worst average error = " << worst << "\n";
}

int main() {
    std::cout << "=== SlidingWindow tests ===\n\n";
    run("single sample",               test_single_sample);
    run("empty average is zero",       test_empty_average_is_zero);
    run("ready() flips at W",          test_ready_flips_at_w);
    run("window not yet full",         test_window_not_yet_full);
    run("known sequence 1 3 -1 ...",   test_known_sequence);
    run("old max expires",             test_old_max_expires);
    run("old min expires",             test_old_min_expires);
    run("average drops expired value", test_average_drops_expired_value);
    run("constant values",             test_constant_values);
    run("strictly increasing",         test_strictly_increasing);
    run("strictly decreasing",         test_strictly_decreasing);
    run("window size one",             test_window_size_one);
    run("random vs brute force",       test_random_vs_brute_force);
    run("long stream sum drift",       test_long_stream_sum_drift);

    std::cout << "All tests passed.\n";
    return 0;
}
