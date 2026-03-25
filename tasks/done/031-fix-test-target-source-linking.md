# Task 031: Fix Test Target Source Linking

## Description
Task 002 creates the test infrastructure but only links against JUCE modules (`juce_core`, `juce_audio_basics`, `juce_dsp`). DSP unit tests (tasks 005-012) test classes like `DCFilter`, `Dither`, `TruePeakDetector` etc. — these tests need to compile or link against the actual plugin source files. Without this, every DSP test will fail to compile with undefined reference errors.

The fix: create a static library target (e.g., `MLIMLib`) containing all `src/` source files, and link both the plugin target and the test target against it. Alternatively, add plugin source files to the test target's `SOURCES`. The CMake setup in task 001 and/or task 002 must account for this.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/CMakeLists.txt` — add a shared source library target (`MLIMLib`) that both plugin and test targets link against
Modify: `M-LIM/tests/CMakeLists.txt` — link `MLIMTests` against `MLIMLib` (or add plugin sources to test target)
Read: `SPEC.md` — file structure for source files

## Acceptance Criteria
- [ ] Run: `grep -E "MLIMLib|target_sources.*MLIMTests" M-LIM/CMakeLists.txt M-LIM/tests/CMakeLists.txt` → Expected: test target has access to plugin source files
- [ ] Run: `cd M-LIM && cmake -B build && cmake --build build --target MLIMTests -j$(nproc) 2>&1 | tail -5` → Expected: test target builds successfully with DSP source files accessible

## Tests
None (build infrastructure fix)

## Technical Details
- In JUCE CMake projects, the common pattern is to collect source files into a variable or INTERFACE library, then use it in both the plugin target and the test target
- Option A: Create `MLIMLib` as a STATIC library with all src/**/*.cpp, link both plugin and tests against it
- Option B: Use `target_sources` to add plugin .cpp files to the test target
- Option A is cleaner — avoids double-compilation and ensures consistent compilation flags
- The library needs to link against the same JUCE modules the plugin uses
- Workers implementing tasks 005-012 will hit this issue immediately when their tests fail to link

## Dependencies
Requires task 001
