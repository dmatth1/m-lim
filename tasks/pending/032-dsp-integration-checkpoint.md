# Task 032: DSP Component Integration Checkpoint

## Description
Task 013 (LimiterEngine) consumes 7 interfaces (TransientLimiter, LevelingLimiter, Oversampler, TruePeakDetector, SidechainFilter, DCFilter, Dither). Per project conventions, when a task consumes 3+ interfaces it should have a preceding integration checkpoint to verify all components wire together before the orchestrator tries to use them.

This task verifies that all individual DSP components (tasks 004-012) compile together, their headers don't conflict, and their prepare/process interfaces are compatible. No new code — just build verification and a minimal wiring test.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/src/dsp/TransientLimiter.h` — verify interface
Read: `M-LIM/src/dsp/LevelingLimiter.h` — verify interface
Read: `M-LIM/src/dsp/Oversampler.h` — verify interface
Read: `M-LIM/src/dsp/TruePeakDetector.h` — verify interface
Read: `M-LIM/src/dsp/SidechainFilter.h` — verify interface
Read: `M-LIM/src/dsp/DCFilter.h` — verify interface
Read: `M-LIM/src/dsp/Dither.h` — verify interface
Read: `M-LIM/src/dsp/LimiterAlgorithm.h` — verify enum/struct
Create: `M-LIM/tests/integration/test_dsp_components.cpp` — compile-and-wire test

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_dsp_components --output-on-failure` → Expected: all tests pass
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -3` → Expected: builds with all DSP sources

## Tests
- Integration: `tests/integration/test_dsp_components.cpp::test_all_dsp_headers_compile` — include all DSP headers, verify no conflicts
- Integration: `tests/integration/test_dsp_components.cpp::test_all_dsp_prepare` — instantiate each component, call prepare(44100, 512, 2), verify no crash
- Integration: `tests/integration/test_dsp_components.cpp::test_dsp_chain_basic` — wire components in order (oversample→limit→filter→dither), process a test buffer

## Technical Details
- This is a verification-only task — fix compilation errors, not DSP bugs
- Verify all DSP components use consistent buffer types (float** vs AudioBuffer)
- Verify prepare() signatures are compatible with how LimiterEngine will call them
- If any component's interface doesn't match what LimiterEngine expects, note it for the 013 worker

## Dependencies
Requires tasks 004, 005, 006, 007, 008, 009, 010, 011, 012
