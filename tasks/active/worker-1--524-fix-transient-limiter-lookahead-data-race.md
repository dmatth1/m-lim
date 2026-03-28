# Task 524: Fix Data Race on TransientLimiter::mLookaheadSamples

## Description
`TransientLimiter::mLookaheadSamples` is a plain `int` that is:
- **Written** on the audio thread via `applyPendingParams()` → `TransientLimiter::setLookahead()`
- **Read** on the message thread via `updateLatency()` → `LimiterEngine::getLatencySamples()` → `TransientLimiter::getLatencyInSamples()`

This is technically undefined behavior under the C++ memory model (concurrent non-atomic read+write). While benign on x86 (aligned int operations are naturally atomic), this is a real data race that sanitizers (TSan) will flag and that could theoretically cause issues on ARM platforms.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/TransientLimiter.h` — Change `int mLookaheadSamples` (line 96) to `std::atomic<int>`
Modify: `src/dsp/TransientLimiter.cpp` — Update `setLookahead()` (line 61) and `getLatencyInSamples()` (line 134) to use atomic load/store. Also update `process()` (line 264, 332) to load the atomic once into a local variable at the start.

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: ThreadSanitizer build shows no data race warnings for mLookaheadSamples

## Tests
None (existing tests cover the functionality; this is a correctness fix for concurrent access)

## Technical Details
- Change `int mLookaheadSamples` to `std::atomic<int> mLookaheadSamples{0}`
- In `setLookahead()`: use `.store()` with `memory_order_relaxed`
- In `getLatencyInSamples()`: use `.load()` with `memory_order_relaxed`
- In `process()` and `processBypassDelay()`: load once into a local `const int lookahead = mLookaheadSamples.load(std::memory_order_relaxed)` (the code already does `const int lookahead = mLookaheadSamples` — just needs the atomic load)
- Also make `mMaxLookaheadSamples` atomic for the same reason (read in `setLookahead` which can be called from `prepare` on the message thread while `process` reads it via the `lookahead` local)

## Dependencies
None
