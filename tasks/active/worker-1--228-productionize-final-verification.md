# Task 228: Final Productionize Verification

## Description
Run a full end-to-end verification of the productionized M-LIM plugin. This task confirms all three work areas are complete:

1. **FontOptions fix**: Verify zero FontOptions occurrences in the entire codebase
2. **Build**: Full clean build succeeds for all configured formats (VST3, Standalone on Linux; VST3+AU on macOS)
3. **Tests**: Full test suite passes with zero failures
4. **Plugin artifacts**: VST3 binary is present and non-empty
5. **README**: User-facing README exists with all required sections
6. **DSP docs**: Key DSP headers have Doxygen-style documentation

If any step fails, diagnose and fix the root cause directly (do not just rerun).

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/ui/ControlStrip.cpp` — verify FontOptions fixed
Read: `README.md` — verify exists and complete
Read: `src/dsp/LoudnessMeter.h` — verify docs added
Skip: `libs/` — JUCE/CLAP submodules, not modified

## Acceptance Criteria
- [ ] Run: `grep -r "FontOptions" /workspace/M-LIM/src/ --include="*.h" --include="*.cpp" | wc -l` → Expected: `0`
- [ ] Run: `export CCACHE_DIR=/build-cache && cmake -B /workspace/M-LIM/build -S /workspace/M-LIM -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache 2>&1 | tail -3 && cmake --build /workspace/M-LIM/build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure -j$(nproc) 2>&1 | tail -10` → Expected: `100% tests passed`
- [ ] Run: `ls -la /workspace/M-LIM/build/MLIM_artefacts/VST3/` → Expected: `M-LIM.vst3` directory present
- [ ] Run: `test -f /workspace/M-LIM/README.md && echo ok` → Expected: `ok`

## Tests
E2E: Build pipeline produces valid plugin artifact
E2E: All Catch2 tests pass

## Technical Details
- Build command: `export CCACHE_DIR=/build-cache && cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache && cmake --build build -j$(nproc)`
- Tests: `cd build && ctest --output-on-failure`

## Dependencies
Requires tasks 223, 224, 225, 226, 227
