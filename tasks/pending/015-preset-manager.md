# Task 015: Preset Manager

## Description
Implement preset management system with save/load/browse capabilities and factory presets.

## Produces
Implements: `PresetManagerInterface`

## Consumes
None

## Relevant Files
Create: `M-LIM/src/state/PresetManager.h` — class declaration
Create: `M-LIM/src/state/PresetManager.cpp` — implementation
Create: `M-LIM/tests/state/test_preset_manager.cpp` — unit tests
Create: `M-LIM/presets/Default.xml` — default preset
Create: `M-LIM/presets/Mastering/Transparent_Master.xml` — factory preset
Create: `M-LIM/presets/Mastering/Loud_Master.xml` — factory preset
Create: `M-LIM/presets/Mixing/Bus_Glue.xml` — factory preset
Create: `M-LIM/presets/Mixing/Drum_Bus.xml` — factory preset
Create: `M-LIM/presets/Broadcast/EBU_R128.xml` — factory preset
Create: `M-LIM/presets/Broadcast/Streaming.xml` — factory preset
Read: `SPEC.md` — PresetManagerInterface

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake --build build --target MLIMTests -j$(nproc) && cd build && ctest -R test_preset_manager --output-on-failure` → Expected: all tests pass

## Tests
- Unit: `tests/state/test_preset_manager.cpp::test_save_and_load` — save a preset, load it, verify parameters match
- Unit: `tests/state/test_preset_manager.cpp::test_get_preset_names` — returns list of available presets
- Unit: `tests/state/test_preset_manager.cpp::test_next_previous` — navigate through presets sequentially
- Unit: `tests/state/test_preset_manager.cpp::test_factory_presets_exist` — verify factory presets are loadable

## Technical Details
- Presets stored as XML (juce::ValueTree serialized to XML)
- User presets in: `juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("M-LIM/Presets/")`
- Factory presets embedded as BinaryData or loaded from presets/ directory
- Each preset XML contains full APVTS state tree
- nextPreset/previousPreset cycle through sorted preset list
- getCurrentPresetName tracks the active preset name

## Dependencies
Requires task 001
