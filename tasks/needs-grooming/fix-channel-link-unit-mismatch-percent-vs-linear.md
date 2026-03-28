# Task: Fix Channel Link Parameter Unit Mismatch (0–100% vs 0–1)

## Description
The APVTS parameters `channelLinkTransients` and `channelLinkRelease` are defined in
`Parameters.cpp` as `NormalisableRange<float>(0.0f, 100.0f, 0.1f)` (percentage, 0–100).
However, `TransientLimiter::setChannelLink(float pct)` and
`LevelingLimiter::setChannelLink(float pct)` both clamp the value to `[0.0, 1.0]`:

```cpp
// TransientLimiter.cpp line 97
void TransientLimiter::setChannelLink(float pct)
{
    mChannelLink = std::clamp(pct, 0.0f, 1.0f);
}
```

`LevelingLimiter::setChannelLink` has identical clamping (line 85).

**Effect:** Any APVTS value > 1.0 (which is any setting above 1%) gets clamped to 1.0
(fully linked). The channel-link control is non-functional — it is always fully linked
regardless of user setting. Only values of exactly 0.0% produce independent-channel
behavior.

The data path is:
1. `pushAllParametersToEngine()` → `limiterEngine.setChannelLinkTransients(75.0f)` (raw %)
2. `LimiterEngine::setChannelLinkTransients` stores 75.0f into `mChannelLinkTransients`
3. `applyPendingParams()` → `mTransientLimiter.setChannelLink(mChannelLinkTransients.load())`
   → receives 75.0f, clamps to 1.0f — **wrong**

The fix is to divide by 100 in `LimiterEngine::applyPendingParams()` (and `prepare()`)
at the call sites where percent values are forwarded to the limiters.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/LimiterEngine.cpp` — divide `mChannelLinkTransients.load()` and
  `mChannelLinkRelease.load()` by 100.0f at the four call sites in `prepare()` (lines 44, 51)
  and `applyPendingParams()` (lines 171, 176)
Read: `src/dsp/TransientLimiter.h` — verify setChannelLink takes 0–1
Read: `src/dsp/LevelingLimiter.h` — verify setChannelLink takes 0–1
Read: `src/Parameters.cpp` — confirm channelLinkTransients/channelLinkRelease are 0–100 range
Read: `src/dsp/LimiterEngine.h` — mChannelLinkTransients / mChannelLinkRelease atomic defaults
  (currently `{ 1.0f }` which is inconsistent with the APVTS default of 75 / 100)

## Acceptance Criteria
- [ ] Run: `cd build && ctest -R "channel_link|channelLink|LevelingLimiter|TransientLimiter" --output-on-failure` → Expected: all matching tests pass
- [ ] Run: `cmake --build build --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds
- [ ] Manual verification: in a test, set channelLinkTransients = 50.0f (50%) and confirm
  `mTransientLimiter.mChannelLink == 0.5f` via a unit test

## Tests
- Unit: `tests/dsp/test_transient_limiter.cpp::test_channel_link_fifty_percent_partially_links` —
  set two channels with different peaks, set channelLink = 0.5, verify the applied gain is between
  the two independent gains (not the minimum as with full linking)
- Unit: `tests/dsp/test_leveling_limiter.cpp::test_channel_link_fifty_percent_partially_links` —
  same verification for LevelingLimiter
- Unit: `tests/dsp/test_limiter_engine.cpp::test_channel_link_transients_passes_fraction_to_limiter` —
  call `setChannelLinkTransients(50.0f)` on LimiterEngine, trigger `applyPendingParams()` via
  prepare/process, verify TransientLimiter received 0.5 (inspect via a getter or a test subclass)

## Technical Details
The fix in `LimiterEngine.cpp`:

```cpp
// In prepare() and applyPendingParams():
mTransientLimiter.setChannelLink(mChannelLinkTransients.load() * 0.01f);
mLevelingLimiter.setChannelLink(mChannelLinkRelease.load() * 0.01f);
```

Also update the atomic defaults in `LimiterEngine.h` to match the APVTS defaults:
```cpp
std::atomic<float> mChannelLinkTransients { 75.0f };  // was 1.0f
std::atomic<float> mChannelLinkRelease    { 100.0f }; // was 1.0f
```

Do NOT change the APVTS parameter range or the TransientLimiter/LevelingLimiter API —
the conversion belongs in LimiterEngine as the glue layer between the UI parameter scale
and the DSP implementation scale.

## Dependencies
None
