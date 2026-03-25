# Task 175: Replace memcpy-based Float Equality Check in SidechainFilter

## Description
`SidechainFilter.cpp` contains a helper used to avoid redundant coefficient recalculations:

```cpp
// SidechainFilter.cpp lines 82-89
static bool floatBitsEqual (float a, float b) noexcept
{
    uint32_t ia, ib;
    std::memcpy (&ia, &a, sizeof(ia));
    std::memcpy (&ib, &b, sizeof(ib));
    return ia == ib;
}
```

The comment says it "avoids -Wfloat-equal while detecting unchanged values." Type-punning via `memcpy` is well-defined in C++17, so this is not undefined behaviour, but it is an unnecessarily elaborate workaround.

The correct modern C++ approach is `std::bit_cast<uint32_t>(a) == std::bit_cast<uint32_t>(b)` (C++20), or — since the project uses C++17 — simply suppressing the warning for this comparison with `#pragma GCC diagnostic` or using a direct `==` comparison with a brief comment explaining intent.

Given that the purpose is just "is this the same IEEE 754 bit pattern we already applied?" (i.e. exact bit equality), the simplest and most readable fix for C++17 is:
```cpp
// Exact bit equality — intentional, avoids recomputing coefficients for the same value.
static bool floatBitsEqual (float a, float b) noexcept
{
    return std::memcmp (&a, &b, sizeof(float)) == 0;
}
```
Or, if the codebase moves to C++20 standard, replace with `std::bit_cast`.

Replace the current implementation with `std::memcmp` which is equally well-defined, shorter, and clearer in intent.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/SidechainFilter.cpp` — replace the `floatBitsEqual` body

## Acceptance Criteria
- [ ] Run: `grep -n "uint32_t ia\|uint32_t ib" M-LIM/src/dsp/SidechainFilter.cpp` → Expected: no matches
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure -R test_sidechain_filter 2>&1 | tail -10` → Expected: all tests pass

## Tests
None

## Technical Details
Replace the body of `floatBitsEqual` with:
```cpp
static bool floatBitsEqual (float a, float b) noexcept
{
    // Exact bit-pattern equality — intentional, detects unchanged parameter values
    // to avoid redundant coefficient recomputation without triggering -Wfloat-equal.
    return std::memcmp (&a, &b, sizeof(float)) == 0;
}
```

No call sites change. `<cstring>` is already included transitively; verify the include is present in `SidechainFilter.cpp` or add `#include <cstring>` explicitly.

## Dependencies
None
