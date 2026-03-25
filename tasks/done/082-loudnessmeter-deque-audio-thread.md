# Task 082: LoudnessMeter Uses std::deque on the Audio Thread

## Description
`LoudnessMeter::onBlockComplete()` is called from `processBlock()` on the audio
thread every 100 ms. It calls `push_back()` and `pop_front()` on two
`std::deque<double>` members:

```cpp
mMomentaryBuffer.push_back(blockMeanSquare);
if (mMomentaryBuffer.size() > kMomentaryBlocks)
    mMomentaryBuffer.pop_front();

mShortTermBuffer.push_back(blockMeanSquare);
if (mShortTermBuffer.size() > kShortTermBlocks)
    mShortTermBuffer.pop_front();
```

`std::deque` is NOT real-time safe: its memory is segmented into fixed-size
chunks. When the deque grows from 0 to its capacity it allocates heap chunks.
The class comment claims no heap allocations in `processBlock()`, but these
deques call `malloc` on the first ~4 pushes (Momentary) and ~30 pushes (Short-term)
after `prepare()`. After that the deques are bounded and no further allocations
occur — but the allocations during warm-up happen on the audio thread.

Additionally, `pop_front()` on `std::deque` is O(1) amortized but may involve
a `free()` call (to release an exhausted chunk), which can block for arbitrary
time in a fragmented heap.

**Fix**: Replace both deques with plain `std::array` ring buffers of fixed size
(`kMomentaryBlocks = 4` and `kShortTermBlocks = 30`). No dynamic allocation,
no pointer indirection, cache-friendly access.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.h` — replace `std::deque<double>` members
  with `std::array<double, kMomentaryBlocks>` and `std::array<double, kShortTermBlocks>`
  plus head/size integers.
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — update `onBlockComplete()` to use
  ring buffer push/pop; update the `prepare()` clear/reset logic; update the
  summing loops in `onBlockComplete()` to iterate the ring buffer.
Read: `M-LIM/tests/dsp/test_loudness_meter.cpp` — existing tests must pass.
Read: `M-LIM/tests/dsp/test_loudness_meter_accuracy.cpp` — existing accuracy tests must pass.

## Acceptance Criteria
- [ ] Run: `grep -n "std::deque" M-LIM/src/dsp/LoudnessMeter.h` → Expected: no output (deques removed).
- [ ] Run: `cd M-LIM && ctest --test-dir build -R "test_loudness_meter" --output-on-failure` → Expected: all tests pass.

## Tests
- Unit: `M-LIM/tests/dsp/test_loudness_meter.cpp` — all existing tests pass.
- Unit: `M-LIM/tests/dsp/test_loudness_meter_accuracy.cpp` — all existing accuracy tests pass.

## Technical Details
Ring buffer replacement pattern for `mMomentaryBuffer` (capacity K=4):

```cpp
std::array<double, kMomentaryBlocks> mMomentaryRing{};
int mMomentaryHead = 0;  // index of oldest element
int mMomentaryCount = 0; // current fill (0..kMomentaryBlocks)

// push:
const int writeIdx = (mMomentaryHead + mMomentaryCount) % kMomentaryBlocks;
mMomentaryRing[writeIdx] = value;
if (mMomentaryCount < kMomentaryBlocks)
    ++mMomentaryCount;
else
    mMomentaryHead = (mMomentaryHead + 1) % kMomentaryBlocks;

// sum:
double sum = 0.0;
for (int i = 0; i < mMomentaryCount; ++i)
    sum += mMomentaryRing[(mMomentaryHead + i) % kMomentaryBlocks];
```

Apply the same pattern to `mShortTermBuffer` with `kShortTermBlocks = 30`.

## Dependencies
Requires task 068 (already done — established bounded history; this task fixes
remaining deque usage that task 068 did not address)
