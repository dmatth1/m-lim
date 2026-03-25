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
- [ ] Run: `find M-LIM/presets -name "*.xml" | wc -l` → Expected: at least 10 preset files
- [ ] Run: `head -3 M-LIM/presets/Default.xml` → Expected: XML with `<Parameters>` root element (NOT `<MLIMState>` or `<MLIM_PRESET>`)
- [ ] Run: `grep -l "<PARAM id=" M-LIM/presets/Default.xml` → Expected: Default.xml (parameters use PARAM child format)

## Tests
None (data files — tested via PresetManager tests)

## Technical Details
- Each preset is a JUCE AudioProcessorValueTreeState (APVTS) state serialized as XML.
- **Root element MUST be `<Parameters>`** (the APVTS tree type in PluginProcessor.cpp line 14).
  Using any other root tag (e.g. `<MLIM_PRESET>`) will break plugin state serialization
  (`setStateInformation` checks `hasTagName(apvts.state.getType())`).
- Each parameter is a `<PARAM id="paramID" value="N"/>` child element.
- **Existing preset files at `M-LIM/presets/` use a wrong format — overwrite them.**
- Use exact parameter IDs from `Parameters.h` (e.g. `oversamplingFactor` not `oversample`,
  `channelLinkTransients`/`channelLinkRelease` not `stereoLink`, `ditherEnabled` not `dither`).

Example preset format (save via APVTS, or hand-write matching this structure):
```xml
<?xml version="1.0" encoding="UTF-8"?>
<Parameters>
  <PARAM id="inputGain" value="0.0"/>
  <PARAM id="outputCeiling" value="-0.1"/>
  <PARAM id="lookahead" value="5.0"/>
  <PARAM id="attack" value="0.5"/>
  <PARAM id="release" value="50.0"/>
  <PARAM id="algorithm" value="0"/>
  <PARAM id="oversamplingFactor" value="2"/>
  <PARAM id="truePeakEnabled" value="1"/>
  <PARAM id="channelLinkTransients" value="1"/>
  <PARAM id="channelLinkRelease" value="1"/>
  <PARAM id="dcFilterEnabled" value="0"/>
  <PARAM id="ditherEnabled" value="0"/>
  <PARAM id="ditherBitDepth" value="0"/>
  <PARAM id="ditherNoiseShaping" value="0"/>
  <PARAM id="bypass" value="0"/>
  <PARAM id="unityGainMode" value="0"/>
  <PARAM id="sidechainHPFreq" value="20.0"/>
  <PARAM id="sidechainLPFreq" value="20000.0"/>
  <PARAM id="sidechainTilt" value="0.0"/>
  <PARAM id="delta" value="0"/>
  <PARAM id="displayMode" value="0"/>
</Parameters>
```

- Preset parameter values (sensible defaults for each use case):
  - Transparent Master: algorithm=6 (Transparent), lookahead=1.0, outputCeiling=-0.3, oversamplingFactor=4
  - Loud Master: algorithm=1 (Modern), lookahead=0.5, outputCeiling=-0.1, oversamplingFactor=4
  - Dynamic Master: algorithm=4 (Dynamic), lookahead=1.5, outputCeiling=-0.3
  - Wall of Sound: algorithm=3 (Aggressive), lookahead=0.1, outputCeiling=-0.1
  - Bus Glue: algorithm=5 (Bus), lookahead=2.0, release=200
  - Drum Bus: algorithm=2 (Punchy), lookahead=0.5, release=50
  - EBU R128: algorithm=7 (Safe), outputCeiling=-1.0, truePeakEnabled=1
  - Streaming (14 LUFS): algorithm=6 (Transparent), outputCeiling=-1.0, truePeakEnabled=1
  - Default: algorithm=0 (Modern), all defaults from Parameters.h
  - Check `LimiterAlgorithm.h` for correct algorithm index values

## Dependencies
Requires task 015
