# Task 091: TransientLimiter — O(lookahead) Per-Sample Peak Scan Is a Performance Bottleneck

## Description

In `TransientLimiter::process()` (lines 222–229), the lookahead peak detection scans the entire lookahead window for every output sample:

```cpp
int rpos = (scanWritePos - 1 + bufSize) % bufSize;
for (int k = 0; k < lookahead; ++k)        // O(lookahead) per sample
{
    const float val = std::abs(scanBuf[rpos]);
    if (val > peakAbs)
        peakAbs = val;
    rpos = (rpos - 1 + bufSize) % bufSize;  // modulo per iteration
}
```

**Cost at 5 ms lookahead:**
- `lookahead` = 5 ms × 44100 Hz = 220 samples (at 44.1 kHz original rate)
- At 2× oversampling: 440 iterations per output sample
- For a 512-sample stereo block: 440 × 512 × 2 = **450,560 iterations** just for peak detection
- Peak detection CPU cost scales as O(lookahead × sampleRate × channels), which dominates the entire limiter at high lookahead settings

**Fix:** Replace the O(N) brute-force scan with a **monotone deque (sliding window maximum)** algorithm that maintains the running maximum in O(1) amortized time per sample — O(n) total for a block of n samples, regardless of window size.

The sliding window max deque stores indices into the circular delay buffer in decreasing-value order. On each new sample:
1. Pop from the back any indices whose buffer value ≤ new value (they can never be the maximum).
2. Push the new index to the back.
3. Pop from the front any index that falls outside the lookahead window.
4. The front of the deque is always the current window maximum.

This requires a small deque data structure (max size = lookahead + 1). Use a pre-allocated ring buffer for the deque to avoid heap allocation on the audio thread.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.h` — add per-channel sliding-window-max deque state
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — replace O(N) scan loop with O(1) deque-based sliding max
Read: `M-LIM/src/dsp/TransientLimiter.h` — understand current buffer layout

## Acceptance Criteria
- [ ] Run: `cd /workspace && grep -c "for.*int k.*lookahead" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: `0` (inner O(lookahead) scan loop removed)
- [ ] Run: `cd /workspace/build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd /workspace && grep -n "deque\|SlidingMax\|slidingMax" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: sliding window max structure referenced in process loop

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_sliding_max_peak_detection` — feed a block with a known peak at a known position; verify the limiter begins reducing gain exactly when the peak is `lookahead` samples away (same result as brute-force scan)
- Unit: `tests/dsp/test_transient_limiter.cpp::test_sliding_max_matches_brute_force` — compare peak values from the new sliding-max path against the old brute-force reference over a randomized input; assert results are identical sample-for-sample

## Technical Details

**Pre-allocated ring-buffer deque** (avoids heap allocation in process loop):

```cpp
// In TransientLimiter.h, per channel:
struct SlidingMaxDeque
{
    std::vector<int>   indices;   // ring buffer, pre-allocated to maxLookahead+1
    int front = 0, back = 0, count = 0;
    int capacity = 0;

    void init(int cap) { capacity = cap + 1; indices.assign(capacity, 0); front = back = count = 0; }
    bool empty() const { return count == 0; }
    void pushBack(int idx) { indices[back] = idx; back = (back + 1) % capacity; ++count; }
    void popBack()         { back = (back - 1 + capacity) % capacity; --count; }
    void popFront()        { front = (front + 1) % capacity; --count; }
    int  peekFront() const { return indices[front]; }
    int  peekBack()  const { return indices[(back - 1 + capacity) % capacity]; }
};

std::vector<SlidingMaxDeque> mPeakDeques;          // main path, one per channel
std::vector<SlidingMaxDeque> mSidechainPeakDeques; // sidechain path, one per channel
```

**Per-sample deque update** (inside the process loop, replacing the k-loop):

```cpp
// On each new sample (after writing to delay buffer):
const int newIdx = /* index of the just-written sample */;
const std::vector<float>& scanBuf = /* main or sidechain buffer */;
SlidingMaxDeque& dq = mPeakDeques[ch];

// Remove old values from back that are ≤ new value (monotone invariant)
while (!dq.empty() && std::abs(scanBuf[dq.peekBack()]) <= std::abs(scanBuf[newIdx]))
    dq.popBack();
dq.pushBack(newIdx);

// Remove front index if outside window
const int outIdx = /* index of sample that has left the window */;
if (!dq.empty() && dq.peekFront() == outIdx)
    dq.popFront();

// Current window maximum:
peakAbs = dq.empty() ? 0.0f : std::abs(scanBuf[dq.peekFront()]);
```

The deque must be initialized to `maxLookaheadSamples + 1` capacity in `prepare()` and reset in `setLookahead()`. Because the delay buffer is circular, index comparisons for "out of window" use modular arithmetic consistent with the existing `bufSize` modulus.

## Dependencies
None
