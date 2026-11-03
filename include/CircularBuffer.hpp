#ifndef CIRCULAR_BUFFER_HPP
#define CIRCULAR_BUFFER_HPP

// I put these small helpers in this file so the buffer is self-contained.

// usize is the unsigned type that sizeof returns, and I use it for sizes and indices.
using usize = decltype(sizeof(0));

// Select picks type A when the condition is true and type B otherwise, decided at compile time.
template <bool Condition, class A, class B>
struct Select { using type = A; };

template <class A, class B>
struct Select<false, A, B> { using type = B; };

// IndexFor picks the smallest unsigned type that can hold 0..N, so my head and count use as little RAM as possible.
template <usize N>
struct IndexFor {
    using type = typename Select<
        (N <= 255), unsigned char,
        typename Select<(N <= 65535), unsigned short, unsigned int>::type
    >::type;
};

// These are the two policies the project asks for when push() is called on a full buffer.
enum class OverflowPolicy {
    OVERWRITE_OLDEST,  // I throw away the oldest sample and keep the newest one
    DROP_NEWEST        // I reject the incoming sample and keep what is already stored
};

// My ring buffer stores its samples in a fixed array inside the object, so no dynamic memory is needed.
template <class T, usize N,
          OverflowPolicy Policy = OverflowPolicy::DROP_NEWEST>
class CircularBuffer {
    static_assert(N > 0, "capacity must be at least 1");  // an array of size zero is not allowed

    using Index = typename IndexFor<N>::type;  // smallest integer type that fits 0..N

public:
    CircularBuffer() : head_(0), count_(0) {}  // starting with head 0 and count 0 means the buffer is empty

    // I add a sample at the tail, and I return false only when it is rejected (DROP_NEWEST and full).
    bool push(const T& value) {
        if (count_ == N) {  // the buffer is full, so the policy decides what happens
            if (Policy == OverflowPolicy::DROP_NEWEST) {
                return false;  // I refuse the new sample and leave the stored data untouched
            }
            // When the buffer is full the tail slot is the same slot as head_, so I overwrite the oldest sample directly.
            data_[head_] = value;
            head_ = advance(head_);  // the second-oldest sample is now the oldest
            return true;
        }
        data_[wrapIndex(static_cast<usize>(head_) + count_)] = value;  // there is room, so I write at the tail slot
        ++count_;
        return true;
    }

    // I remove the oldest sample into out (first in, first out), and I return false if the buffer is empty.
    bool pop(T& out) {
        if (count_ == 0) return false;
        out = data_[head_];
        head_ = advance(head_);  // I remove it just by moving head forward, so nothing is shifted
        --count_;
        return true;
    }

    // I copy the oldest sample into out without removing it, and I return false if the buffer is empty.
    bool peek(T& out) const {
        if (count_ == 0) return false;
        out = data_[head_];
        return true;
    }

    // at(i) gives the sample at logical position i, where 0 is the oldest, and the caller must keep i below size().
    const T& at(usize i) const {
        return data_[wrapIndex(static_cast<usize>(head_) + i)];
    }

    bool isEmpty() const { return count_ == 0; }  // true when no samples are stored
    bool isFull()  const { return count_ == N; }  // true when every slot is in use
    usize size()   const { return count_; }       // how many samples are stored right now

    static constexpr usize getCapacity() { return N; }  // the capacity is a compile-time constant, so it uses no RAM

    void clear() { head_ = 0; count_ = 0; }  // I reset only the bookkeeping, so clearing is O(1)

private:
    // I wrap an index from [0, 2N-1] back into [0, N-1] with one subtraction, which is cheaper than using %.
    static usize wrapIndex(usize i) { return (i >= N) ? i - N : i; }

    // I move an index forward by one and wrap N-1 back to 0.
    static Index advance(Index i) {
        return static_cast<Index>((static_cast<usize>(i) + 1 == N) ? 0 : i + 1);
    }

    T     data_[N];  // the fixed array that holds all the samples
    Index head_;     // index of the oldest sample, which is where pop reads
    Index count_;    // number of stored samples, and the tail is head_ + count_ wrapped, so I do not store it
};

#endif  // CIRCULAR_BUFFER_HPP
