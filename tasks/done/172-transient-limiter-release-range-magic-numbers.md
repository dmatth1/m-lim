# Task 172: Name Magic Numbers for TransientLimiter Release Range

## Description
`TransientLimiter::setAlgorithmParams()` maps the `releaseShape` parameter (0–1) to a release time in milliseconds using two unnamed magic numbers:

```cpp
// TransientLimiter.cpp line 89
const float releaseMs = 10.0f + params.releaseShape * 490.0f;
```

The comment above (line 87–88) explains the mapping but the numbers themselves — minimum release (10 ms) and the span (490 ms → maximum 500 ms) — are unnamed. The corresponding release range in `Parameters.cpp` uses different bounds (10–1000 ms for the user-facing release parameter) which makes the relationship even harder to understand.

Add named constants to `TransientLimiter.h` or in an anonymous namespace in `TransientLimiter.cpp` and replace the literal values.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — replace magic numbers with named constants
Read: `M-LIM/src/dsp/TransientLimiter.h` — see existing constant naming style

## Acceptance Criteria
- [ ] Run: `grep -n "10\.0f + params\.releaseShape \* 490\.0f" M-LIM/src/dsp/TransientLimiter.cpp` → Expected: no matches
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure -R test_transient_limiter 2>&1 | tail -10` → Expected: all tests pass

## Tests
None

## Technical Details
Add near the top of `TransientLimiter.cpp` (anonymous namespace or as `static constexpr` before the function):
```cpp
// Release time range for the shape-controlled release envelope.
// releaseShape 0 → kReleaseMinMs, releaseShape 1 → kReleaseMaxMs.
static constexpr float kReleaseMinMs  = 10.0f;
static constexpr float kReleaseMaxMs  = 500.0f;
```

Then replace:
```cpp
const float releaseMs = 10.0f + params.releaseShape * 490.0f;
```
with:
```cpp
const float releaseMs = kReleaseMinMs + params.releaseShape * (kReleaseMaxMs - kReleaseMinMs);
```

This makes the intent (lerp from min to max) explicit and avoids the confusing 490 span that conceals the 500 ms ceiling.

## Dependencies
None
