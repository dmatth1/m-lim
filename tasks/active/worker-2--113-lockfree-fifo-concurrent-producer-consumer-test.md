# Task 113: LockFreeFIFO Concurrent Producer/Consumer Test

## Description
`LockFreeFIFO<T>` is documented as a **single-producer, single-consumer** lock-free ring
buffer. It is used in production code with the audio thread pushing `MeterData` and the UI
timer thread popping it. All existing tests are **single-threaded** — they push and pop
from the same thread sequentially.

The lack of a concurrent test means correctness under actual race conditions is unverified.
Even simple bugs (e.g., wrong memory order on the atomic read position) would not be caught.

Add a concurrent stress test to `tests/dsp/test_lockfree_fifo.cpp` that:

1. Runs a **producer thread** that pushes 100,000 `MeterData` items with sequential
   `inputLevelL` values (0, 1, 2, ..., 99999 cast to float).
2. Runs a **consumer thread** simultaneously that pops all available items and verifies
   that values arrive in FIFO order (no skips or duplicates in popped sequence).
3. After both threads complete, verifies that all pushed items (that were not dropped due
   to the FIFO being full) were received in the correct order and that no item was
   corrupted (e.g., NaN or Inf fields).

Also add a **data integrity test** that verifies the entire `MeterData` struct survives
concurrent round-trips without field corruption — not just `inputLevelL`, but also
`gainReduction`, `waveformSize`, and at least the first 3 `waveformBuffer` entries.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/MeterData.h` — LockFreeFIFO definition and MeterData struct
Modify: `tests/dsp/test_lockfree_fifo.cpp` — add concurrent tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "lockfree_fifo" --output-on-failure` → Expected: all tests pass with 0 failures
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | grep -E "passed|failed"` → Expected: no regressions

## Tests
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_concurrent_producer_consumer_ordering` — producer and consumer run in parallel; all popped items are in FIFO order
- Unit: `tests/dsp/test_lockfree_fifo.cpp::test_concurrent_data_integrity` — full MeterData struct (all populated fields) survives concurrent round-trips without corruption

## Technical Details
```cpp
TEST_CASE("test_concurrent_producer_consumer_ordering", "[lockfree_fifo]")
{
    LockFreeFIFO<MeterData> fifo(256);
    constexpr int N = 100000;

    std::thread producer([&]() {
        for (int i = 0; i < N; ++i) {
            MeterData d;
            d.inputLevelL = static_cast<float>(i);
            while (!fifo.push(d)) {}  // spin until space available
        }
    });

    int lastSeen = -1;
    int received = 0;
    std::thread consumer([&]() {
        while (received < N) {
            MeterData d;
            if (fifo.pop(d)) {
                REQUIRE(d.inputLevelL == Catch::Approx(static_cast<float>(lastSeen + 1)));
                lastSeen = static_cast<int>(d.inputLevelL);
                ++received;
            }
        }
    });

    producer.join();
    consumer.join();
    REQUIRE(received == N);
}
```

For data integrity test: populate `gainReduction`, `waveformSize`, and the first 3
`waveformBuffer` entries with unique values per iteration; verify all fields match after pop.

## Dependencies
None
