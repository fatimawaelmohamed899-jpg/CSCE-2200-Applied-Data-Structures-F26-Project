#ifndef SIGNAL_SOURCE_HPP
#define SIGNAL_SOURCE_HPP

#include "CircularBuffer.hpp"  // I reuse usize from my ring buffer file

// My random generator below assumes a 32-bit unsigned int, so I check it at compile time.
static_assert(sizeof(unsigned int) == 4, "unsigned int must be 32 bits");

// Xorshift32 is my pseudo-random generator, and it only needs shifts and XORs (bitwise operations) and 4 bytes of state.
class Xorshift32 {
public:
    explicit Xorshift32(unsigned int seed = 2463534242u) : state_(seed != 0 ? seed : 1u) {}  // a zero state would stay zero forever, so I replace it with 1

    unsigned int next() {
        state_ ^= state_ << 13;  // mix the bits by shifting left and XORing
        state_ ^= state_ >> 17;  // mix again by shifting right and XORing
        state_ ^= state_ << 5;   // one more shift and XOR to finish the step
        return state_;
    }

private:
    unsigned int state_;  // the whole generator is just this one number
};

// One line of the CSV file after parsing: the time, which sensor, and its value.
struct CsvRecord {
    unsigned int  tick;
    unsigned char sensorId;
    short         value;
};

// CsvReader walks through CSV text that is already in memory and parses it character by character.
class CsvReader {
public:
    CsvReader() : cur_(0), end_(0), bad_(0) {}  // an empty reader that has nothing to read
    CsvReader(const char* text, usize length) : cur_(text), end_(text + length), bad_(0) {}

    // I read the next valid line into out, I skip blank and header lines, and I count malformed lines in bad_.
    bool next(CsvRecord& out) {
        while (cur_ < end_) {
            const char* lineStart = cur_;
            const char* lineEnd = lineStart;
            while (lineEnd < end_ && *lineEnd != '\n') ++lineEnd;  // find the end of this line
            cur_ = (lineEnd < end_) ? lineEnd + 1 : lineEnd;       // the next call starts after it

            const char* p = lineStart;
            skipSpaces(p, lineEnd);
            if (p == lineEnd) continue;    // a blank line is skipped without counting it as bad
            if (isLetter(*p)) continue;    // a line starting with a letter is the header, so I skip it
            if (parseLine(p, lineEnd, out)) return true;
            ++bad_;                        // anything else that failed to parse is a bad line
        }
        return false;  // I reached the end of the text
    }

    // I keep reading until I find a record that belongs to the given sensor.
    bool nextForSensor(unsigned char id, CsvRecord& out) {
        while (next(out)) {
            if (out.sensorId == id) return true;
        }
        return false;
    }

    unsigned int badLines() const { return bad_; }  // how many malformed lines I skipped so far

private:
    static bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\r'; }  // I count \r as a space so Windows line endings work
    static bool isDigit(char c) { return c >= '0' && c <= '9'; }

    // ASCII trick: OR-ing with 0x20 (a bit operation) turns an uppercase letter into lowercase, so one range check covers both cases.
    static bool isLetter(char c) {
        unsigned char lower = static_cast<unsigned char>(c) | 0x20;
        return lower >= 'a' && lower <= 'z';
    }

    static void skipSpaces(const char*& p, const char* e) {
        while (p < e && isSpace(*p)) ++p;
    }

    // I parse a non-negative number and fail if there are no digits or if it goes above maxVal.
    static bool parseUnsigned(const char*& p, const char* e, unsigned long long maxVal, unsigned long long& out) {
        if (p == e || !isDigit(*p)) return false;
        unsigned long long v = 0;
        while (p < e && isDigit(*p)) {
            v = v * 10 + static_cast<unsigned long long>(*p - '0');
            if (v > maxVal) return false;  // I check after every digit so the number can never overflow
            ++p;
        }
        out = v;
        return true;
    }

    // I parse one line in the form tick,sensorId,value and I only accept it if the whole line is consumed.
    static bool parseLine(const char* p, const char* e, CsvRecord& r) {
        unsigned long long tick = 0, id = 0, mag = 0;

        if (!parseUnsigned(p, e, 0xFFFFFFFFull, tick)) return false;
        skipSpaces(p, e);
        if (p == e || *p != ',') return false;
        ++p;
        skipSpaces(p, e);

        if (!parseUnsigned(p, e, 255, id)) return false;  // the sensor id must fit in one byte
        skipSpaces(p, e);
        if (p == e || *p != ',') return false;
        ++p;
        skipSpaces(p, e);

        bool negative = false;
        if (p < e && (*p == '-' || *p == '+')) {
            negative = (*p == '-');
            ++p;
        }
        if (!parseUnsigned(p, e, negative ? 32768ull : 32767ull, mag)) return false;  // the value must fit in a short
        skipSpaces(p, e);
        if (p != e) return false;  // extra characters after the value make the line bad

        r.tick = static_cast<unsigned int>(tick);
        r.sensorId = static_cast<unsigned char>(id);
        r.value = negative ? static_cast<short>(-static_cast<int>(mag)) : static_cast<short>(mag);
        return true;
    }

