# Task 128: LoudnessMeter — Short-Term LUFS Reported Before Full 3-Second Window Violates EBU R128

## Description

`LoudnessMeter::onBlockComplete()` updates the short-term LUFS value (lines 167–169 of `LoudnessMeter.cpp`):

```cpp
// --- Update short-term LUFS ---
if (mShortTermRing.count > 0)
    mShortTermLUFS.store(powerToLUFS(mShortTermRing.mean()));
```

The `mShortTermRing` is a `FixedRingBuffer<30>` (30 × 100 ms = 3 s). Its `mean()` method divides by `count` (the number of blocks actually stored), not by 30. With this condition, a short-term LUFS value is published after just **100 ms** (1 block), based on only 1/30th of the required window. This gives a value equivalent to momentary loudness, not short-term loudness.

**EBU R128 / ITU-R BS.1770-4 requirement:** Short-term loudness is defined as the loudness over a 3-second rectangular window. A compliant implementation must accumulate 30 consecutive 100 ms blocks before reporting a valid short-term value.

**Fix:** Change the condition from `count > 0` to `full()`:

```cpp
if (mShortTermRing.full())
    mShortTermLUFS.store(powerToLUFS(mShortTermRing.mean()));
```

Before the ring buffer is full, `mShortTermLUFS` should remain at `kNegInf` (its initial value set in `prepare()`), indicating no valid measurement yet — exactly as `mMomentaryLUFS` already handles (line 164: `if (mMomentaryRing.full())`).

This makes the short-term and momentary LUFS conditions consistent:
- Momentary (400 ms, 4 blocks): already uses `mMomentaryRing.full()` ✓
- Short-term (3 s, 30 blocks): must also use `mShortTermRing.full()` ✗→✓

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — change short-term condition, line 168
Read: `M-LIM/src/dsp/LoudnessMeter.h` — verify FixedRingBuffer::full() is available
Read: `M-LIM/tests/dsp/test_loudness_meter.cpp` — existing tests
Read: `M-LIM/tests/dsp/test_loudness_meter_accuracy.cpp` — accuracy tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R LoudnessMeter --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: `grep -n "count > 0" /workspace/M-LIM/src/dsp/LoudnessMeter.cpp` → Expected: no output (condition removed)
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp::test_short_term_lufs_requires_full_window` — prepare meter at 44100 Hz, process 2900 ms of audio (29 × 100 ms blocks), assert `getShortTermLUFS()` returns `-infinity`; then process 100 ms more (one more block), assert `getShortTermLUFS()` returns a finite value
- Unit: `tests/dsp/test_loudness_meter.cpp::test_short_term_lufs_matches_momentary_after_one_block` — verify that after exactly 30 blocks, `getShortTermLUFS()` equals the loudness over the 30-block window (not just the last block)
- Unit: `tests/dsp/test_loudness_meter.cpp::test_short_term_initial_value_is_neg_inf` — immediately after `prepare()`, `getShortTermLUFS()` returns `-infinity`

## Technical Details

This is a one-line change in `LoudnessMeter.cpp`:

```cpp
// Before (incorrect):
if (mShortTermRing.count > 0)

// After (correct, consistent with momentary handling on line 164):
if (mShortTermRing.full())
```

`FixedRingBuffer<N>::full()` returns `count == N`, which is `true` only after exactly 30 blocks have been accumulated.

The `resetIntegrated()` method does not reset the short-term ring; this is correct behaviour — short-term loudness continues to track even when the integrated measurement is reset. No changes to `resetIntegrated()` are required.

## Dependencies
None
