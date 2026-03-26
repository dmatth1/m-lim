# Task 224: Plugin Metadata, Build Validation, and DAW Compatibility

## Description
Ensure M-LIM builds cleanly and has correct plugin metadata for DAW compatibility. This includes verifying the VST3/AU/CLAP metadata in CMakeLists.txt, confirming latency reporting is correct, ensuring the plugin passes basic lifecycle tests (load/unload without crash), and verifying parameter serialization round-trips.

Specific checks:
1. **CMakeLists.txt metadata audit**: Verify `PLUGIN_MANUFACTURER_CODE`, `PLUGIN_CODE`, `COMPANY_NAME`, `VERSION` are set correctly. The 4-char codes must be unique and follow VST3 conventions (manufacturer code first char uppercase, remaining lowercase is ideal). Current values are MlAu/MLim — verify these don't conflict with known plugins.
2. **Latency reporting**: `getLatencySamples()` must return the correct lookahead buffer size from `LimiterEngine`. Check that `prepareToPlay` calls `setLatencySamples()` with the right value, and that `setStateInformation` re-applies latency via `updateLatency()`.
3. **`isBusesLayoutSupported`**: Must accept stereo and mono in/out layouts — verify it handles mono gracefully.
4. **`getTailLengthSeconds`**: Should return a small positive value (lookahead/sampleRate) or 0.0 — verify it doesn't return a huge value that would cause DAW issues.
5. **Build the plugin** with the standard build command and verify it produces valid output binaries.
6. **Run the test suite** to ensure no regressions from the FontOptions fix.

## Produces
None

## Consumes
None

## Relevant Files
Read: `CMakeLists.txt` — verify metadata fields
Modify: `CMakeLists.txt` — fix any metadata issues found
Read: `src/PluginProcessor.cpp` — check latency reporting, prepareToPlay, isBusesLayoutSupported, getTailLengthSeconds
Read: `src/PluginProcessor.h` — verify interface
Read: `src/dsp/LimiterEngine.h` — check getLatencySamples() API

## Acceptance Criteria
- [ ] Run: `export CCACHE_DIR=/build-cache && cmake -B /workspace/M-LIM/build -S /workspace/M-LIM -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache && cmake --build /workspace/M-LIM/build -j$(nproc) 2>&1 | tail -5` → Expected: `[100%] Built target MLIM_VST3` (or similar success), exit 0
- [ ] Run: `cd /workspace/M-LIM/build && ctest --output-on-failure -j$(nproc) 2>&1 | tail -20` → Expected: all tests pass
- [ ] Run: `grep -E "PLUGIN_MANUFACTURER_CODE|PLUGIN_CODE|COMPANY_NAME|VERSION" /workspace/M-LIM/CMakeLists.txt` → Expected: all 4 fields present with non-empty values
- [ ] Run: `grep "setLatencySamples" /workspace/M-LIM/src/PluginProcessor.cpp` → Expected: at least one match confirming latency is reported

## Tests
None

## Technical Details
- Build command: `export CCACHE_DIR=/build-cache && cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache && cmake --build build -j$(nproc)`
- VST3 output: `build/MLIM_artefacts/VST3/M-LIM.vst3`
- PLUGIN_MANUFACTURER_CODE must be exactly 4 characters, first char uppercase
- PLUGIN_CODE must be exactly 4 characters, first char uppercase
- Both codes must be unique (avoid common ones like "Manu", "Demo")

## Dependencies
Requires task 223
