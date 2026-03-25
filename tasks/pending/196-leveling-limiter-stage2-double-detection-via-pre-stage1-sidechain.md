# Task 196: LevelingLimiter (Stage 2) Detects on Pre-Stage-1 Sidechain Causing Over-Limiting

## Description

In `LimiterEngine::stepRunLimiters()`, both the TransientLimiter (Stage 1) and the LevelingLimiter (Stage 2) receive the same pre-Stage-1 sidechain copy for peak detection:

**File:** `src/dsp/LimiterEngine.cpp`, `stepRunLimiters()`:
```cpp
// CURRENT (wrong for Stage 2):
mTransientLimiter.process(mUpPtrs.data(), upChannels, upSamples, mSidePtrs.data());
mLevelingLimiter.process(mUpPtrs.data(), upChannels, upSamples, mSidePtrs.data());
```

`mSidePtrs` is a filtered copy of the **original pre-limiting input** (the sidechain path). `mUpPtrs` is the main audio which, after Stage 1, contains the **already-limited output**.

**The bug:** Stage 2 detects a peak on the pre-Stage-1 sidechain. When Stage 1 has reduced a peak from, say, 1.2 to 1.0 (ceiling), Stage 2 still "sees" 1.2 and computes `requiredGain = 1.0/1.2 = 0.833`. It applies this −1.58 dB reduction to the main audio, which is already at 1.0 (no reduction needed). The result is the output drops to 0.833 — a full 1.58 dB below the ceiling when Stage 1 alone would have been sufficient. This causes:
- Excessive gain reduction on transients (the sound of over-limiting / "pumping")
- The two stages fighting each other on peaks that Stage 1 already handles
- More gain reduction than any single stage alone would produce

**The fix:** Stage 2 should detect on the post-Stage-1 main audio (pass `nullptr` as sidechain, so `LevelingLimiter` falls back to detecting on its own `channelData` — which is the already-limited audio):
```cpp
// CORRECT:
mTransientLimiter.process(mUpPtrs.data(), upChannels, upSamples, mSidePtrs.data());
mLevelingLimiter.process(mUpPtrs.data(), upChannels, upSamples, nullptr);
```

With `nullptr`, `LevelingLimiter::process()` detects peaks on `channelData` (the post-Stage-1 audio), which only triggers further GR if Stage 1 left residual peaks above the threshold. This matches the dual-stage limiter design intent: Stage 1 catches fast transients, Stage 2 handles residual slow-release peaks.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.cpp` — change `stepRunLimiters()` to pass `nullptr` sidechain to `mLevelingLimiter`
Read: `src/dsp/LevelingLimiter.h` — confirm `nullptr` sidechain causes detection on channelData
Read: `src/dsp/LevelingLimiter.cpp` — verify process() check: `const float* detectCh = (sidechainData != nullptr) ? sidechainData[ch] : channelData[ch];`
Read: `tests/dsp/test_limiter_engine.cpp` — update or add tests for cascaded stage behaviour

## Acceptance Criteria
- [ ] Run: `grep -A5 "stepRunLimiters" src/dsp/LimiterEngine.cpp` → Expected: `mLevelingLimiter.process(...)` is called with `nullptr` as the last argument
- [ ] Run: `cd build && ctest -R test_limiter_engine --output-on-failure` → Expected: all limiter engine tests pass
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: full test suite passes

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp` — add a test: feed a 1.2-amplitude sine wave through the engine with both stages active. After processing, verify the output peak does not drop more than ~0.5 dB below the output ceiling (i.e., Stage 2 is not over-limiting on peaks Stage 1 already handled). Compare against a baseline with Stage 2 bypassed.
- Unit: `tests/dsp/test_limiter_engine.cpp` — add a test: verify that a signal exactly at the ceiling (1.0 linear) passes through without Stage 2 applying any gain reduction (since Stage 1 passes it cleanly and Stage 2 detects peak ≤ threshold).

## Technical Details
The key code path in `LevelingLimiter::process()`:
```cpp
const float* detectCh = (sidechainData != nullptr)
                            ? sidechainData[ch]
                            : channelData[ch];
perChRequiredGain[ch] = computeRequiredGain(std::abs(detectCh[s]));
```

When `sidechainData == nullptr`, Stage 2 detects on `channelData` — which at that point in the DSP chain is the post-Stage-1 limited audio. Peaks above the threshold in this signal are residual peaks Stage 1 didn't fully catch (e.g., during release), and Stage 2 correctly cleans those up.

The `mSidePtrs` sidechain should continue to be used only by Stage 1 (TransientLimiter), which uses it for lookahead peak anticipation. Stage 2 has no lookahead and should react only to actual residuals in the limited signal.

## Dependencies
None
