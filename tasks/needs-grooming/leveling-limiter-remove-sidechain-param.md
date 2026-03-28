# Task: Structurally Enforce Stage 2 Detection Source in `LevelingLimiter`

## Description

`LevelingLimiter::process()` accepts a `sidechainData` pointer (same signature as `TransientLimiter::process()`), but passing anything other than `nullptr` causes over-limiting. This was a real bug: when `mSidePtrs` was passed here, Stage 2 detected on the pre-Stage-1 peak and applied redundant GR even after Stage 1 had already handled the transient.

The fix at `LimiterEngine.cpp:324` passes `nullptr` and documents it with a comment:

```cpp
// Stage 2 detects on the post-Stage-1 main audio (nullptr = use channelData).
// Passing mSidePtrs here caused over-limiting: Stage 2 would see the
// pre-Stage-1 peak and apply additional GR even when Stage 1 already handled it.
mLevelingLimiter.process(mUpPtrs.data(), upChannels, upSamples, nullptr);
```

This is a comment-only guard. Nothing in the API prevents a future developer from passing `mSidePtrs` — re-introducing the bug. The correct fix is to make the right behaviour the only expressible behaviour.

**Fix:** Remove the `sidechainData` parameter from `LevelingLimiter::process()` entirely. The leveling stage always detects on the main audio it is processing (post-Stage-1), so the parameter has no valid non-null use case. Update `LimiterEngine::stepRunLimiters()` to call `mLevelingLimiter.process(mUpPtrs.data(), upChannels, upSamples)`.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LevelingLimiter.h` — remove `sidechainData` parameter from `process()` declaration
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — remove `sidechainData` parameter from `process()` definition; if the implementation internally uses it (it likely aliases it to channelData when null), simplify to always use `channelData`
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — line 324: update call from `mLevelingLimiter.process(mUpPtrs.data(), upChannels, upSamples, nullptr)` to `mLevelingLimiter.process(mUpPtrs.data(), upChannels, upSamples)`
Read: `M-LIM/tests/dsp/test_leveling_limiter.cpp` — update any test calls that pass a sidechain argument

## Acceptance Criteria
- [ ] Run: `grep -n "sidechainData\|sidePtrs\|nullptr" M-LIM/src/dsp/LevelingLimiter.h M-LIM/src/dsp/LevelingLimiter.cpp` → Expected: no sidechain parameter in the process() signature
- [ ] Run: `cd /workspace/M-LIM/build && cmake --build . --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -5` → Expected: exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure -R "leveling"` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_leveling_limiter.cpp` — update existing tests that pass a sidechain pointer to remove the argument; verify no behaviour change (since the old nullptr path should be equivalent to the new parameterless path)

## Technical Details
Check `LevelingLimiter::process()` in the .cpp: it likely resolves `sidechain` as `sidechain ? sidechain : channelData`. After removing the parameter, replace that resolution with just `channelData`.

The corresponding `TransientLimiter::process()` retains its `sidechainData` parameter because Stage 1 legitimately uses the sidechain filter output for detection.

## Dependencies
None
