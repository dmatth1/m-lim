# Task 002: Test Infrastructure

## Description
Set up Catch2 v3 test framework for M-LIM. Create the test directory structure, download Catch2 single-header, create test CMakeLists.txt that links against JUCE modules, and write a test_main.cpp with a basic sanity test.

## Produces
None

## Consumes
None

## Relevant Files
Create: `M-LIM/tests/CMakeLists.txt` — test target linking Catch2 + JUCE modules
Create: `M-LIM/tests/test_main.cpp` — Catch2 main entry + sanity test
Create: `M-LIM/tests/catch2/catch_amalgamated.hpp` — Catch2 v3 single header
Create: `M-LIM/tests/catch2/catch_amalgamated.cpp` — Catch2 v3 implementation
Create: `M-LIM/tests/dsp/.gitkeep` — DSP test directory
Create: `M-LIM/tests/state/.gitkeep` — state test directory
Create: `M-LIM/tests/integration/.gitkeep` — integration test directory

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build --target MLIMTests -j$(nproc) 2>&1 | tail -5` → Expected: test binary builds
- [ ] Run: `cd M-LIM/build && ctest --output-on-failure 2>&1 | tail -10` → Expected: sanity test passes

## Tests
None (test infrastructure task)

## Technical Details
- Download Catch2 v3 amalgamated from GitHub releases (catch_amalgamated.hpp + catch_amalgamated.cpp)
- Test executable name: MLIMTests
- Link test target against: juce::juce_core, juce::juce_audio_basics, juce::juce_dsp
- The sanity test should verify basic JUCE types work (e.g., `juce::AudioBuffer<float>` creation)
- Add `add_test(NAME MLIMTests COMMAND MLIMTests)` to CMakeLists.txt
- Root CMakeLists.txt must include `add_subdirectory(tests)` with `enable_testing()`

## Dependencies
Requires task 001
