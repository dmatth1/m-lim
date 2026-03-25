# Task 030: Final Testing and Verification

## Description
Run the complete test suite end-to-end, verify the plugin builds as VST3, verify all DSP and state tests pass, and confirm the project meets all success criteria from SPEC.md.

## Produces
None

## Consumes
None

## Relevant Files
Read: `M-LIM/CMakeLists.txt` — build config
Read: `M-LIM/tests/` — all test files
Read: `SPEC.md` — success criteria checklist

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc) 2>&1 | tail -10` → Expected: Release build completes with no errors
- [ ] Run: `cd M-LIM/build && ctest --output-on-failure 2>&1` → Expected: ALL tests pass (0 failures)
- [ ] Run: `ls -la M-LIM/build/MLIM_artefacts/Release/VST3/` → Expected: .vst3 bundle exists
- [ ] Run: `cd M-LIM && grep -r "class MLIMProcessor" src/PluginProcessor.h` → Expected: class defined
- [ ] Run: `cd M-LIM && grep -r "class MLIMEditor" src/PluginEditor.h` → Expected: class defined
- [ ] Run: `wc -l M-LIM/src/dsp/*.cpp M-LIM/src/ui/*.cpp M-LIM/src/state/*.cpp 2>/dev/null | tail -1` → Expected: substantial codebase (>2000 total lines)

## Tests
- E2E: All existing unit tests pass
- E2E: All existing integration tests pass
- E2E: Plugin binary exists and is valid

## Technical Details
- Build in Release mode for final verification
- Run full ctest suite — every single test must pass
- If any test fails, fix the issue (may involve modifying source or test code)
- Verify all 8 limiter algorithms are defined
- Verify all UI components compile
- Verify preset files exist and are valid XML
- Check for compiler warnings and fix any serious ones
- This is the gate — nothing ships until this task passes

## Dependencies
Requires tasks 026, 027, 028, 029
