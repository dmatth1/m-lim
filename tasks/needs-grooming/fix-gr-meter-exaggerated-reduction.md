# Task: Fix exaggerated gain reduction meter reporting

## Description
In `LimiterEngine::process()` (line 263), the combined gain reduction is computed as:
```cpp
const float combinedMinGain = mTransientLimiter.getMinGainLinear() * mLevelingLimiter.getMinGainLinear();
```
This multiplies the *per-stage minimum* gains, but those minimums may not occur at the same sample. Stage 1's minimum gain could be at sample 50 while Stage 2's minimum is at sample 200. Multiplying them produces a GR value more negative than what any individual sample actually experienced, causing the GR meter to show exaggerated gain reduction.

For example, if Stage 1's worst gain was 0.5 (-6 dB) at sample 50, and Stage 2's worst gain was 0.8 (-1.9 dB) at sample 200, the reported combined GR would be 0.4 (-8 dB), even though no sample actually experienced more than -6 dB of total reduction.

This only affects the GR meter display — the actual audio gain reduction is applied correctly per-sample by each stage independently.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — `process()` method, GR computation at line 263
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — would need to expose per-sample combined gain tracking
Modify: `M-LIM/src/dsp/TransientLimiter.h` — new method to retrieve per-sample gain array
Read: `M-LIM/src/dsp/LevelingLimiter.cpp` — same pattern for per-sample gain tracking

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass, exit 0
- [ ] Run: A test that feeds a block with a transient peak at sample 0 and verifies the reported GR does not exceed the actual per-sample combined GR maximum → Expected: reported GR within 0.5 dB of actual

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_gr_meter_accuracy` — verify that reported GR matches actual per-sample combined GR within 0.5 dB

## Technical Details
**Recommended fix approach**: Track the per-sample minimum combined gain within the LimiterEngine process loop rather than multiplying per-stage post-hoc minimums.

Option A (minimal change): After Stage 2 runs, scan the output buffer and compare to the pre-limit buffer to compute per-sample actual gain. This requires the pre-limit buffer to be captured before both stages.

Option B (more accurate): Have `LimiterEngine::stepRunLimiters()` return a per-sample gain array from Stage 1 (or expose `mGainState` per sample), then multiply by Stage 2's per-sample gain. This requires modifying both limiter stages to expose per-sample gain history.

Option C (simplest): Keep the current approach but document in the UI that GR meter shows conservative (worst-case) estimate. This is actually what many commercial limiters do.

This is low priority — the visual error is typically < 1 dB and the conservative bias is arguably desirable for a professional limiter (better to show "too much" GR than "too little").

## Dependencies
None
