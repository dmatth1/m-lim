# Task 074: Add AU (AudioUnit) Format Target to CMakeLists.txt

## Description
The SPEC lists VST3/AU/CLAP as the three target plugin formats. `CMakeLists.txt` only builds
VST3 (and conditionally CLAP). The `AudioUnit` format is completely absent from `PLUGIN_FORMATS`.

AU is macOS-only so it must be guarded with an `APPLE` platform check. Without it, macOS DAW
users (Logic Pro, GarageBand, etc.) cannot load M-LIM.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `CMakeLists.txt` — add conditional AU format

## Acceptance Criteria
- [ ] Run: `grep -E "AudioUnit|AU" M-LIM/CMakeLists.txt` → Expected: line containing `AudioUnit` inside a platform guard
- [ ] Run: `cmake -B build -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -i "error"` → Expected: no cmake errors (AU skipped on Linux automatically)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds (AU artifact only on macOS)

## Tests
None

## Technical Details
In `CMakeLists.txt`, change the `PLUGIN_FORMATS` section to:

```cmake
set(PLUGIN_FORMATS VST3)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()
if(CLAP_SUPPORT)
    list(APPEND PLUGIN_FORMATS CLAP)
endif()
```

On macOS JUCE requires `juce_audio_plugin_client` which is already pulled in by
`juce::juce_audio_processors`. No extra link libraries are needed. AU builds also require Xcode
and the CoreAudio SDK but those are macOS-only constraints.

## Dependencies
None
