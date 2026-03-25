# Task 106: TransientLimiter Lookahead Scan Is O(lookahead) Per Sample

## Description
`TransientLimiter::process()` scans the entire lookahead delay buffer on every
single audio sample to find the maximum peak (step 2 of the inner loop,
`LimiterEngine.cpp` lines ~220–231):

```cpp
peakAbs = 0.0f;
int rpos = (scanWritePos - 1 + bufSize) % bufSize;
for (int k = 0; k < lookahead; ++k)
{
    const float val = std::abs(scanBuf[rpos]);
    if (val > peakAbs) peakAbs = val;
    rpos = (rpos - 1 + bufSize) % bufSize;
}
```

With a 5 ms lookahead at 48kHz oversampled 2× = 480 samples lookahead, the
inner loop runs 480 iterations per sample. For a 512-sample block at 96kHz:
512 samples × 480 = **245,760 iterations per block**. At 32× oversampling and
48kHz: 512 × 32 × 220 ≈ **3.6 million iterations per process() call**.

The canonical solution is a **sliding-window maximum** using a monotone deque
(also called a "deque-based sliding maximum" or Lemire's algorithm). This
reduces the per-sample work to O(1) amortized, making the total inner loop
O(numSamples) instead of O(numSamples × lookahead).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — replace the linear scan loop
  with a sliding-window maximum using a monotone deque.
Modify: `M-LIM/src/dsp/TransientLimiter.h` — add per-channel monotone deque
  members for main-path and sidechain-path peak tracking.
Read: `M-LIM/tests/dsp/test_transient_limiter.cpp` — existing tests must pass.

## Acceptance Criteria
- [ ] Run: `cd M-LIM && ctest --test-dir build -R test_transient_limiter --output-on-failure` → Expected: all tests pass.
- [ ] Run: `cd M-LIM && ctest --test-dir build -R test_limiter_engine --output-on-failure` → Expected: all tests pass.
- [ ] Run: `grep -n "for.*k.*lookahead\|k < lookahead" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: no output (the O(N) scan loop removed).

## Tests
- Unit: `M-LIM/tests/dsp/test_transient_limiter.cpp` — all existing tests pass.
- Unit: add test `test_transient_limiter_lookahead_peak` — feed a buffer with a
  single peak spike followed by silence; verify the gain reduction anticipates
  the spike by exactly the lookahead duration (within 1 sample tolerance).

## Technical Details
**Sliding-window maximum algorithm:**

Maintain a deque of (absolute-value, position) pairs sorted descending by value.
On each new sample:
1. Pop from the back while the back value ≤ new abs value (new sample dominates them).
2. Push (abs(newSample), writePos) onto the back.
3. Pop from the front while front position is outside the lookahead window.
4. Front of deque is the window maximum.

This needs separate deques for each channel, and separate deques for main vs
sidechain detection path (4 deques total for stereo).

The deques must be pre-allocated in `prepare()` to avoid audio-thread heap
allocations. A fixed-size circular buffer of capacity `mMaxLookaheadSamples + 1`
suffices (the deque can never exceed the window size).

**Important**: The delay buffer and write/read positions remain the same — only
the peak-finding scan is replaced. The read-position arithmetic for the delayed
output (step 5 in the process loop) is unchanged.

## Dependencies
None
