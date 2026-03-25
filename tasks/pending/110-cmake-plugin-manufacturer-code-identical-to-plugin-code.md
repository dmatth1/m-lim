# Task 110: PLUGIN_MANUFACTURER_CODE and PLUGIN_CODE Are Identical in CMakeLists.txt

## Description
`CMakeLists.txt` sets:
```cmake
PLUGIN_MANUFACTURER_CODE "Mlim"
PLUGIN_CODE              "Mlim"
```

Both use the same 4-character code "Mlim". These two codes serve distinct purposes:

- `PLUGIN_MANUFACTURER_CODE` — identifies the **company/developer** across all their plugins.
  Maps to `JucePlugin_ManufacturerCode` and is used as the AU `componentManufacturer`.
- `PLUGIN_CODE` — identifies this **specific plugin** within that manufacturer's catalogue.
  Maps to `JucePlugin_PluginCode` and is used as the AU `componentSubType`.

When both are identical:
- **AU (AudioUnit)**: `componentSubType == componentManufacturer`, which violates the
  AudioUnit convention and can confuse host lookups (the pair must be unique per plugin
  family, and `subType != manufacturer` is strongly recommended by Apple).
- **VST2**: the 4-byte plugin ID is derived from `PLUGIN_CODE`; having the same value
  as the manufacturer code is harmless for VST2 but is indicative of an oversight.
- **VST3**: a UUID is generated from both codes; identical values produce a degenerate
  UUID that could collide with other plugins using the same shortcut.

**Fix**: change `PLUGIN_MANUFACTURER_CODE` to a distinct 4-character uppercase code that
identifies the developer, e.g. `"MlAu"` or `"Maud"`. Keep `PLUGIN_CODE "Mlim"` (it
already identifies this specific plugin).

Suggested values (must be 4 ASCII characters, all printable):
```cmake
PLUGIN_MANUFACTURER_CODE "MlAu"   # "Mlim Audio" developer
PLUGIN_CODE              "MLim"   # M-LIM plugin (uppercase for clarity)
```

After changing, the VST3 and AU GUIDs will change, so any existing DAW sessions that
saved the plugin by GUID will lose the connection. This is acceptable at this stage of
development before any release.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/CMakeLists.txt` — lines 76–77: change `PLUGIN_MANUFACTURER_CODE` to a distinct code

## Acceptance Criteria
- [ ] Run: `grep "PLUGIN_MANUFACTURER_CODE\|PLUGIN_CODE" /workspace/M-LIM/CMakeLists.txt` → Expected: two lines with DIFFERENT 4-character string values.
- [ ] Run: `cd /workspace/M-LIM && cmake -B build -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -5` → Expected: CMake configure succeeds without errors.
- [ ] Run: `cd /workspace/M-LIM && cmake --build build -j$(nproc) 2>&1 | tail -5` → Expected: build succeeds.

## Tests
None — CMake configuration change only; no DSP logic affected.

## Technical Details
- 4-character codes are case-sensitive; each character must be in the printable ASCII range.
- Apple recommends all-uppercase or mixed-case for AU codes; the JUCE convention follows suit.
- Changing these codes is a breaking change for saved sessions — acceptable pre-release.
- After the change, regenerate the build directory (`cmake -B build`) to pick up the new GUIDs.

## Dependencies
None
