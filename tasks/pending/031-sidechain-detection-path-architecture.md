# Task 031: Sidechain Detection Path Architecture

## Description
The TransientLimiter and LevelingLimiter interfaces lack a separate sidechain/detection input. The sidechain filter is meant to shape the signal the limiter *responds to* (the detection path), not the audio being processed. Currently, both limiters only accept `process(float** channelData, ...)` which processes in-place — there's no way to feed a separately-filtered detection signal while applying gain reduction to the unfiltered audio.

The fix: add a sidechain overload or a separate detection buffer input to both TransientLimiter and LevelingLimiter. The LimiterEngine then passes the sidechain-filtered copy for peak detection while applying gain reduction to the original audio.

## Produces
None

## Consumes
TransientLimiterInterface
LevelingLimiterInterface
SidechainFilterInterface

## Relevant Files
Modify: `SPEC.md` — Update TransientLimiterInterface and LevelingLimiterInterface to add sidechain input parameter
Modify: `tasks/pending/011-transient-limiter.md` — Add sidechain detection buffer to interface and tests
Modify: `tasks/pending/012-leveling-limiter.md` — Add sidechain detection buffer to interface and tests
Modify: `tasks/pending/013-limiter-engine.md` — Update DSP chain description to clarify sidechain routing

## Acceptance Criteria
- [ ] Run: `grep -c "sidechain\|detection" SPEC.md` → Expected: at least 4 (interfaces updated)
- [ ] Run: `grep "const float\*\* sidechainData\|const float\* sidechain" SPEC.md` → Expected: matches in TransientLimiter and LevelingLimiter interfaces

## Tests
None (plan/architecture fix)

## Technical Details
Recommended interface change for both limiters:
```cpp
// Current (broken for sidechain use):
void process(float** channelData, int numChannels, int numSamples);

// Fixed — add optional sidechain input:
void process(float** channelData, int numChannels, int numSamples,
             const float* const* sidechainData = nullptr);
```
When `sidechainData` is non-null, use it for peak detection / envelope following. When null, detect on the audio data directly (no sidechain filter active).

The LimiterEngine should:
1. Copy the input buffer
2. Apply sidechain filter to the copy
3. Pass both buffers to the limiters: audio for processing, filtered copy for detection

## Dependencies
None (plan-level fix, should be done before tasks 011, 012, 013 are started)
