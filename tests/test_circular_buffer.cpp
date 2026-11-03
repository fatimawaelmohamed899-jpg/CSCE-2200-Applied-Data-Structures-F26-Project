// Build with: g++ -std=c++11 -Wall -Wextra -I include tests/test_circular_buffer.cpp -o t
// The exit code is the result: 0 means every test passed, and k means test number k failed first.
#include "CircularBuffer.hpp"
#include "Queue.hpp"

// If the condition is false, the current test returns false.
#define CHECK(cond) do { if (!(cond)) return false; } while (0)

// Test 1: I check that a new buffer is empty, not full, and reports its capacity.
static bool t_initial_state() {
    CircularBuffer<int, 4> b;
    CHECK(b.isEmpty());
    CHECK(!b.isFull());
    CHECK(b.size() == 0);
    CHECK(b.getCapacity() == 4);
    return true;
}

// Test 2: I check that items come out in the same order they went in.
static bool t_fifo_order() {
    CircularBuffer<int, 4> b;
    int v = 0;
    CHECK(b.push(10)); CHECK(b.push(20)); CHECK(b.push(30));
    CHECK(b.size() == 3);
    CHECK(b.pop(v) && v == 10);
    CHECK(b.pop(v) && v == 20);
    CHECK(b.pop(v) && v == 30);
    CHECK(b.isEmpty());
    return true;
}

// Test 3: I check that pop and peek on an empty buffer fail and leave the output unchanged.
static bool t_pop_empty() {
    CircularBuffer<int, 4> b;
    int v = 99;
    CHECK(!b.pop(v));
    CHECK(!b.peek(v));
    CHECK(v == 99);
    return true;
}

// Test 4: I check that head and tail wrap past the end of the array and the order stays correct.
static bool t_index_wrapping() {
    CircularBuffer<int, 4> b;
    int v = 0;
    b.push(1); b.push(2); b.push(3);
    CHECK(b.pop(v) && v == 1);
    CHECK(b.pop(v) && v == 2);  // head is now at index 2
    CHECK(b.push(4));           // goes to index 3
    CHECK(b.push(5));           // the tail wraps around to index 0
    CHECK(b.push(6));           // goes to index 1
    CHECK(b.isFull());
    CHECK(b.pop(v) && v == 3);  // head wraps from 3 to 0 after this
    CHECK(b.pop(v) && v == 4);
    CHECK(b.pop(v) && v == 5);
    CHECK(b.pop(v) && v == 6);
    CHECK(b.isEmpty());
    return true;
}

// Test 5: I check that DROP_NEWEST rejects new samples when full and keeps the old ones.
static bool t_drop_newest() {
    CircularBuffer<int, 3, OverflowPolicy::DROP_NEWEST> b;
    int v = 0;
    CHECK(b.push(1)); CHECK(b.push(2)); CHECK(b.push(3));
    CHECK(b.isFull());
    CHECK(!b.push(4));  // rejected
    CHECK(!b.push(5));  // rejected
    CHECK(b.size() == 3);
    CHECK(b.pop(v) && v == 1);
    CHECK(b.pop(v) && v == 2);
    CHECK(b.pop(v) && v == 3);
    return true;
}

// Test 6: I check that OVERWRITE_OLDEST removes the oldest sample when the buffer is full.
static bool t_overwrite_oldest() {
    CircularBuffer<int, 3, OverflowPolicy::OVERWRITE_OLDEST> b;
    int v = 0;
    b.push(1); b.push(2); b.push(3);
    CHECK(b.push(4));  // accepted, and 1 is evicted
    CHECK(b.size() == 3);
    CHECK(b.pop(v) && v == 2);
    CHECK(b.pop(v) && v == 3);
    CHECK(b.pop(v) && v == 4);
    return true;
}

// Test 7: I check that many overwrites in a row keep only the newest samples.
static bool t_overwrite_many() {
    CircularBuffer<int, 4, OverflowPolicy::OVERWRITE_OLDEST> b;
    for (int i = 1; i <= 10; ++i) b.push(i);  // this wraps the array more than twice
    CHECK(b.size() == 4);
    int v = 0;
    CHECK(b.pop(v) && v == 7);
    CHECK(b.pop(v) && v == 8);
    CHECK(b.pop(v) && v == 9);
    CHECK(b.pop(v) && v == 10);
    return true;
}

