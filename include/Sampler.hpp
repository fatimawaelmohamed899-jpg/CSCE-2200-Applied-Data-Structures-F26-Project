#ifndef SAMPLER_HPP
#define SAMPLER_HPP

#include "CircularBuffer.hpp"
#include "SignalSource.hpp"

// Each channel keeps its status as bits inside one byte, so eight flags cost only one byte of RAM.
enum ChannelFlag : unsigned char {
    FLAG_ACTIVE      = 1u << 0,  // bit 0: the channel is still sampling
    FLAG_SOURCE_DONE = 1u << 1,  // bit 1: the CSV source ran out of data
    FLAG_OVERFLOWED  = 1u << 2   // bit 2: the buffer overwrote an old sample at least once
};

// My sampler simulates the microcontroller: on every tick it samples each channel that is due and pushes into that channel's ring buffer.
template <usize BufN, usize MaxChannels>
class Sampler {
    static_assert(MaxChannels > 0 && MaxChannels <= 255, "MaxChannels must be between 1 and 255");

public:
    // Sensor buffers use OVERWRITE_OLDEST because for live data the newest readings matter most.
    typedef CircularBuffer<short, BufN, OverflowPolicy::OVERWRITE_OLDEST> SampleBuffer;

    Sampler() : count_(0), tick_(0) {}

    // I register a sensor that samples once every 2^periodShift ticks, and I return false if it cannot be added.
    bool addChannel(unsigned char id, unsigned char periodShift, const SignalSource& source) {
        if (count_ == MaxChannels) return false;     // there is no room for another channel
        if (periodShift > 31) return false;          // a shift of 32 or more would be invalid on a 32-bit tick
        if (indexOf(id) < count_) return false;      // every sensor id must be unique
        Channel& c = channels_[count_];
        c.id = id;
        c.periodShift = periodShift;
        c.flags = FLAG_ACTIVE;
        c.source = source;
        c.buffer.clear();
        ++count_;
        return true;
    }

    // I advance the simulated clock by one tick and sample every channel that is due.
    void tick() {
        for (usize i = 0; i < count_; ++i) {
            Channel& c = channels_[i];
            if ((c.flags & FLAG_ACTIVE) == 0) continue;     // test bit 0, and skip finished channels
            unsigned int mask = (1u << c.periodShift) - 1u;  // for a period of 2^k ticks, the mask is k ones
            if ((tick_ & mask) != 0) continue;               // the low k bits are zero exactly every 2^k ticks, so no division is needed

            short v = 0;
            if (c.source.next(tick_, v)) {
                if (c.buffer.isFull()) c.flags |= FLAG_OVERFLOWED;  // set bit 2 because this push will overwrite the oldest sample
                c.buffer.push(v);
            } else {
                c.flags |= FLAG_SOURCE_DONE;                                  // set bit 1 because the source is finished
                c.flags &= static_cast<unsigned char>(~FLAG_ACTIVE);          // clear bit 0 so the channel stops sampling
            }
        }
        ++tick_;
    }

    // I give other members access to a channel's buffer, or 0 if that sensor id does not exist.
    SampleBuffer* buffer(unsigned char id) {
        usize i = indexOf(id);
        return (i < count_) ? &channels_[i].buffer : 0;
    }

    // I return the raw flag byte of a channel, or 0 if the id does not exist.
    unsigned char flags(unsigned char id) const {
        usize i = indexOf(id);
        return (i < count_) ? channels_[i].flags : 0;
    }

    // I check one flag bit of a channel.
    bool hasFlag(unsigned char id, ChannelFlag flag) const {
        return (flags(id) & flag) != 0;
    }

    unsigned int tickCount() const { return tick_; }      // how many ticks have been simulated
    usize channelCount() const { return count_; }          // how many channels are registered

private:
    struct Channel {
        unsigned char id;           // the unique sensor id
        unsigned char periodShift;  // the sensor samples every 2^periodShift ticks
        unsigned char flags;        // status bits (see ChannelFlag)
        SignalSource  source;       // where the values come from
        SampleBuffer  buffer;       // this sensor's own ring buffer
    };

    // I search for a channel by id, and I return count_ when it is not found.
    usize indexOf(unsigned char id) const {
        for (usize i = 0; i < count_; ++i) {
            if (channels_[i].id == id) return i;
        }
        return count_;
    }

    Channel      channels_[MaxChannels];  // all channels live in a fixed array, so there is no dynamic memory
    usize        count_;                  // how many channels are registered
    unsigned int tick_;                   // the simulated clock
};

#endif  // SAMPLER_HPP
