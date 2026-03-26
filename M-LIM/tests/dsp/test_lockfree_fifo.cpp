#include "catch2/catch_amalgamated.hpp"
#include "dsp/MeterData.h"
#include <cmath>
#include <limits>
#include <thread>
#include <atomic>

// ============================================================
// MeterData default values
// ============================================================

TEST_CASE("test_meter_data_default_values", "[lockfree_fifo]")
{
    MeterData d;
    CHECK(d.inputLevelL   == 0.0f);
    CHECK(d.inputLevelR   == 0.0f);
    CHECK(d.outputLevelL  == 0.0f);
    CHECK(d.outputLevelR  == 0.0f);
    CHECK(d.gainReduction == 0.0f);
    CHECK(d.truePeakL     == 0.0f);
    CHECK(d.truePeakR     == 0.0f);
    CHECK(d.waveformSample == 0.0f);
    CHECK_FALSE(std::isnan(d.waveformSample));
}

// ============================================================
// push / pop single item
// ============================================================

TEST_CASE("test_push_pop_single_item", "[lockfree_fifo]")
{
    LockFreeFIFO<MeterData> fifo(4);

    MeterData in;
    in.inputLevelL   = 0.5f;
    in.inputLevelR   = 0.6f;
    in.outputLevelL  = 0.3f;
    in.outputLevelR  = 0.4f;
    in.gainReduction = -3.0f;
    in.truePeakL     = 0.7f;
    in.truePeakR     = 0.8f;
    in.waveformSample = 1.0f;

    REQUIRE(fifo.push(in));

    MeterData out;
    REQUIRE(fifo.pop(out));

    CHECK(out.inputLevelL   == Catch::Approx(0.5f));
    CHECK(out.inputLevelR   == Catch::Approx(0.6f));
    CHECK(out.outputLevelL  == Catch::Approx(0.3f));
    CHECK(out.outputLevelR  == Catch::Approx(0.4f));
    CHECK(out.gainReduction == Catch::Approx(-3.0f));
    CHECK(out.truePeakL     == Catch::Approx(0.7f));
    CHECK(out.truePeakR     == Catch::Approx(0.8f));
    CHECK(out.waveformSample == Catch::Approx(1.0f));
}

// ============================================================
// FIFO ordering
// ============================================================

TEST_CASE("test_fifo_ordering", "[lockfree_fifo]")
{
    constexpr int N = 8;
    LockFreeFIFO<MeterData> fifo(N + 4);  // plenty of room

    for (int i = 0; i < N; ++i)
    {
        MeterData d;
        d.inputLevelL = static_cast<float>(i);
        d.waveformSample = static_cast<float>(i) * 0.5f;
        REQUIRE(fifo.push(d));
    }

    for (int i = 0; i < N; ++i)
    {
        MeterData d;
        REQUIRE(fifo.pop(d));
        CHECK(d.inputLevelL   == Catch::Approx(static_cast<float>(i)));
        CHECK(d.waveformSample == Catch::Approx(static_cast<float>(i) * 0.5f));
    }

    // Should be empty now
    MeterData dummy;
    CHECK_FALSE(fifo.pop(dummy));
}

// ============================================================
// Pop from empty FIFO returns false
// ============================================================

TEST_CASE("test_pop_empty_returns_false", "[lockfree_fifo]")
{
    LockFreeFIFO<MeterData> fifo(8);

    MeterData d;
    CHECK_FALSE(fifo.pop(d));

    // Fields should not be garbage (default-initialised)
    CHECK(d.inputLevelL == 0.0f);
}

// ============================================================
// Push beyond capacity should not crash and should return false
// ============================================================

TEST_CASE("test_push_full_drops_or_fails", "[lockfree_fifo]")
{
    // Capacity is rounded up to next power of two, so capacity(4) → 4.
    // A ring buffer of size N can hold N-1 items before reporting full.
    LockFreeFIFO<MeterData> fifo(4);
    const int cap = fifo.capacity();  // should be 4

    // Fill the FIFO until it reports full
    int pushed = 0;
    for (int i = 0; i < cap + 4; ++i)
    {
        MeterData d;
        d.inputLevelL = static_cast<float>(i);
        if (fifo.push(d))
            ++pushed;
    }

    // At most cap-1 items fit (ring buffer semantics)
    CHECK(pushed <= cap - 1);
    // At least 1 item must have been pushed
    CHECK(pushed >= 1);

    // Drain the FIFO — should not crash
    int popped = 0;
    MeterData d;
    while (fifo.pop(d))
        ++popped;

    CHECK(popped == pushed);
}

