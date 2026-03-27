# Task 478: Extract UI Sources from Test CMakeLists into Shared Library

## Description
`tests/CMakeLists.txt` (lines 39–51) directly compiles all UI `.cpp` files (`PluginEditor.cpp`, `LookAndFeel.cpp`, `AlgorithmSelector.cpp`, etc.) instead of linking against the MLIM plugin target or a shared UI library. This means:

1. **UI code is compiled twice** — once for the plugin, once for tests — doubling incremental build time for any UI change.
2. **Divergence risk** — if a new UI source is added to `CMakeLists.txt` but not to `tests/CMakeLists.txt`, tests will fail to link with a confusing error.
3. **The pattern already works for DSP** — `MLIMLib` is a static library shared between plugin and tests. The UI sources should follow the same pattern.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/CMakeLists.txt` — create a `MLIMUILib` static library (or add UI sources to `MLIMLib`) that both the plugin target and test target link against
Modify: `M-LIM/tests/CMakeLists.txt` — remove directly-compiled UI sources (lines 39–51) and `../src/Parameters.cpp` (line 37), replace with `target_link_libraries(MLIMTests PRIVATE MLIMUILib)` or equivalent

## Acceptance Criteria
- [ ] Run: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds, exit 0
- [ ] Run: `cd build && ctest --output-on-failure` → Expected: all tests pass
- [ ] Run: `grep -c "PluginEditor.cpp\|LookAndFeel.cpp\|AlgorithmSelector.cpp" tests/CMakeLists.txt` → Expected: 0 (no direct UI source compilation in test CMake)

## Tests
None

## Technical Details
Two approaches:
1. **Add UI sources to MLIMLib** — simplest, but makes MLIMLib depend on `juce_gui_basics`/`juce_graphics`
2. **Create MLIMUILib** — cleaner separation but more CMake boilerplate

Option 1 is recommended since tests already link `juce_gui_basics` transitively through `juce_audio_utils`.

Parameters.cpp should also move into the shared library since it's needed by both targets.

## Dependencies
None
