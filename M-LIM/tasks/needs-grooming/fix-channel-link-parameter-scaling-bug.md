# Task: Fix Channel Link Parameter Scaling Bug (0–100% → 0.0–1.0 Conversion Missing)

## Description
**This is a functional bug.** The channel link parameters (`channelLinkTransients` and
`channelLinkRelease`) have a 0–100% parameter range but are passed to the DSP engine unchanged,
while the DSP layer expects a 0.0–1.0 value. The result: the channel link control is
non-functional for any value above ~1% — everything above 1% is clamped to 100% linked.

**Trace of the bug:**

1. `Parameters.cpp` defines both parameters with `NormalisableRange<float>(0.0f, 100.0f, ...)` and
   defaults of `75.0f` (transients) and `100.0f` (release). The UI displays "%", confirming
   the intent is 0–100%.

2. `PluginProcessor.cpp::pushAllParametersToEngine()` (lines 272–273) passes the raw parameter
   value with **no normalisation**:
   ```cpp
   limiterEngine.setChannelLinkTransients(pChannelLinkTransients->load());  // 75.0, not 0.75
   limiterEngine.setChannelLinkRelease(pChannelLinkRelease->load());
   ```

3. `LimiterEngine` stores the value as-is and passes it through to:
   ```cpp
   mTransientLimiter.setChannelLink(mChannelLinkTransients.load());  // receives 75.0
   ```

4. `TransientLimiter::setChannelLink()` and `LevelingLimiter::setChannelLink()` both clamp:
   ```cpp
   mChannelLink = std::clamp(pct, 0.0f, 1.0f);  // clamps 75.0 → 1.0 (always fully linked)
   ```

**Effect:** Channel link has no gradation from 0% to 100%. Any setting above ~1% behaves
identically (fully linked). Only the extreme values (0% vs. >0%) have any effect.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginProcessor.cpp` — in `pushAllParametersToEngine()`, divide the channel
  link values by 100.0f before passing to the engine (lines 272–273)
Read: `M-LIM/src/dsp/LimiterEngine.h` — confirms `setChannelLinkTransients/Release` expect 0–1
Read: `M-LIM/src/dsp/TransientLimiter.cpp` — confirms `setChannelLink` clamps to [0, 1]
Read: `M-LIM/src/dsp/LevelingLimiter.cpp` — same for release channel link

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Manual verification: setting Channel Link (Transients) to 50% should produce intermediate
  stereo behaviour, not identical to 0% or 100%

## Tests
Add a unit test to `M-LIM/tests/integration/test_plugin_processor.cpp` (or equivalent) that:
- Sets `channelLinkTransients` parameter to 50.0 (50%)
- Calls `pushAllParametersToEngine()` (or uses the processor)
- Verifies that `limiterEngine.setChannelLinkTransients` was called with ~0.5, NOT 50.0
  (e.g. by verifying the GR behaviour with a test signal is intermediate between 0% and 100%)

## Technical Details
Fix in `pushAllParametersToEngine()`:
```cpp
// Before (broken):
if (pChannelLinkTransients) limiterEngine.setChannelLinkTransients(pChannelLinkTransients->load());
if (pChannelLinkRelease)    limiterEngine.setChannelLinkRelease   (pChannelLinkRelease->load());

// After (fixed):
if (pChannelLinkTransients) limiterEngine.setChannelLinkTransients(pChannelLinkTransients->load() * 0.01f);
if (pChannelLinkRelease)    limiterEngine.setChannelLinkRelease   (pChannelLinkRelease->load() * 0.01f);
```

Do NOT change the parameter range in `Parameters.cpp` (it is correct — 0–100% is the right UX).
Do NOT change `LimiterEngine::setChannelLinkTransients/Release` or the TransientLimiter/LevelingLimiter
setters — the convention that the engine expects 0–1 is correct and consistent.

Note: the default value of `channelLinkTransients` is `75.0f` in Parameters.cpp. After the fix,
75.0f * 0.01f = 0.75 which is 75% link — the intended default.

## Dependencies
None
