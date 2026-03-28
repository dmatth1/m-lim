# Task: Consolidate setIfChanged() overloads into a template

## Description
`LimiterEngine.cpp` (lines 190-209) has three near-identical `setIfChanged()` overloads for `float`, `int`, and `bool`. These should be a single template function to eliminate repetition.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — replace 3 overloads with 1 template
Modify: `M-LIM/src/dsp/LimiterEngine.h` — declare template in private section

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: `grep -c "void.*setIfChanged" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: 1 (was 3)

## Tests
None (refactor, no behavior change)

## Technical Details
Replace the three overloads with:
```cpp
template<typename T>
void setIfChanged(std::atomic<T>& param, T newValue) {
    if (param.load(std::memory_order_relaxed) != newValue) {
        param.store(newValue, std::memory_order_relaxed);
        mParamsDirty.store(true, std::memory_order_relaxed);
    }
}
```
For the float overload, the existing exact-comparison semantics are fine (these are parameter values, not computed results).

## Dependencies
None
