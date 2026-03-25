# Task 145: LimiterEngine — No Test That Bypass Mode Still Populates MeterFIFO

## Description
`LimiterEngine::setBypass(true)` passes audio through unchanged and skips all DSP
processing. However, the engine is supposed to still build and push `MeterData` to
`getMeterFIFO()` even in bypass mode — the UI needs to display input levels even when the
limiter is bypassed.

The existing `test_bypass_disables_all_processing` only checks that `getGainReduction()`
returns 0 dB. It does not verify that `getMeterFIFO()` receives any data, or that the
`inputLevelL`/`inputLevelR` fields are populated with the bypassed audio levels.

If the FIFO is not populated in bypass mode, the meters would freeze at their last values,
which is a visual regression.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/LimiterEngine.h` — getMeterFIFO(), bypass mode comment
Read: `M-LIM/src/dsp/LimiterEngine.cpp` — buildMeterData() call site; is it inside or outside bypass branch?
Read: `M-LIM/src/dsp/MeterData.h` — MeterData struct fields
Modify: `M-LIM/tests/dsp/test_limiter_engine_modes.cpp` — add bypass metering test

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R LimiterEngineModes --output-on-failure` → Expected: all tests pass, exit 0

## Tests
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_bypass_still_pushes_meter_data` —
  set bypass=true, process 5 blocks of a -6 dBFS sine, then pop from `getMeterFIFO()` and
  verify: (a) at least one MeterData item was pushed (pop returns true), (b)
  `inputLevelL > 0` (input is metered), (c) `gainReduction == 0.0f` (no GR applied).
- Unit: `tests/dsp/test_limiter_engine_modes.cpp::test_bypass_input_level_reflects_signal` —
  set bypass=true, process 10 blocks of a +6 dBFS signal (amplitude=2.0), drain the FIFO,
  verify the last-popped `inputLevelL` is approximately 2.0 (or within 10% of peak
  amplitude), confirming the meter tracks the bypassed audio.

## Technical Details
- The FIFO may only push one entry per block — drain all items between blocks if needed.
- Use `engine.getMeterFIFO().pop(meterData)` in a loop after processing.
- If the test finds the FIFO is empty in bypass mode, this is a bug: `buildMeterData()` or
  an equivalent call must be executed even in the bypass branch of `process()`. The worker
  should fix the bug if found.

## Dependencies
None
