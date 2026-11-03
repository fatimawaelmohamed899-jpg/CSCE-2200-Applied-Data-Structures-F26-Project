#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "../include/MonotonicDeque.hpp"

// Runs one test and prints its name before and after.
// If an assert fails, the program aborts and the last [RUN] line shows which test failed.
void run(const char* name, void (*test)()) {
    std::cout << "[RUN ] " << name << "\n";
    test();
    std::cout << "[PASS] " << name << "\n\n";
}

// ---------------- Deque tests ----------------

void test_queue_starts_empty() {
    Deque<int, 4> q;
    std::cout << "  new queue: empty=" << q.empty() << " full=" << q.full()
              << " size=" << q.getSize() << "\n";
    assert(q.empty() && "new queue should be empty");
    assert(!q.full() && "new queue should not be full");
    assert(q.getSize() == 0);
}

void test_queue_fifo_order() {
    Deque<int, 4> q;
    q.push_back(1);
    q.push_back(2);
    q.push_back(3);
    std::cout << "  after push_back 1,2,3: front=" << q.getFront()
              << " back=" << q.getBack() << "\n";
    assert(q.getFront() == 1);
    assert(q.getBack() == 3);

    q.pop_front();
    std::cout << "  after pop_front: front=" << q.getFront() << "\n";
    assert(q.getFront() == 2);

    q.pop_front();
    std::cout << "  after pop_front: front=" << q.getFront()
              << " back=" << q.getBack() << "\n";
    assert(q.getFront() == 3);
    assert(q.getBack() == 3);
}

void test_queue_push_front() {
    Deque<int, 4> q;
    q.push_front(1);
    q.push_front(2);
    q.push_front(3);
    std::cout << "  after push_front 1,2,3: front=" << q.getFront()
              << " back=" << q.getBack() << " size=" << q.getSize() << "\n";
    assert(q.getFront() == 3);
    assert(q.getBack() == 1);
    assert(q.getSize() == 3);
}

void test_queue_pop_back() {
    Deque<int, 4> q;
    q.push_back(1);
    q.push_back(2);
    q.push_back(3);
    q.pop_back();
    std::cout << "  after pop_back: front=" << q.getFront()
              << " back=" << q.getBack() << " size=" << q.getSize() << "\n";
    assert(q.getBack() == 2);
    assert(q.getFront() == 1);
    assert(q.getSize() == 2);
}

void test_queue_full_rejects_push() {
    Deque<int, 3> q;
    q.push_back(1);
    q.push_back(2);
    q.push_back(3);
    std::cout << "  filled to capacity: full=" << q.full() << "\n";
    assert(q.full());

    q.push_back(99);    // should be rejected
    q.push_front(98);   // should be rejected
    std::cout << "  after rejected pushes: size=" << q.getSize()
              << " front=" << q.getFront() << " back=" << q.getBack() << "\n";
    assert(q.getSize() == 3);
    assert(q.getFront() == 1);
    assert(q.getBack() == 3);
}

void test_queue_pop_on_empty_is_safe() {
    Deque<int, 3> q;
    q.pop_front();
    q.pop_back();
    std::cout << "  popped an empty queue: size=" << q.getSize() << "\n";
    assert(q.empty());
    assert(q.getSize() == 0);

    q.push_back(7);
    std::cout << "  pushed 7 afterwards: front=" << q.getFront()
              << " back=" << q.getBack() << "\n";
    assert(q.getFront() == 7);
    assert(q.getBack() == 7);
}

void test_queue_wraparound() {
    Deque<int, 4> q;
    for (int i = 1; i <= 4; i++) q.push_back(i);   // 1 2 3 4
    q.pop_front();
    q.pop_front();                                 // 3 4
    q.push_back(5);
    q.push_back(6);                                // 3 4 5 6 (wrapped)
    std::cout << "  after wrap: full=" << q.full() << " front=" << q.getFront()
              << " back=" << q.getBack() << "\n";
    assert(q.full());
    assert(q.getFront() == 3);
    assert(q.getBack() == 6);

    q.push_front(0);   // rejected, full
    std::cout << "  push_front on full queue rejected: front=" << q.getFront() << "\n";
    assert(q.getFront() == 3);
}

void test_queue_mixed_ends_wrap() {
    Deque<int, 3> q;
    q.push_back(1);
    q.push_front(2);   // 2 1  (front wraps below index 0)
    q.push_back(3);    // 2 1 3
    std::cout << "  contents 2 1 3: front=" << q.getFront()
              << " back=" << q.getBack() << " full=" << q.full() << "\n";
    assert(q.full());
    assert(q.getFront() == 2);
    assert(q.getBack() == 3);

    q.pop_back();
    std::cout << "  after pop_back: back=" << q.getBack() << "\n";
    assert(q.getBack() == 1);

    q.pop_front();
    std::cout << "  after pop_front: front=" << q.getFront() << "\n";
    assert(q.getFront() == 1);
}

void test_queue_empty_then_refill() {
    Deque<int, 3> q;
    for (int round = 0; round < 3; round++) {
        q.push_back(1);
        q.push_back(2);
        q.push_back(3);
        assert(q.full());
        q.pop_front();
        q.pop_front();
        q.pop_front();
        assert(q.empty());
        std::cout << "  round " << round << ": filled and emptied OK\n";
    }
}

// ---------------- Monotonic deque tests ----------------

