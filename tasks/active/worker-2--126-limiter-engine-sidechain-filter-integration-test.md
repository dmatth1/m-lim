# Task 126: LimiterEngine Sidechain Filter Integration Tests

## Description
`LimiterEngine` exposes `setSidechainHPFreq()`, `setSidechainLPFreq()`, and `setSidechainTilt()`
which configure the sidechain detection filter (applied to the copy of the input used for
peak detection). These methods have **no integration tests** in the engine context — the
`SidechainFilter` is only tested in isolation.

The sidechain filter matters for real-world use: a DJ might set a high sidechain HP to prevent
bass from triggering the limiter, or a mastering engineer might use tilt to de-emphasize highs
in detection. If the wiring between `LimiterEngine::setSidechainHPFreq()` and the actual
`SidechainFilter` instance is broken, no test would catch it.

Add integration tests to `test_limiter_engine.cpp` that verify:

1. **High sidechain HP suppresses low-frequency triggering**: Feed a low-frequency (e.g. 50 Hz)
   loud signal. With sidechain HP at 500 Hz, the detection path filters out the bass, so GR
   should be significantly less than with sidechain HP at 20 Hz.

2. **Sidechain tilt changes GR for high-frequency content**: Feed a broadband signal rich in
   high frequencies. With sidechain tilt boosting highs (+6 dB), GR should be greater than
   with flat tilt (0 dB), because the detection path sees more energy.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/dsp/LimiterEngine.h` — setSidechainHPFreq, setSidechainLPFreq, setSidechainTilt, getGainReduction
Read: `src/dsp/LimiterEngine.cpp` — how sidechain filter is applied to sidechainBuffer
Read: `src/dsp/SidechainFilter.h` — filter interface
Modify: `tests/dsp/test_limiter_engine.cpp` — add sidechain filter integration tests

## Acceptance Criteria
- [ ] Run: `cd /workspace/M-LIM/build && ctest -R "LimiterEngine" --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure 2>&1 | grep -E "passed|failed"` → Expected: no regressions

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_sidechain_hp_reduces_bass_triggering` — high HP cutoff reduces GR from low-freq signal vs. low HP cutoff
- Unit: `tests/dsp/test_limiter_engine.cpp::test_sidechain_tilt_affects_gr` — positive tilt increases GR for treble-heavy input vs. flat tilt

## Technical Details
Pattern for HP test:
```cpp
auto measureGR = [&](float hpFreq) -> float {
    LimiterEngine eng;
    eng.prepare(44100.0, 512, 2);
    eng.setOutputCeiling(0.0f);
    eng.setSidechainHPFreq(hpFreq);
    // Feed 20 blocks of a loud 50 Hz sine
    for (int i = 0; i < 20; ++i) {
        auto buf = make50HzSine(2.0f, 512);
        eng.process(buf);
    }
    return eng.getGainReduction();
};
float grLowHP  = measureGR(20.0f);   // bass passes sidechain detection
float grHighHP = measureGR(500.0f);  // bass filtered from detection
// With high HP, sidechain sees less energy → less GR
REQUIRE(grHighHP > grLowHP);         // closer to 0 dB = less reduction
```

## Dependencies
None