// Test 8: I check that at(i) returns oldest-first order even after the data has wrapped.
static bool t_at_logical_order() {
    CircularBuffer<int, 4, OverflowPolicy::OVERWRITE_OLDEST> b;
    for (int i = 1; i <= 6; ++i) b.push(i);  // it holds 3,4,5,6 stored in wrapped positions
    CHECK(b.at(0) == 3);
    CHECK(b.at(1) == 4);
    CHECK(b.at(2) == 5);
    CHECK(b.at(3) == 6);
    return true;
}

// Test 9: I check that peek reads the oldest sample without removing it.
static bool t_peek() {
    CircularBuffer<int, 4> b;
    int v = 0;
    b.push(7); b.push(8);
    CHECK(b.peek(v) && v == 7);
    CHECK(b.size() == 2);  // nothing was removed
    return true;
}

// Test 10: I check that clear empties the buffer and it can be used again.
static bool t_clear() {
    CircularBuffer<int, 4> b;
    b.push(1); b.push(2); b.push(3);
    b.clear();
    CHECK(b.isEmpty());
    CHECK(b.push(42));
    int v = 0;
    CHECK(b.pop(v) && v == 42);
    return true;
}

// Test 11: I check the smallest case, capacity 1, with both policies.
static bool t_capacity_one() {
    CircularBuffer<int, 1, OverflowPolicy::DROP_NEWEST> d;
    int v = 0;
    CHECK(d.push(1));
    CHECK(d.isFull());
    CHECK(!d.push(2));
    CHECK(d.pop(v) && v == 1);

    CircularBuffer<int, 1, OverflowPolicy::OVERWRITE_OLDEST> o;
    CHECK(o.push(1));
    CHECK(o.push(2));
    CHECK(o.pop(v) && v == 2);
    CHECK(o.isEmpty());
    return true;
}

// Test 12: I check the memory footprint, which should be the data plus two small indices.
static bool t_memory_footprint() {
    CHECK(sizeof(CircularBuffer<unsigned char, 8>) == 8 + 2);      // two 1-byte indices
    CHECK(sizeof(CircularBuffer<unsigned char, 300>) == 300 + 4);  // two 2-byte indices, since 300 is above 255
    return true;
}

// Test 13: I check that a buffer with more than 255 slots, which uses 2-byte indices, still wraps correctly.
static bool t_large_capacity() {
    CircularBuffer<int, 300, OverflowPolicy::OVERWRITE_OLDEST> b;
    for (int i = 0; i < 700; ++i) b.push(i);  // this wraps the array twice
    CHECK(b.size() == 300);
    CHECK(b.at(0) == 400);
    CHECK(b.at(299) == 699);
    return true;
}

// Test 14: I check that the queue keeps FIFO order, rejects when full, and reuses freed slots.
static bool t_queue() {
    Queue<int, 3> q;
    int v = 0;
    CHECK(q.isEmpty());
    CHECK(q.enqueue(1)); CHECK(q.enqueue(2)); CHECK(q.enqueue(3));
    CHECK(q.isFull());
    CHECK(!q.enqueue(4));  // the queue is full, so the packet is refused
    CHECK(q.front(v) && v == 1);
    CHECK(q.dequeue(v) && v == 1);
    CHECK(q.enqueue(5));   // this reuses the freed slot, so the tail wraps
    CHECK(q.dequeue(v) && v == 2);
    CHECK(q.dequeue(v) && v == 3);
    CHECK(q.dequeue(v) && v == 5);
    CHECK(!q.dequeue(v));
    CHECK(q.isEmpty());
    return true;
}

// I run every test in order and return the number of the first one that fails, or 0 if all pass.
int main() {
    bool (*const tests[])() = {
        t_initial_state, t_fifo_order, t_pop_empty, t_index_wrapping,
        t_drop_newest, t_overwrite_oldest, t_overwrite_many,
        t_at_logical_order, t_peek, t_clear, t_capacity_one,
        t_memory_footprint, t_large_capacity, t_queue
    };
    const int count = static_cast<int>(sizeof(tests) / sizeof(tests[0]));
    for (int i = 0; i < count; ++i) {
        if (!tests[i]()) return i + 1;
    }
    return 0;
}
