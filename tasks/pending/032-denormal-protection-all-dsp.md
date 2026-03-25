# Task 032: Denormal Float Protection Across All DSP Modules

## Description
All DSP modules using IIR filters or exponential envelope smoothing must include denormal float protection. On x86/x64, denormalized floats (values near zero like 1e-38) cause the CPU to fall back to microcode, resulting in 10-100x slower processing — catastrophic for real-time audio. This is a well-known audio DSP issue.

Every `process()` / `processBlock()` method in the DSP chain must either:
1. Use `juce::ScopedNoDenormals` at the top of the function (sets FTZ/DAZ CPU flags), OR
2. Add a tiny DC bias (e.g., 1e-15f) to filter states after each sample, OR
3. Use `juce::snapToZero()` on filter state variables

Option 1 (`ScopedNoDenormals`) is preferred — it's a single line, JUCE handles it cross-platform, and it protects all arithmetic in scope.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/dsp/DCFilter.cpp` — add ScopedNoDenormals to process()
Modify: `M-LIM/src/dsp/SidechainFilter.cpp` — add ScopedNoDenormals to process()
Modify: `M-LIM/src/dsp/TransientLimiter.cpp` — add ScopedNoDenormals to process()
Modify: `M-LIM/src/dsp/LevelingLimiter.cpp` — add ScopedNoDenormals to process()
Modify: `M-LIM/src/dsp/LoudnessMeter.cpp` — add ScopedNoDenormals to processBlock()
Modify: `M-LIM/src/dsp/TruePeakDetector.cpp` — add ScopedNoDenormals to processBlock()
Modify: `M-LIM/src/dsp/Dither.cpp` — add ScopedNoDenormals to process() (noise shaping feedback)
Modify: `M-LIM/src/dsp/LimiterEngine.cpp` — add ScopedNoDenormals to process() as top-level guard

## Acceptance Criteria
- [ ] Run: `grep -r "ScopedNoDenormals" M-LIM/src/dsp/` → Expected: at least 5 matches across different files
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest --output-on-failure` → Expected: all tests still pass

## Tests
- Unit: `tests/dsp/test_limiter_engine.cpp::test_no_cpu_spike_on_silence` — process 10 seconds of silence, verify processing time per block stays consistent (no denormal slowdown)

## Technical Details
Usage pattern (one line at top of each process function):
```cpp
void DCFilter::process(float* data, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    // ... existing code
}
```

The `juce::ScopedNoDenormals` constructor sets the MXCSR register's FTZ (Flush To Zero) and DAZ (Denormals Are Zero) bits. The destructor restores the previous state. This is RAII and thread-safe.

Additionally, all `reset()` methods should zero filter states explicitly (not leave them at potentially denormal values).

## Dependencies
Requires tasks 005, 006, 007, 009, 010, 011, 012 (files must exist to modify)
