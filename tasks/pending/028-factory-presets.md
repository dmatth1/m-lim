# Task 028: Factory Presets

## Description
Create a comprehensive set of factory presets covering common mastering, mixing, and broadcast use cases. Presets should be XML files loadable by the PresetManager.

## Produces
None

## Consumes
PresetManagerInterface
ParameterLayout

## Relevant Files
Create: `M-LIM/presets/Mastering/Transparent_Master.xml`
Create: `M-LIM/presets/Mastering/Loud_Master.xml`
Create: `M-LIM/presets/Mastering/Dynamic_Master.xml`
Create: `M-LIM/presets/Mastering/Wall_of_Sound.xml`
Create: `M-LIM/presets/Mixing/Bus_Glue.xml`
Create: `M-LIM/presets/Mixing/Drum_Bus.xml`
Create: `M-LIM/presets/Mixing/Vocal_Limiter.xml`
Create: `M-LIM/presets/Broadcast/EBU_R128.xml`
Create: `M-LIM/presets/Broadcast/Streaming_14LUFS.xml`
Create: `M-LIM/presets/Broadcast/Podcast.xml`
Create: `M-LIM/presets/Default.xml`
Read: `M-LIM/src/Parameters.h` — parameter IDs and ranges
Read: `M-LIM/src/state/PresetManager.h` — expected XML format

## Acceptance Criteria
- [ ] Run: `ls M-LIM/presets/**/*.xml | wc -l` → Expected: at least 10 preset files
- [ ] Run: `xmllint --noout M-LIM/presets/Default.xml 2>&1` → Expected: valid XML (or use grep to verify XML structure)

## Tests
None (data files — tested via PresetManager tests)

## Technical Details
- Each preset is a JUCE ValueTree serialized as XML
- Root element: `<MLIM_PRESET>` with child parameter values
- Preset parameter values (sensible defaults for each use case):
  - Transparent Master: algorithm=Transparent, lookahead=1.0, output=-0.3, oversampling=4x
  - Loud Master: algorithm=Modern, lookahead=0.5, output=-0.1, oversampling=4x
  - Dynamic Master: algorithm=Dynamic, lookahead=1.5, output=-0.3
  - Wall of Sound: algorithm=Aggressive, lookahead=0.1, output=-0.1
  - Bus Glue: algorithm=Bus, lookahead=2.0, release=200
  - Drum Bus: algorithm=Punchy, lookahead=0.5, release=50
  - EBU R128: algorithm=Safe, output=-1.0, truePeak=on
  - Streaming: algorithm=Transparent, output=-1.0, truePeak=on
  - Default: algorithm=Modern, all defaults from Parameters.h

## Dependencies
Requires task 015
