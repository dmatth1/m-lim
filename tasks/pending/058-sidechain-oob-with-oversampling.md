# Task 058: LimiterEngine — Sidechain Buffer Out-of-Bounds Read When Oversampling Enabled

## Description
**Critical bug**: In `LimiterEngine::process()`, the sidechain buffer is created at the original sample rate (`numSamples` long), but is passed to `TransientLimiter::process()` and `LevelingLimiter::process()` which operate at the oversampled rate (`upSamples` = `numSamples * oversamplingFactor`). The limiters will read past the end of the sidechain buffer — undefined behavior that will crash or corrupt memory.

Code flow (LimiterEngine.cpp):
- Line 174: `juce::AudioBuffer<float> sideBuf(buffer)` — creates copy with `numSamples` samples
- Line 175: `mSidechainFilter.process(sideBuf)` — filters at original rate
- Lines 178-180: `sidePtrs` points to sideBuf channels (length `numSamples`)
- Line 185: `upBlock = mOversampler.upsample(buffer)` — upsamples main audio
- Line 186: `upSamples = upBlock.getNumSamples()` — e.g., `numSamples * 4` for 4x oversampling
- Line 196-197: `mTransientLimiter.process(upPtrs, upChannels, upSamples, sidePtrs)` — reads `upSamples` from sidechain buffer of length `numSamples` → **OOB read**

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — fix sidechain handling with oversampling
Read: `M-LIM/src/dsp/LimiterEngine.h` — check working buffer declarations
Read: `M-LIM/src/dsp/Oversampler.h` — upsample API

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: Verify no out-of-bounds access by code review: sidechain buffer length matches upSamples when passed to limiters

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_oversampling_with_sidechain` — process a buffer through LimiterEngine with oversampling factor > 0 and sidechain filter active, verify no crash and output is valid

## Technical Details
Two valid fix approaches:

**Option A (recommended)**: Upsample the sidechain data too. Create a second Oversampler instance for the sidechain path, or copy the sidechain into the main buffer before upsampling, then separate after.

**Option B**: Don't pass sidechain to the oversampled limiters. Instead, run sidechain detection at the original sample rate in a separate pass, and pass `nullptr` for the sidechain parameter to the oversampled limiter calls. This is simpler but loses the benefit of oversampled sidechain detection.

**Option C**: If sidechain filter is flat (default settings), skip sidechain entirely and pass `nullptr`. Only create and pass sidechain when the user has actually adjusted sidechain filter settings.

For any option, the sidechain `sidePtrs` must NEVER be passed to a function that reads more samples than the buffer contains.

## Dependencies
None