// ============================================================
// Waveform sample survives push/pop roundtrip
// ============================================================

TEST_CASE("test_waveform_sample_roundtrip", "[lockfree_fifo]")
{
    LockFreeFIFO<MeterData> fifo(4);

    MeterData in;
    in.waveformSample = -6.5f;

    REQUIRE(fifo.push(in));

    MeterData out;
    REQUIRE(fifo.pop(out));

    CHECK(out.waveformSample == Catch::Approx(-6.5f));
}

// ============================================================
// Rapid push/pop cycle — no corruption over many iterations
// ============================================================

TEST_CASE("test_rapid_push_pop_cycle", "[lockfree_fifo]")
{
    LockFreeFIFO<MeterData> fifo(64);
    constexpr int ITERATIONS = 10000;

    int mismatch = 0;
    for (int i = 0; i < ITERATIONS; ++i)
    {
        MeterData in;
        in.inputLevelL   = static_cast<float>(i % 1000) * 0.001f;
        in.gainReduction = static_cast<float>(-(i % 100));
        in.waveformSample = static_cast<float>(i % 30) * -0.1f;

        // Push may fail if full (non-blocking); drain first if needed
        if (!fifo.push(in))
        {
            MeterData drain;
            fifo.pop(drain);  // make room
            fifo.push(in);
        }

        MeterData out;
        if (fifo.pop(out))
        {
            // Values should not be NaN or Inf
            if (!std::isfinite(out.inputLevelL)   ||
                !std::isfinite(out.gainReduction))
                ++mismatch;
        }
    }

    CHECK(mismatch == 0);
}

// ============================================================
// Concurrent producer/consumer — FIFO ordering
// ============================================================

TEST_CASE("test_concurrent_producer_consumer_ordering", "[lockfree_fifo]")
{
    LockFreeFIFO<MeterData> fifo(256);
    constexpr int N = 100000;

    std::thread producer([&]() {
        for (int i = 0; i < N; ++i)
        {
            MeterData d;
            d.inputLevelL = static_cast<float>(i);
            while (!fifo.push(d)) {}  // spin until space available
        }
    });

    int lastSeen = -1;
    int received = 0;
    std::thread consumer([&]() {
        while (received < N)
        {
            MeterData d;
            if (fifo.pop(d))
            {
                REQUIRE(d.inputLevelL == Catch::Approx(static_cast<float>(lastSeen + 1)));
                REQUIRE_FALSE(std::isnan(d.inputLevelL));
                REQUIRE_FALSE(std::isinf(d.inputLevelL));
                lastSeen = static_cast<int>(d.inputLevelL);
                ++received;
            }
        }
    });

    producer.join();
    consumer.join();
    REQUIRE(received == N);
}

// ============================================================
// Concurrent producer/consumer — full struct data integrity
// ============================================================

TEST_CASE("test_concurrent_data_integrity", "[lockfree_fifo]")
{
    LockFreeFIFO<MeterData> fifo(256);
    constexpr int N = 100000;
    std::atomic<int> corrupted{0};

    std::thread producer([&]() {
        for (int i = 0; i < N; ++i)
        {
            MeterData d;
            d.inputLevelL      = static_cast<float>(i);
            d.gainReduction    = static_cast<float>(-(i % 10000));
            d.waveformSample   = static_cast<float>(i) * 0.1f;
            while (!fifo.push(d)) {}
        }
    });

    int received = 0;
    std::thread consumer([&]() {
        while (received < N)
        {
            MeterData d;
            if (fifo.pop(d))
            {
                ++received;
                int i = static_cast<int>(d.inputLevelL);

                // Verify all populated fields are finite and self-consistent
                bool ok = std::isfinite(d.inputLevelL)
                       && std::isfinite(d.gainReduction)
                       && d.gainReduction == Catch::Approx(static_cast<float>(-(i % 10000))).margin(1e-3f)
                       && d.waveformSample == Catch::Approx(static_cast<float>(i) * 0.1f).margin(1e-3f);

                if (!ok)
                    ++corrupted;
            }
        }
    });

    producer.join();
    consumer.join();

    REQUIRE(received == N);
    CHECK(corrupted.load() == 0);
}

