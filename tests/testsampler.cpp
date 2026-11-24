// Build with: g++ -std=c++11 -Wall -Wextra -I include tests/test_sampler.cpp -o t2
// The exit code is the result: 0 means every test passed, and k means test number k failed first.
#include "CircularBuffer.hpp"
#include "SignalSource.hpp"
#include "Sampler.hpp"

// If the condition is false, the current test returns false.
#define CHECK(cond) do { if (!(cond)) return false; } while (0)

// Test 1: I check that the random generator is repeatable, depends on the seed, and survives a zero seed.
static bool t_prng() {
    Xorshift32 a(123), b(123), c(456);
    bool same = true, different = false, nonzero = true;
    for (int i = 0; i < 1000; ++i) {
        unsigned int x = a.next(), y = b.next(), z = c.next();
        if (x != y) same = false;
        if (x != z) different = true;
        if (x == 0) nonzero = false;
    }
    CHECK(same);
    CHECK(different);
    CHECK(nonzero);
    Xorshift32 zero(0);
    CHECK(zero.next() != 0);
    return true;
}

// Test 2: I check that the CSV reader parses a header and normal lines, including a negative value.
static bool t_csv_basic() {
    static const char text[] = "tick,sensorId,value\n0,1,25\n10,2,-300\n";
    CsvReader r(text, sizeof(text) - 1);
    CsvRecord rec;
    CHECK(r.next(rec) && rec.tick == 0 && rec.sensorId == 1 && rec.value == 25);
    CHECK(r.next(rec) && rec.tick == 10 && rec.sensorId == 2 && rec.value == -300);
    CHECK(!r.next(rec));
    CHECK(r.badLines() == 0);
    return true;
}

// Test 3: I check that malformed lines are skipped and counted, and good lines around them still work.
static bool t_csv_bad_lines() {
    static const char text[] = "0,1,25\n5,1\n7,1,99999\n,1,4\n9,1,12\n";
    CsvReader r(text, sizeof(text) - 1);
    CsvRecord rec;
    CHECK(r.next(rec) && rec.tick == 0 && rec.value == 25);
    CHECK(r.next(rec) && rec.tick == 9 && rec.value == 12);
    CHECK(!r.next(rec));
    CHECK(r.badLines() == 3);  // a missing field, a value too big, and an empty tick
    return true;
}

// Test 4: I check spaces, tabs, Windows line endings, blank lines, plus signs, the short limits, and a missing final newline.
static bool t_csv_formatting() {
    static const char text[] = "  0 , 1 , +7 \r\n\r\n\t5,2,-32768\n6,3,32767";
    CsvReader r(text, sizeof(text) - 1);
    CsvRecord rec;
    CHECK(r.next(rec) && rec.tick == 0 && rec.sensorId == 1 && rec.value == 7);
    CHECK(r.next(rec) && rec.tick == 5 && rec.sensorId == 2 && rec.value == -32768);
    CHECK(r.next(rec) && rec.tick == 6 && rec.sensorId == 3 && rec.value == 32767);
    CHECK(!r.next(rec));
    CHECK(r.badLines() == 0);
    return true;
}

// Test 5: I check that nextForSensor returns only the records of the requested sensor.
static bool t_csv_filter() {
    static const char text[] = "0,1,10\n1,2,20\n2,1,11\n3,2,21\n";
    CsvReader r(text, sizeof(text) - 1);
    CsvRecord rec;
    CHECK(r.nextForSensor(2, rec) && rec.value == 20);
    CHECK(r.nextForSensor(2, rec) && rec.value == 21);
    CHECK(!r.nextForSensor(2, rec));
    return true;
}

// Test 6: I check the constant, ramp and square signals, including a negative ramp step.
static bool t_simple_signals() {
    short v = 0;
    SignalSource c = SignalSource::constant(42);
    CHECK(c.next(0, v) && v == 42);
    CHECK(c.next(99, v) && v == 42);

    SignalSource r = SignalSource::ramp(10, 5);
    CHECK(r.next(0, v) && v == 10);
    CHECK(r.next(0, v) && v == 15);
    CHECK(r.next(0, v) && v == 20);

    SignalSource d = SignalSource::ramp(0, -2);
    CHECK(d.next(0, v) && v == 0);
    CHECK(d.next(0, v) && v == -2);
    CHECK(d.next(0, v) && v == -4);

    SignalSource s = SignalSource::square(-1, 1, 2);  // switches every 4 ticks
    for (unsigned int t = 0; t < 4; ++t) CHECK(s.next(t, v) && v == -1);
    for (unsigned int t = 4; t < 8; ++t) CHECK(s.next(t, v) && v == 1);
    CHECK(s.next(8, v) && v == -1);
    return true;
}

// Test 7: I check that the noisy signal stays in range, is repeatable, and produces spikes when asked to.
static bool t_noisy() {
    SignalSource a = SignalSource::noisy(100, 3, 0, 1);  // noise of 3 bits gives offsets from -3 to +4
    SignalSource b = SignalSource::noisy(100, 3, 0, 1);
    short x = 0, y = 0;
    for (int i = 0; i < 500; ++i) {
        CHECK(a.next(0, x) && b.next(0, y));
        CHECK(x >= 97 && x <= 104);
        CHECK(x == y);
    }
    SignalSource s = SignalSource::noisy(100, 3, 50, 1);
    int spikes = 0;
    for (int i = 0; i < 2000; ++i) {
        CHECK(s.next(0, x));
        if (x > 104) ++spikes;
    }
    CHECK(spikes > 0);
    CHECK(spikes < 200);  // spikes should be rare, about 1 in 64
    return true;
}

