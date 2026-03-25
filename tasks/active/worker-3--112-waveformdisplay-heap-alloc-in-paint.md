# Task 112: WaveformDisplay::drawPeakMarkers() Allocates std::vector Inside paint()

## Description
`WaveformDisplay::drawPeakMarkers()` (`WaveformDisplay.cpp` line 414) allocates a
`std::vector<float>` on every repaint (60 fps):

```cpp
std::vector<float> gr (static_cast<std::size_t> (total));
```

`drawPeakMarkers()` is called from `paint()`.  Heap allocations inside `paint()`
cause micro-stutters under memory pressure, trigger jemalloc/tcmalloc
fragmentation, and make the message thread unpredictable.

The vector's capacity is `history_.capacity()` which is a fixed compile-time
constant (`kHistorySize` defined in the header).  Replace the `std::vector` with
a `std::array<float, WaveformDisplay::kHistorySize>` member variable that is
reused every frame.

Steps:
1. Add `std::array<float, kHistorySize> mGrScratch_ {};` as a `mutable` private
   member in `WaveformDisplay.h`.
2. In `drawPeakMarkers()`, remove the local `std::vector<float> gr (...)` and
   replace with `auto& gr = mGrScratch_;` (then only write into indices
   0..total-1 as before).
3. Verify `kHistorySize` is accessible (`public static constexpr`) in the header
   so the `std::array` template argument resolves without a magic number.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/WaveformDisplay.h` — add mGrScratch_ member, confirm kHistorySize is accessible
Modify: `M-LIM/src/ui/WaveformDisplay.cpp` — replace local vector with member array reference

## Acceptance Criteria
- [ ] Run: `grep -n "std::vector" M-LIM/src/ui/WaveformDisplay.cpp` → Expected: no matches (vector removed)
- [ ] Run: `grep -n "mGrScratch_" M-LIM/src/ui/WaveformDisplay.h` → Expected: line declaring the member
- [ ] Run: `cd M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds
- [ ] Run: `cd M-LIM && cd build && ctest --output-on-failure 2>&1 | tail -5` → Expected: all tests pass

## Tests
None — no new logic; existing tests validate correctness.

## Technical Details
- Mark the member `mutable` so `drawPeakMarkers()` can write to it from a
  `const`-qualified paint path if needed.
- The scratch array does not need to be zeroed before use; only indices 0..total-1
  are read after writing.

## Dependencies
None
