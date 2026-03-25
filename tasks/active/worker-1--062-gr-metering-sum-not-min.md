# Task 062: LimiterEngine — Total Gain Reduction Should Sum Both Stages, Not Min

## Description
In `LimiterEngine::process()` at line 283:

```cpp
const float totalGR = std::min(grL, grS);  // both are 0 or negative dB
```

This takes whichever single stage had the most reduction, ignoring the other stage's contribution. Since both stages operate in series (Stage 1 output feeds Stage 2), the total gain reduction applied to the signal is the **sum** of both stages in dB.

Example: if TransientLimiter reduces by -3 dB and LevelingLimiter reduces by -2 dB, the total reduction is -5 dB. Using `std::min` reports -3 dB (underreporting by 2 dB).

This affects the gain reduction meter display, the GR trace in the waveform display, and the MeterData FIFO — all will show less gain reduction than is actually occurring.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — change GR combination from min to sum

## Acceptance Criteria
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: `grep -n "std::min.*grL.*grS" M-LIM/src/dsp/LimiterEngine.cpp` → Expected: no output (min replaced with addition)

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_total_gr_is_sum_of_stages` — configure engine so both stages are actively reducing, verify `getGainReduction()` returns the sum of both stage reductions

## Technical Details
Replace line 283 in LimiterEngine.cpp:

```cpp
// BEFORE:
const float totalGR = std::min(grL, grS);

// AFTER:
const float totalGR = grL + grS;  // both are 0 or negative dB; sum gives total reduction
```

Both `getGainReduction()` values are ≤ 0 dB, so the sum will always be ≤ 0 dB. Clamp to a reasonable floor (e.g., -60 dB) to prevent display issues with extreme values.

## Dependencies
None
