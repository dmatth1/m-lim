# Task 202: Document Algorithm Parameter Tuples in LimiterAlgorithm.h

## Description
`LimiterAlgorithm.h` defines 8 algorithm presets as raw numeric tuples, for example:

```cpp
{ 0.3f, 0.5f, 0.0f, 0.0f, 6.0f, true }   // "Transparent"
{ 1.5f, 2.0f, 0.2f, 0.1f, 3.0f, false }  // "Aggressive"
```

There are no comments explaining what each field position means or why each algorithm uses its particular values. A developer reading this code has no way to know that field 0 is "attack factor", field 4 is "soft knee width", etc., without tracing the struct definition.

Add:
1. A comment above the `AlgorithmParams` struct (or at its field declarations) naming each field.
2. A short one-line comment on each of the 8 algorithm entries explaining its sonic character.

This is a documentation-only change — no logic changes.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LimiterAlgorithm.h` — read the full file before making changes
Modify: `M-LIM/src/dsp/LimiterAlgorithm.h` — add field documentation and algorithm comments

## Acceptance Criteria
- [ ] Run: `grep -c "//" M-LIM/src/dsp/LimiterAlgorithm.h` → Expected: count increases by at least 9 (8 algorithm lines + struct field comments)
- [ ] Run: `cmake --build build -j4 2>&1 | tail -5` → Expected: build succeeds, exit 0

## Tests
None

## Technical Details
Document each field of `AlgorithmParams` inline, for example:
```cpp
struct AlgorithmParams {
    float attackFactor;    // Multiplier applied to the base attack time
    float releaseFactor;   // Multiplier applied to the base release time
    float saturation;      // Drive amount for soft-saturation stage (0 = off)
    float transientBoost;  // Pre-emphasis for transient detection (0 = flat)
    float kneeWidthDb;     // Soft knee width in dB
    bool  truePeak;        // Enable ITU-R BS.1770-4 true peak detection
};
```
(Adjust field names/descriptions to match the actual struct layout.)

For algorithm entries, add brief tonal descriptions, e.g.:
```cpp
// Transparent: minimal coloration, gentle knee, true peak enabled
{ 0.3f, 0.5f, 0.0f, 0.0f, 6.0f, true },
```

## Dependencies
None
