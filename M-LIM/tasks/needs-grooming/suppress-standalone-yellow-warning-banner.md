# Task: Suppress JUCE Standalone Yellow Warning Banner for Screenshot RMSE

## Description
The JUCE standalone wrapper displays a bright yellow "Audio input is muted to avoid feedback loop" banner at the top of the window. This occupies ~30px of height and is a massive RMSE contributor — the top bar region has 54.9% RMSE solely because of this bright yellow strip vs the reference's dark gray top bar area.

The banner appears at Y=25-55 in the screenshot. Current pixel values at Y=35: srgb(250,250,210) (bright yellow) vs reference srgb(51,46,53) (dark gray). This single element adds significant error to the Full RMSE measurement.

The fix should either:
1. Override JUCE's `StandaloneFilterWindow` to suppress the warning banner, OR
2. Add `audioDeviceAboutToStart()` callback to unmute the input (safe in standalone context), OR
3. Override the standalone wrapper's `createEditor()` to remove the notification bar

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/PluginProcessor.cpp` — main processor entry point
Read: `CMakeLists.txt` — standalone target configuration
Modify: `src/PluginProcessor.cpp` — may need to override standalone behavior
Read: `libs/JUCE/modules/juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h` — JUCE standalone wrapper source

## Acceptance Criteria
- [ ] Run: `cmake --build build --target MLIM_Standalone -j$(nproc)` → Expected: builds successfully
- [ ] Run: launch standalone, take screenshot → Expected: no yellow "Audio input is muted" banner visible
- [ ] Run: RMSE comparison → Expected: top bar RMSE drops from ~54.9% to <20%

## Tests
None

## Technical Details
- The banner is drawn by `juce::StandaloneFilterWindow` when audio input is muted
- Options to suppress:
  1. Set `StandalonePluginHolder::Settings::shouldMuteInput = false` in the AudioProcessor's `wrapperType` config
  2. Create a custom `StandaloneFilterApp` that overrides the default behavior
  3. In CMakeLists.txt, set `JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP` and provide a custom main that configures the standalone holder
- The simplest approach is likely setting `shouldMuteInput = false` in the properties or via the JUCE standalone plugin holder settings
- JUCE's `StandalonePluginHolder` reads `shouldMuteInput` from `PropertiesFile` settings; an alternative is to supply a `PropertySet` with `shouldMuteInput=0`

## Dependencies
None