// Brute-force reference: max/min of the last W samples ending at index i
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

void test_single_sample() {
    MonotonicDeque<3> d;
    d.push(42.0);
    std::cout << "  pushed 42: max=" << d.getMax() << " min=" << d.getMin() << "\n";
    assert(d.getMax() == 42.0);
    assert(d.getMin() == 42.0);
}

void test_window_not_yet_full() {
    MonotonicDeque<5> d;
    d.push(3);
    d.push(1);
    std::cout << "  pushed 3,1 (W=5): max=" << d.getMax()
              << " min=" << d.getMin() << "\n";
    assert(d.getMax() == 3);
    assert(d.getMin() == 1);
}

void test_known_sequence() {
    // W = 3, samples 5 1 4 2
    MonotonicDeque<3> d;
    double in[]     = {5, 1, 4, 2};
    double expMax[] = {5, 5, 5, 4};
    double expMin[] = {5, 1, 1, 1};
    for (int i = 0; i < 4; i++) {
        d.push(in[i]);
        std::cout << "  push " << in[i]
                  << " -> max=" << d.getMax() << " (expected " << expMax[i] << ")"
                  << ", min=" << d.getMin() << " (expected " << expMin[i] << ")\n";
        assert(d.getMax() == expMax[i]);
        assert(d.getMin() == expMin[i]);
    }
}

void test_old_max_expires() {
    MonotonicDeque<3> d;
    d.push(100);
    d.push(1);
    d.push(2);
    std::cout << "  window {100,1,2}: max=" << d.getMax() << "\n";
    assert(d.getMax() == 100);
    d.push(3);
    std::cout << "  window {1,2,3}: max=" << d.getMax() << " (100 expired)\n";
    assert(d.getMax() == 3);
}

void test_old_min_expires() {
    MonotonicDeque<3> d;
    d.push(-100);
    d.push(1);
    d.push(2);
    std::cout << "  window {-100,1,2}: min=" << d.getMin() << "\n";
    assert(d.getMin() == -100);
    d.push(3);
    std::cout << "  window {1,2,3}: min=" << d.getMin() << " (-100 expired)\n";
    assert(d.getMin() == 1);
}

void test_constant_values() {
    MonotonicDeque<4> d;
    for (int i = 0; i < 20; i++) {
        d.push(7.5);
        assert(d.getMax() == 7.5);
        assert(d.getMin() == 7.5);
    }
    std::cout << "  20 pushes of 7.5: max and min stayed 7.5\n";
}

void test_strictly_increasing() {
    const int W = 3;
    MonotonicDeque<W> d;
    for (int i = 0; i < 20; i++) {
        d.push(i);
        int start = (i - W + 1 > 0) ? i - W + 1 : 0;
        assert(d.getMax() == i);
        assert(d.getMin() == start);
    }
    std::cout << "  0..19 increasing: max tracked newest, min tracked window start\n";
}

void test_strictly_decreasing() {
    const int W = 3;
    MonotonicDeque<W> d;
    for (int i = 0; i < 20; i++) {
        d.push(-i);
        int start = (i - W + 1 > 0) ? i - W + 1 : 0;
        assert(d.getMin() == -i);
        assert(d.getMax() == -start);
    }
    std::cout << "  0..-19 decreasing: min tracked newest, max tracked window start\n";
}

void test_window_size_one() {
    MonotonicDeque<1> d;
    double in[] = {4, 9, 2, 7};
    for (double x : in) {
        d.push(x);
        std::cout << "  W=1 push " << x << " -> max=" << d.getMax()
                  << " min=" << d.getMin() << "\n";
        assert(d.getMax() == x);
        assert(d.getMin() == x);
    }
}

template <int W>
void check_against_brute_force(int n, unsigned seed) {
    std::srand(seed);
    MonotonicDeque<W> d;
    std::vector<double> v;
    for (int i = 0; i < n; i++) {
        double x = (std::rand() % 2001) - 1000;   // -1000 .. 1000
        v.push_back(x);
        d.push(x);
        assert(d.getMax() == brute_max(v, i, W));
        assert(d.getMin() == brute_min(v, i, W));
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

int main() {
    std::cout << "=== Deque tests ===\n\n";
    run("queue starts empty",          test_queue_starts_empty);
    run("queue FIFO order",            test_queue_fifo_order);
    run("queue push_front",            test_queue_push_front);
    run("queue pop_back",              test_queue_pop_back);
    run("queue full rejects push",     test_queue_full_rejects_push);
    run("queue pop on empty is safe",  test_queue_pop_on_empty_is_safe);
    run("queue wraparound",            test_queue_wraparound);
    run("queue mixed ends wrap",       test_queue_mixed_ends_wrap);
    run("queue empty then refill",     test_queue_empty_then_refill);

    std::cout << "=== MonotonicDeque tests ===\n\n";
    run("single sample",               test_single_sample);
    run("window not yet full",         test_window_not_yet_full);
    run("known sequence 5 1 4 2",      test_known_sequence);
    run("old max expires",             test_old_max_expires);
    run("old min expires",             test_old_min_expires);
    run("constant values",             test_constant_values);
    run("strictly increasing",         test_strictly_increasing);
    run("strictly decreasing",         test_strictly_decreasing);
    run("window size one",             test_window_size_one);
    run("random vs brute force",       test_random_vs_brute_force);

    std::cout << "All tests passed.\n";
    return 0;
}
