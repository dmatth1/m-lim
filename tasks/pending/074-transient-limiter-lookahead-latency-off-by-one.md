# Task 074: TransientLimiter — Lookahead Latency Reported 1 Sample Too High

## Description

In `TransientLimiter::process()`, the delay buffer write/read pattern introduces `lookaheadSamples - 1` samples of audio delay, but `LimiterEngine::getLatencySamples()` reports `lookaheadSamples`. This 1-sample over-report causes the DAW to over-compensate latency, so the limited track arrives 1 sample early relative to other tracks in the session.

**The bug:**

```cpp
// In the process loop, for each sample:
mDelayBuffers[ch][mWritePos[ch]] = channelData[ch][s];  // write to W
mWritePos[ch] = (mWritePos[ch] + 1) % bufSize;           // advance to W+1

// ...later in the same iteration:
const int readPos = (mWritePos[ch] - lookahead + bufSize) % bufSize;
// readPos = (W+1 - N + bufSize) % bufSize = W+1-N
```

`W+1-N` is the position of the sample written `N-1` steps ago (not `N` steps ago). So the actual delay is `lookaheadSamples - 1` samples.

`LimiterEngine::getLatencySamples()` (line 483) computes:
```cpp
const int lookaheadSamples = static_cast<int>(lookaheadMs * 0.001 * mSampleRate);
```
and returns this directly — always 1 sample too high.

**Correct fix:** Change the read position to include the current-step sample in the delay:

```cpp
// Option A: write AFTER computing readPos (read before write, then write)
const int readPos = (mWritePos[ch] + bufSize - lookahead) % bufSize;
delayed = mDelayBuffers[ch][readPos];
// Then write the current input (shifted to after output computation)
mDelayBuffers[ch][mWritePos[ch]] = channelData[ch][s];
mWritePos[ch] = (mWritePos[ch] + 1) % bufSize;
```

Or equivalently, keep write-then-advance but correct the read offset to `lookahead - 1`:

```cpp
// Option B: adjust read offset to match actual delay
const int readPos = (mWritePos[ch] - (lookahead - 1) + bufSize) % bufSize;
```

Option A (write after read) is preferred because it makes the data flow explicit: output the delayed sample, then record the new input. The detection scan also needs adjustment to still look ahead from the *just-written* position.

After fixing the read position, also update `getLatencySamples()` if needed — the formula `lookaheadMs * 0.001 * mSampleRate` should then match the actual delay.

Note: this also applies to the sidechain delay buffer (`mSidechainWritePos`), which uses the same write-then-advance pattern for the sidechain detection buffer.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — fix read position to match actual delay (lines 181-182, 222-223, 292-293)
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — `getLatencySamples()` at line 479; verify formula still correct after fix
Read: `M-LIM/src/PluginProcessor.cpp` — `setLatencySamples()` call at line 277

## Acceptance Criteria
- [ ] Run: `python3 -c "
# Simulate the corrected delay buffer read/write to verify 0-indexed delay is exactly N samples
N = 5; bufSize = 20; buf = [0.0]*bufSize; wp = 0
for s in range(20):
    # read before write
    rp = (wp + bufSize - N) % bufSize
    delayed = buf[rp]
    buf[wp] = float(s)
    wp = (wp + 1) % bufSize
    if s >= N:
        expected = float(s - N)
        assert delayed == expected, f's={s} delayed={delayed} expected={expected}'
print('PASS')
"` → Expected: `PASS`
- [ ] Run: `cd /workspace && grep -n "readPos\|mWritePos.*lookahead" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: the read position formula uses `lookahead` offset from the position BEFORE the write (not after)
- [ ] Run: `cd /workspace/build && ctest --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_lookahead_delay_exact` — with lookahead=N, feed a known signal, confirm output is delayed by exactly N samples (not N-1) and that `getLatencySamples()` matches the actual measured delay

## Technical Details

The read-before-write ordering:
```cpp
// Step 1: Read the delayed output (before overwriting the position)
const int readPos = (mWritePos[ch] + bufSize - lookahead) % bufSize;
float delayed = mDelayBuffers[ch][readPos];

// Step 2: Apply gain and write output
float out = delayed * mGainState[ch];
out = softSaturate(out, mParams.saturationAmount);
channelData[ch][s] = out;

// Step 3: Write new input AFTER reading output
mDelayBuffers[ch][mWritePos[ch]] = channelData[ch][s_input];  // save original input
mWritePos[ch] = (mWritePos[ch] + 1) % bufSize;
```

The detection scan still needs to cover the full lookahead window. With write-after-read, the most-recently-written sample is at `mWritePos[ch] - 1` (same as before, since the write hasn't happened yet at scan time). The scan must continue to include the current input sample (from `channelData[ch][s]`).

## Dependencies
None
