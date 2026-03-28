# Task 511: mSidechainDelayBuffers in TransientLimiter

## Description
`TransientLimiter` allocates and writes to `mSidechainDelayBuffers` and `mSidechainWritePos`
but **never reads from them**. They are pure dead code.

Evidence:
- **Allocated** in `prepare()` (TransientLimiter.cpp:35): `mSidechainDelayBuffers.assign(...)`
- **Written** in `process()` (TransientLimiter.cpp:540-541): sidechain samples stored per-sample
- **Never read**: grep of entire `src/` directory shows no read path

The sidechain detection path correctly uses `mSCDeques` (the sliding-window monotone deque) to
track the maximum absolute value of the sidechain signal in the lookahead window. There is no
need to delay the sidechain through a separate delay buffer — the point of the sidechain is to
detect peaks `lookahead` samples before they emerge from the main delay buffer, which is
exactly what the deque-based detection achieves.

These dead buffers waste memory proportional to `numChannels * (maxLookaheadSamples + 1)`:
at 32x oversampling (mSampleRate ≈ 1.41 MHz), `maxLookaheadSamples ≈ 7050`, so each
channel holds 7051 floats (~28 KB) — unused.

**Fix**: Remove:
1. `mSidechainDelayBuffers` member (TransientLimiter.h:113)
2. `mSidechainWritePos` member (TransientLimiter.h — adjacent field)
3. Allocation in `prepare()` (TransientLimiter.cpp:35-36)
4. Write code in `process()` (TransientLimiter.cpp:537-542)
5. Any use in `resetCounters()` if present

## Produces
None

## Consumes
None

## Relevant Files
Modify: `src/dsp/TransientLimiter.cpp` — remove allocation in `prepare()` and write in `process()`
Modify: `src/dsp/TransientLimiter.h` — remove `mSidechainDelayBuffers` and `mSidechainWritePos` members

## Acceptance Criteria
- [ ] Run: `grep -n "mSidechainDelayBuffers\|mSidechainWritePos" src/dsp/TransientLimiter.h src/dsp/TransientLimiter.cpp` → Expected: no matches
- [ ] Run: `cd build && cmake --build . --target MLIM_Standalone_Standalone -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure 2>&1 | tail -10` → Expected: all tests pass (no regressions)

## Tests
None (dead code removal — no new logic, existing tests verify sidechain detection still works correctly)

## Technical Details
- `mSCDeques` (the sliding-window monotone deques for sidechain peak tracking) remain intact — they are the real sidechain detection mechanism
- `mSidechainWritePos` is only used to index into `mSidechainDelayBuffers`, so both can be removed together
- Double-check `resetCounters()` in TransientLimiter.cpp for any `mSidechainWritePos` reset that should also be removed

## Dependencies
None
