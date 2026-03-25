# Task 076: LoudnessMeter — std::deque push_back/pop_front May Allocate on Audio Thread

## Description

`LoudnessMeter` uses `std::deque<double>` for `mMomentaryBuffer` (max 4 elements) and `mShortTermBuffer` (max 30 elements), declared in `LoudnessMeter.h` lines 99–100:

```cpp
std::deque<double> mMomentaryBuffer;  // up to 4 blocks
std::deque<double> mShortTermBuffer;  // up to 30 blocks
```

These deques are written in `onBlockComplete()` → called from `processBlock()` on the **audio thread**:

```cpp
mMomentaryBuffer.push_back(blockMeanSquare);
if (static_cast<int>(mMomentaryBuffer.size()) > kMomentaryBlocks)
    mMomentaryBuffer.pop_front();
```

`std::deque` allocates internal fixed-size chunks dynamically. Although in steady state (after initial fill) the deque stays at constant size, `push_back` can trigger chunk allocation if the back-of-deque chunk is full. `pop_front` may also call `delete` when it releases the front chunk. These allocations are implementation-defined and **not guaranteed to be allocation-free**, violating CLAUDE.md's rule: "No allocations or locks in processBlock."

**Fix:** Replace both deques with pre-allocated fixed-size circular arrays (ring buffers), allocated once in `prepare()` and reused without any heap activity on the audio thread.

The ring buffer needs:
- A `std::array<double, N>` or `std::vector<double>` (sized in `prepare()`)
- A write index and element count
- O(1) push (overwrite oldest) and O(1) sum (maintained incrementally)

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LoudnessMeter.h` — replace `std::deque` members with fixed-size ring buffer structs
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — update `prepare()`, `onBlockComplete()`, `updateIntegratedAndLRA()` to use ring buffers; maintain running sums to avoid O(N) sum recomputation
Read: `M-LIM/src/dsp/LoudnessMeter.h` — full class layout

## Acceptance Criteria
- [ ] Run: `cd /workspace && grep -n "std::deque" M-LIM/src/dsp/LoudnessMeter.h` → Expected: no output (no deques in header)
- [ ] Run: `cd /workspace && grep -n "push_back\|pop_front" M-LIM/src/dsp/LoudnessMeter.cpp` → Expected: no output from mMomentary/mShortTerm paths (ring buffer used instead)
- [ ] Run: `cd /workspace/build && ctest --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/dsp/test_loudness_meter.cpp::test_no_alloc_in_processblock` — use JUCE's `ScopedJuceInitialiser_NoGui` and an allocation-tracking allocator to confirm zero heap allocations during `processBlock()` after `prepare()`
- Unit: `tests/dsp/test_loudness_meter.cpp::test_momentary_lufs_ring_buffer` — verify momentary LUFS output is identical when using ring buffer vs. reference deque implementation

## Technical Details

Suggested ring buffer structure:

```cpp
template<int N>
struct FixedRingBuffer
{
    std::array<double, N> buf{};
    int    head  = 0;  // oldest element index
    int    count = 0;  // number of valid elements
    double runSum = 0.0;  // maintained incrementally

    void push(double val)
    {
        if (count == N)
        {
            runSum -= buf[head];        // remove oldest from sum
            buf[head] = val;
            head = (head + 1) % N;
        }
        else
        {
            buf[(head + count) % N] = val;
            ++count;
        }
        runSum += val;
    }

    double sum()  const { return runSum; }
    double mean() const { return count > 0 ? runSum / count : 0.0; }
    bool   full() const { return count == N; }
};

FixedRingBuffer<kMomentaryBlocks>  mMomentaryRing;
FixedRingBuffer<kShortTermBlocks>  mShortTermRing;
```

Maintaining `runSum` incrementally avoids the O(4) and O(30) sum loops in `onBlockComplete()`.

The `prepare()` method must call `reset()` on both rings (zero-fill buf, head=0, count=0, runSum=0.0).

## Dependencies
None