// Test 8: I check that a channel with period 2^2 samples exactly every 4 ticks.
static bool t_sampler_rate() {
    Sampler<8, 2> s;
    CHECK(s.addChannel(1, 2, SignalSource::ramp(0, 1)));
    for (int i = 0; i < 16; ++i) s.tick();
    CHECK(s.tickCount() == 16);
    Sampler<8, 2>::SampleBuffer* b = s.buffer(1);
    CHECK(b != 0);
    CHECK(b->size() == 4);  // ticks 0, 4, 8 and 12
    short v = 0;
    CHECK(b->pop(v) && v == 0);
    CHECK(b->pop(v) && v == 1);
    CHECK(b->pop(v) && v == 2);
    CHECK(b->pop(v) && v == 3);
    return true;
}

// Test 9: I check that two sensors with different rates fill their own buffers independently.
static bool t_two_rates() {
    Sampler<16, 2> s;
    CHECK(s.addChannel(1, 1, SignalSource::constant(5)));  // every 2 ticks
    CHECK(s.addChannel(2, 3, SignalSource::constant(9)));  // every 8 ticks
    for (int i = 0; i < 16; ++i) s.tick();
    CHECK(s.buffer(1)->size() == 8);
    CHECK(s.buffer(2)->size() == 2);
    return true;
}

// Test 10: I check that a full buffer overwrites the oldest samples and raises the overflow flag bit.
static bool t_overflow_flag() {
    Sampler<4, 1> s;
    CHECK(s.addChannel(1, 0, SignalSource::ramp(0, 1)));  // samples on every tick
    for (int i = 0; i < 4; ++i) s.tick();
    CHECK(!s.hasFlag(1, FLAG_OVERFLOWED));                 // exactly full, nothing overwritten yet
    for (int i = 0; i < 6; ++i) s.tick();                  // ticks 4 to 9 force overwrites
    CHECK(s.hasFlag(1, FLAG_OVERFLOWED));
    Sampler<4, 1>::SampleBuffer* b = s.buffer(1);
    short v = 0;
    CHECK(b->pop(v) && v == 6);
    CHECK(b->pop(v) && v == 7);
    CHECK(b->pop(v) && v == 8);
    CHECK(b->pop(v) && v == 9);
    return true;
}

// Test 11: I check that a CSV-driven channel reads its sensor's values in order and then stops.
static bool t_csv_channel() {
    static const char text[] = "tick,sensorId,value\n0,1,5\n1,2,99\n2,1,6\n3,1,7\n";
    Sampler<8, 1> s;
    CHECK(s.addChannel(1, 0, SignalSource::fromCsv(text, sizeof(text) - 1, 1)));
    for (int i = 0; i < 6; ++i) s.tick();
    Sampler<8, 1>::SampleBuffer* b = s.buffer(1);
    CHECK(b->size() == 3);  // sensor 2's line was ignored
    short v = 0;
    CHECK(b->pop(v) && v == 5);
    CHECK(b->pop(v) && v == 6);
    CHECK(b->pop(v) && v == 7);
    return true;
}

// Test 12: I check the flag bits: a new channel is only ACTIVE, and a finished CSV channel is only SOURCE_DONE.
static bool t_flag_bits() {
    static const char text[] = "0,1,5\n";
    Sampler<4, 1> s;
    CHECK(s.addChannel(1, 0, SignalSource::fromCsv(text, sizeof(text) - 1, 1)));
    CHECK(s.flags(1) == FLAG_ACTIVE);
    s.tick();  // reads the only value
    CHECK(s.flags(1) == FLAG_ACTIVE);
    s.tick();  // the source is empty now
    CHECK(s.flags(1) == FLAG_SOURCE_DONE);  // bit 0 cleared and bit 1 set
    CHECK(s.hasFlag(1, FLAG_SOURCE_DONE));
    CHECK(!s.hasFlag(1, FLAG_ACTIVE));
    return true;
}

// Test 13: I check the rules of addChannel and the lookup of an unknown id.
static bool t_add_rules() {
    Sampler<4, 2> s;
    CHECK(s.addChannel(1, 0, SignalSource()));
    CHECK(!s.addChannel(1, 0, SignalSource()));   // duplicate id
    CHECK(s.addChannel(2, 0, SignalSource()));
    CHECK(!s.addChannel(3, 0, SignalSource()));   // no room left
    CHECK(s.channelCount() == 2);
    CHECK(s.buffer(9) == 0);                      // unknown id
    CHECK(s.flags(9) == 0);

    Sampler<4, 2> t;
    CHECK(!t.addChannel(1, 32, SignalSource()));  // shift too large
    CHECK(t.addChannel(1, 31, SignalSource()));   // the largest allowed shift
    return true;
}

// Test 14: I check the memory used by one sensor buffer: 8 shorts plus two 1-byte indices.
static bool t_memory() {
    CHECK(sizeof(Sampler<8, 1>::SampleBuffer) == 8 * sizeof(short) + 2);
    return true;
}

// I run every test in order and return the number of the first one that fails, or 0 if all pass.
int main() {
    bool (*const tests[])() = {
        t_prng, t_csv_basic, t_csv_bad_lines, t_csv_formatting, t_csv_filter,
        t_simple_signals, t_noisy, t_sampler_rate, t_two_rates, t_overflow_flag,
        t_csv_channel, t_flag_bits, t_add_rules, t_memory
    };
    const int count = static_cast<int>(sizeof(tests) / sizeof(tests[0]));
    for (int i = 0; i < count; ++i) {
        if (!tests[i]()) return i + 1;
    }
    return 0;
}