    const char*  cur_;  // where the next line starts
    const char*  end_;  // one past the last character of the text
    unsigned int bad_;  // number of malformed lines skipped
};

// SignalSource produces the "physical" value of one sensor, either from a generated signal or from the CSV text.
class SignalSource {
private:
    enum Kind : unsigned char { CONSTANT, RAMP, SQUARE, NOISY, FROM_CSV };

public:
    SignalSource() : kind_(CONSTANT), a_(0), b_(0), shift_(0), sensorId_(0), count_(0), rng_(), csv_() {}  // by default I produce the value 0

    // A signal that always returns the same value.
    static SignalSource constant(short value) {
        SignalSource s;
        s.kind_ = CONSTANT;
        s.a_ = value;
        return s;
    }

    // A signal that starts at start and changes by step on every sample.
    static SignalSource ramp(short start, short step) {
        SignalSource s;
        s.kind_ = RAMP;
        s.a_ = start;
        s.b_ = step;
        return s;
    }

    // A square wave that switches between low and high every 2^halfPeriodShift ticks.
    static SignalSource square(short low, short high, unsigned char halfPeriodShift) {
        SignalSource s;
        s.kind_ = SQUARE;
        s.a_ = low;
        s.b_ = high;
        s.shift_ = halfPeriodShift;
        return s;
    }

    // A noisy signal: base plus a random offset of noiseBits bits, with a spike added about once every 64 samples.
    static SignalSource noisy(short base, unsigned char noiseBits, short spike, unsigned int seed) {
        SignalSource s;
        s.kind_ = NOISY;
        s.a_ = base;
        s.b_ = spike;
        s.shift_ = (noiseBits > 15) ? 15 : noiseBits;  // I limit the noise to 15 bits so it stays in range
        s.rng_ = Xorshift32(seed);
        return s;
    }

    // A signal read from the CSV text: it returns the values of the given sensor in file order.
    static SignalSource fromCsv(const char* text, usize length, unsigned char sensorId) {
        SignalSource s;
        s.kind_ = FROM_CSV;
        s.sensorId_ = sensorId;
        s.csv_ = CsvReader(text, length);
        return s;
    }

    // I produce the next sample into out, and I return false only when a CSV source has no more data.
    bool next(unsigned int tick, short& out) {
        switch (kind_) {
        case CONSTANT:
            out = a_;
            return true;

        case RAMP:
            // I use unsigned arithmetic so the value wraps predictably instead of overflowing.
            out = static_cast<short>(static_cast<unsigned int>(a_) + static_cast<unsigned int>(b_) * count_);
            ++count_;
            return true;

        case SQUARE:
            out = ((tick >> shift_) & 1u) ? b_ : a_;  // bit number shift_ of the tick decides low or high
            return true;

        case NOISY: {
            unsigned int r = rng_.next();
            unsigned int mask = (1u << shift_) - 1u;                              // a mask with noiseBits ones
            int noise = static_cast<int>(r & mask) - static_cast<int>(mask >> 1);  // keep the low bits, then center around zero
            int v = a_ + noise;
            if (((r >> 24) & 63u) == 0u) v += b_;                                  // 6 bits all zero happens 1 time in 64, so that is my spike
            out = static_cast<short>(v);
            return true;
        }

        case FROM_CSV: {
            CsvRecord rec;
            if (!csv_.nextForSensor(sensorId_, rec)) return false;  // the file has no more values for this sensor
            out = rec.value;
            return true;
        }
        }
        return false;
    }

private:
    Kind          kind_;      // which type of signal this is
    short         a_;         // first parameter (value, start, low or base)
    short         b_;         // second parameter (step, high or spike)
    unsigned char shift_;     // bit count or shift used by the square and noisy signals
    unsigned char sensorId_;  // which sensor to read from the CSV
    unsigned int  count_;     // how many samples the ramp has produced
    Xorshift32    rng_;       // random numbers for the noisy signal
    CsvReader     csv_;       // my own reader over the CSV text, so each sensor reads independently
};

#endif  // SIGNAL_SOURCE_HPP
