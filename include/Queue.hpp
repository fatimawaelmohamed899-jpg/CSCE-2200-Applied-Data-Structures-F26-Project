#ifndef QUEUE_HPP
#define QUEUE_HPP

#include "CircularBuffer.hpp"

// My queue holds outgoing packets in first-in-first-out order, and I built it on top of my ring buffer.
template <class T, usize N>
class Queue {
public:
    bool enqueue(const T& item) { return buf_.push(item); }  // I add a packet at the back, and I return false if the queue is full
    bool dequeue(T& out) { return buf_.pop(out); }           // I remove the packet at the front, and I return false if the queue is empty
    bool front(T& out) const { return buf_.peek(out); }      // I read the front packet without removing it

    bool  isEmpty() const { return buf_.isEmpty(); }     // true when no packets are waiting
    bool  isFull()  const { return buf_.isFull(); }      // true when no more packets fit
    usize size()    const { return buf_.size(); }        // how many packets are waiting
    static constexpr usize getCapacity() { return N; }   // the most packets the queue can hold
    void  clear() { buf_.clear(); }                      // I drop all the waiting packets

private:
    // I use DROP_NEWEST so a packet that is already waiting is never lost, and the new one is refused instead.
    CircularBuffer<T, N, OverflowPolicy::DROP_NEWEST> buf_;
    // The bandwidth limit (packets per tick) is part of the final phase and will be added here.
};

#endif  // QUEUE_HPP
