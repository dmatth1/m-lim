# Task 089: Remove Unused mSidechainData Allocation in LimiterEngine

## Description
`LimiterEngine` allocates `mSidechainData` (a `std::vector<std::vector<float>>`)
in `prepare()` but never reads or writes it anywhere in the class. The actual
sidechain working buffer used in `process()` is `mSidechainBuffer` (a
`juce::AudioBuffer<float>`), which was introduced when the sidechain code was
rewritten to fix the OOB oversampling bug (task 067).

`mSidechainData` is a dead allocation that wastes memory (numChannels ×
maxBlockSize × 4 bytes per prepare call) and causes misleading confusion
for anyone reading the code (two "sidechain buffer" variables, only one used).

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.h` — remove `mSidechainData` member
  declaration (around line 165)
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — remove the
  `mSidechainData.assign(...)` line in `prepare()` (around line 86)

## Acceptance Criteria
- [ ] Run: `grep -n "mSidechainData" M-LIM/src/dsp/LimiterEngine.h M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no output (member removed entirely)
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: builds without errors.

## Tests
None — removal of unused code; existing tests verify behavior is unchanged.

## Technical Details
- `mSidechainData` declared in `LimiterEngine.h` around line 165 with comment
  `// per-channel sidechain copy`
- Allocated in `LimiterEngine::prepare()` around line 86:
  `mSidechainData.assign(numChannels, std::vector<float>(maxBlockSize, 0.0f));`
- `mSidechainBuffer` (juce::AudioBuffer<float>) is the actual sidechain buffer,
  already properly pre-allocated; do not remove it.

## Dependencies
Requires task 067 (already done — introduced mSidechainBuffer, leaving
mSidechainData as dead code)
