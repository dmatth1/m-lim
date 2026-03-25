# Task 065: Eliminate Audio-Thread Heap Allocations in LimiterEngine::process()

## Description
LimiterEngine::process() performs several heap allocations on the audio thread, violating real-time safety. These can cause priority inversion, page faults, and audio glitches in production DAW usage. The following allocations must be moved to prepare():

1. **Line 165**: `preLimitBuffer.makeCopyOf(buffer)` — allocates a new AudioBuffer every time delta mode is active
2. **Line 174**: `juce::AudioBuffer<float> sideBuf(buffer)` — copy-constructs an AudioBuffer (heap alloc for channel data)
3. **Line 178**: `std::vector<const float*> sidePtrs(numChannels)` — heap-allocated vector every call
4. **Line 189**: `std::vector<float*> upPtrs(upChannels)` — heap-allocated vector every call

## Produces
None

## Consumes
LimiterEngineInterface

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.h` — add pre-allocated member buffers
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — use pre-allocated members instead of local allocations
Read: `M-LIM/src/dsp/LimiterEngine.h` — current member layout

## Acceptance Criteria
- [ ] Run: `grep -n "std::vector" M-LIM/src/dsp/LimiterEngine.cpp | grep -v "^[0-9]*:.*mSidechainData"` → Expected: no vector construction inside process()
- [ ] Run: `grep -n "makeCopyOf\|AudioBuffer<float>.*(" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no buffer construction/copy inside process() — only in prepare()
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest --output-on-failure` → Expected: all tests pass

## Tests
None (existing tests cover correctness; this is a real-time safety fix that doesn't change behavior)

## Technical Details
**Fix approach:**
1. Add member variables to LimiterEngine:
   ```cpp
   juce::AudioBuffer<float> mPreLimitBuffer;   // for delta mode
   juce::AudioBuffer<float> mSidechainBuffer;  // sidechain copy
   std::vector<const float*> mSidePtrs;        // pointer array
   std::vector<float*> mUpPtrs;                // pointer array
   ```
2. In `prepare()`, pre-allocate these to maxBlockSize/maxChannels:
   ```cpp
   mPreLimitBuffer.setSize(numChannels, maxBlockSize);
   mSidechainBuffer.setSize(numChannels, maxBlockSize);
   mSidePtrs.resize(numChannels);
   mUpPtrs.resize(numChannels);
   ```
3. In `process()`, use the pre-allocated members:
   - For delta mode: copy buffer data into mPreLimitBuffer using `mPreLimitBuffer.makeCopyOf(buffer)` — but note this still allocates if size differs. Instead, use `juce::AudioBuffer::copyFrom()` into the pre-sized buffer.
   - For sidechain: copy channel data into mSidechainBuffer via `copyFrom()`.
   - For pointer arrays: just update the pre-allocated vectors' elements.

## Dependencies
Requires task 013