// ============================================================
// Non-power-of-two capacity rounds up to next power of two
// ============================================================

TEST_CASE("test_non_power_of_two_capacity_rounds_up", "[lockfree_fifo]")
{
    // Each of these is non-power-of-two; capacity() must return a power of two > requested
    for (int requested : {3, 5, 6, 7})
    {
        LockFreeFIFO<MeterData> fifo(requested);
        const int cap = fifo.capacity();

        // cap must be >= requested
        CHECK(cap >= requested);
        // cap must be a power of two
        CHECK((cap & (cap - 1)) == 0);
        // cap must be strictly greater than requested (since requested is not a power of two)
        CHECK(cap > requested);
    }
}

// ============================================================
// Fill, drain, fill again — exercises ring-buffer wraparound
// ============================================================

TEST_CASE("test_fill_drain_fill_wraparound", "[lockfree_fifo]")
{
    // capacity=4 rounds to 4 (already a power of two); ring holds 3 items max
    LockFreeFIFO<MeterData> fifo(4);
    REQUIRE(fifo.capacity() == 4);

    // --- First fill: push 3 items with distinct sentinel values ---
    for (int i = 0; i < 3; ++i)
    {
        MeterData d;
        d.inputLevelL = 10.0f + static_cast<float>(i);  // 10, 11, 12
        REQUIRE(fifo.push(d));
    }
    // FIFO should now be full
    CHECK(fifo.isFull());

    // --- Drain all 3 ---
    for (int i = 0; i < 3; ++i)
    {
        MeterData d;
        REQUIRE(fifo.pop(d));
        CHECK(d.inputLevelL == Catch::Approx(10.0f + static_cast<float>(i)));
    }
    CHECK(fifo.isEmpty());

    // --- Second fill: head/tail pointers have advanced; this crosses the ring boundary ---
    for (int i = 0; i < 3; ++i)
    {
        MeterData d;
        d.inputLevelL = 20.0f + static_cast<float>(i);  // 20, 21, 22
        REQUIRE(fifo.push(d));
    }
    CHECK(fifo.isFull());

    // --- Drain second batch: verify no corruption at wraparound ---
    for (int i = 0; i < 3; ++i)
    {
        MeterData d;
        REQUIRE(fifo.pop(d));
        CHECK(d.inputLevelL == Catch::Approx(20.0f + static_cast<float>(i)));
    }
    CHECK(fifo.isEmpty());
}

// ============================================================
// Capacity=1 construction — degenerate edge case
// ============================================================

TEST_CASE("test_capacity_one_construction", "[lockfree_fifo]")
{
    // nextPowerOfTwo(1) == 2, so capacity() should return 2; ring holds 1 item
    LockFreeFIFO<MeterData> fifo(1);
    const int cap = fifo.capacity();

    CHECK(cap >= 1);
    // capacity must be a power of two
    CHECK((cap & (cap - 1)) == 0);

    // Must be able to push and pop at least one item without crash
    MeterData in;
    in.inputLevelL = 42.0f;
    REQUIRE(fifo.push(in));

    MeterData out;
    REQUIRE(fifo.pop(out));
    CHECK(out.inputLevelL == Catch::Approx(42.0f));
    CHECK(fifo.isEmpty());
}

// ============================================================
// isEmpty / isFull transitions
// ============================================================

TEST_CASE("test_isempty_isfull_transitions", "[lockfree_fifo]")
{
    LockFreeFIFO<MeterData> fifo(4);  // capacity=4, max items=3

    CHECK(fifo.isEmpty());
    CHECK_FALSE(fifo.isFull());

    MeterData d;
    // Fill until full
    while (!fifo.isFull())
        REQUIRE(fifo.push(d));

    CHECK_FALSE(fifo.isEmpty());
    CHECK(fifo.isFull());

    // Drain one — no longer full
    REQUIRE(fifo.pop(d));
    CHECK_FALSE(fifo.isFull());

    // Drain rest
    while (!fifo.isEmpty())
        fifo.pop(d);

    CHECK(fifo.isEmpty());
}
