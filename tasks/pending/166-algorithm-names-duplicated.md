# Task 166: Remove Duplicate Algorithm Name Strings

## Description
The algorithm names array is defined in two separate places:
- `AlgorithmSelector.cpp` lines 4–13: `ALGORITHM_NAMES[]` static C-string array
- `Parameters.cpp` line 30: `StringArray { "Transparent", "Punchy", ... }` literal inline

If an algorithm is added, renamed, or reordered, both must be updated in sync. They are currently identical, so any mismatch would cause silent bugs (UI shows wrong name for a given index, or APVTS serialises an old name that no longer maps correctly).

The fix: define the canonical list once in `LimiterAlgorithm.h` (already owned by the DSP layer and included everywhere), and have both consumers pull from it.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LimiterAlgorithm.h` — add a `constexpr` array of algorithm names here
Modify: `M-LIM/src/ui/AlgorithmSelector.cpp` — replace private array with reference to the shared constant
Modify: `M-LIM/src/ui/AlgorithmSelector.h` — remove `ALGORITHM_NAMES` and `NUM_ALGORITHMS` declarations
Modify: `M-LIM/src/Parameters.cpp` — replace inline `StringArray` literal with values drawn from the shared constant
Skip: `M-LIM/tests/` — no new tests required; existing build and parameter tests provide coverage

## Acceptance Criteria
- [ ] Run: `grep -rn "Transparent" M-LIM/src/` → Expected: exactly one definition site (in `LimiterAlgorithm.h`); all other occurrences are references to it, not string literals
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure -R test_limiter_algorithm 2>&1 | tail -10` → Expected: all tests pass

## Tests
None

## Technical Details
Add to `LimiterAlgorithm.h` inside the existing file (after the enum):
```cpp
static constexpr int kNumAlgorithms = 8;
static constexpr const char* kAlgorithmNames[kNumAlgorithms] = {
    "Transparent", "Punchy", "Dynamic", "Aggressive",
    "Allround", "Bus", "Safe", "Modern"
};
```

In `AlgorithmSelector.h`, remove:
```cpp
static const char* const ALGORITHM_NAMES[NUM_ALGORITHMS];
static constexpr int NUM_ALGORITHMS = 8;
```
and replace usages in `AlgorithmSelector.cpp` with `kAlgorithmNames` / `kNumAlgorithms`.

In `Parameters.cpp`, replace:
```cpp
StringArray { "Transparent", "Punchy", "Dynamic", "Aggressive", "Allround", "Bus", "Safe", "Modern" }
```
with a loop or `StringArray(kAlgorithmNames, kNumAlgorithms)`.

## Dependencies
None
