# Project: M-LIM — Professional Limiter Audio Plugin

## Goal
Build a professional-grade limiter audio plugin (VST3/AU/CLAP) using JUCE C++ framework that achieves full feature and UI parity with FabFilter Pro-L 2, including 8 limiter algorithms, true peak limiting, loudness metering, and a polished dark-themed UI with real-time waveform display.

## Architecture
M-LIM uses a JUCE-based plugin architecture with clean separation between DSP processing and UI rendering. The audio processor runs on the audio thread with lock-free communication to the UI via atomic variables and lock-free FIFOs. The DSP engine uses a dual-stage limiter design (fast transient stage + slow release/leveling stage) with per-algorithm parameter tuning.

### Core Design Decisions
- **JUCE 7.x** as the framework (CMake-based build)
- **Dual-stage limiter**: Stage 1 = fast peak/transient limiter (controlled by Lookahead), Stage 2 = slow leveling limiter (controlled by Attack/Release)
- **Lock-free audio-to-UI communication**: Atomic parameters + lock-free FIFO for waveform/meter data
- **Oversampling**: JUCE's `dsp::Oversampling` class, up to 32x
- **True peak detection**: 4x oversampled peak measurement per ITU-R BS.1770-4
- **Loudness metering**: ITU-R BS.1770-4 compliant (K-weighted, gated)
- **State management**: JUCE `AudioProcessorValueTreeState` for all parameters with A/B snapshots
- **UI rendering**: Custom JUCE `Component` classes with `Graphics` path-based drawing, 60fps timer-driven repaint

## Technology Stack
- Language: C++17
- Framework: JUCE 7.x
- Build system: CMake 3.22+
- Plugin formats: VST3, AU, CLAP (via clap-juce-extensions)
- Test framework: Catch2 v3 (header-only) + JUCE AudioPluginHost for manual testing
- CI: CMake build verification

## File Structure
```
M-LIM/
├── CMakeLists.txt                    # Root CMake config
├── libs/
│   ├── JUCE/                         # JUCE submodule
│   └── clap-juce-extensions/         # CLAP support submodule
├── src/
│   ├── PluginProcessor.h/.cpp        # Main AudioProcessor
│   ├── PluginEditor.h/.cpp           # Main AudioProcessorEditor
│   ├── Parameters.h/.cpp             # Parameter definitions & layout
│   ├── dsp/
│   │   ├── LimiterEngine.h/.cpp      # Top-level DSP orchestrator
│   │   ├── TransientLimiter.h/.cpp   # Stage 1: fast peak limiter
│   │   ├── LevelingLimiter.h/.cpp    # Stage 2: slow release limiter
│   │   ├── LimiterAlgorithm.h        # Algorithm enum + parameter sets
│   │   ├── TruePeakDetector.h/.cpp   # ITU-R BS.1770-4 true peak
│   │   ├── Oversampler.h/.cpp        # Oversampling wrapper
│   │   ├── SidechainFilter.h/.cpp    # HP/LP/Tilt sidechain filter
│   │   ├── DCFilter.h/.cpp           # DC offset removal filter
│   │   ├── Dither.h/.cpp             # TPDF dithering + noise shaping
│   │   └── LoudnessMeter.h/.cpp      # LUFS metering (momentary/short/integrated)
│   ├── ui/
│   │   ├── LookAndFeel.h/.cpp        # Custom dark theme
│   │   ├── WaveformDisplay.h/.cpp    # Main gain reduction waveform
│   │   ├── LevelMeter.h/.cpp         # Vertical level meters
│   │   ├── GainReductionMeter.h/.cpp # GR meter with peak labels
│   │   ├── LoudnessPanel.h/.cpp      # LUFS metering panel
│   │   ├── RotaryKnob.h/.cpp         # Custom rotary knob component
│   │   ├── AlgorithmSelector.h/.cpp  # Style/algorithm dropdown
│   │   ├── ControlStrip.h/.cpp       # Bottom control bar
│   │   ├── TopBar.h/.cpp             # Top toolbar (presets, A/B, undo)
│   │   └── Colours.h                 # Color constants
│   └── state/
│       ├── ABState.h/.cpp            # A/B comparison state
│       ├── PresetManager.h/.cpp      # Preset load/save/browse
│       └── UndoManager.h/.cpp        # Undo/redo stack
├── presets/                          # Factory preset XML files
│   ├── Default.xml
│   ├── Mastering/
│   ├── Mixing/
│   └── Broadcast/
├── tests/
│   ├── CMakeLists.txt
│   ├── catch2/                       # Catch2 header
│   ├── test_main.cpp
│   ├── dsp/
│   │   ├── test_limiter_engine.cpp
│   │   ├── test_transient_limiter.cpp
│   │   ├── test_leveling_limiter.cpp
│   │   ├── test_true_peak.cpp
│   │   ├── test_oversampler.cpp
│   │   ├── test_sidechain_filter.cpp
│   │   ├── test_dc_filter.cpp
│   │   ├── test_dither.cpp
│   │   └── test_loudness_meter.cpp
│   ├── state/
│   │   ├── test_ab_state.cpp
│   │   ├── test_preset_manager.cpp
│   │   └── test_undo_manager.cpp
│   └── integration/
│       ├── test_plugin_processor.cpp
│       └── test_parameter_state.cpp
└── resources/
    └── presets/                      # Embedded preset data
```

## Success Criteria
- [ ] Plugin builds as VST3, AU, and CLAP on Linux (and macOS/Windows with appropriate SDKs)
- [ ] All 8 limiter algorithms produce correct, artifact-free output
- [ ] True peak limiting keeps inter-sample peaks within 0.1 dB of ceiling at 4x oversampling
- [ ] Loudness metering matches ITU-R BS.1770-4 within 0.1 LU tolerance
- [ ] UI renders at 60fps with real-time waveform display
- [ ] A/B comparison, undo/redo, and preset system fully functional
- [ ] All unit and integration tests pass
- [ ] Plugin loads and processes audio without crashes or memory leaks
- [ ] UI matches Pro-L 2 aesthetic (dark theme, blue/cyan accents, proper layout)
- [ ] Gain reduction display shows waveform history with peak markers
- [ ] Oversampling up to 32x works correctly
- [ ] DC filter and dithering produce correct results
- [ ] Parameter state saves/loads correctly across sessions

## Key Decisions
- Use JUCE's built-in `dsp::Oversampling` rather than rolling our own — well-tested, SIMD-optimized
- Catch2 for unit tests rather than Google Test — simpler integration, header-only
- Custom UI components rather than JUCE widgets — needed for Pro-L 2 visual parity
- Lock-free FIFO for audio→UI data rather than message passing — lower latency for waveform display
- CMake-only build (no Projucer) — modern, CI-friendly, better dependency management
- CLAP support via clap-juce-extensions rather than native CLAP SDK — leverages existing JUCE code

## Interfaces

### PluginProcessorCore
- File: `src/PluginProcessor.h`
- `class MLIMProcessor : public juce::AudioProcessor`
- `void prepareToPlay(double sampleRate, int samplesPerBlock) override`
- `void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override`
- `void releaseResources() override`
- `juce::AudioProcessorEditor* createEditor() override`
- `void getStateInformation(juce::MemoryBlock& destData) override`
- `void setStateInformation(const void* data, int sizeInBytes) override`
- `juce::AudioProcessorValueTreeState apvts` — all automatable parameters
- `LimiterEngine& getLimiterEngine()` — access for editor metering
- `LockFreeFIFO<MeterData>& getMeterFIFO()` — waveform/meter data for UI

### ParameterLayout
- File: `src/Parameters.h`
- `juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()`
- Parameter IDs (all `juce::String`):
  - `"inputGain"` — float, -12 to +36 dB, default 0
  - `"outputCeiling"` — float, -30 to 0 dB, default -0.1
  - `"algorithm"` — choice, 0-7 (Transparent/Punchy/Dynamic/Aggressive/Allround/Bus/Safe/Modern)
  - `"lookahead"` — float, 0 to 5 ms, default 1.0
  - `"attack"` — float, 0 to 100 ms, default 0
  - `"release"` — float, 10 to 1000 ms, default 100
  - `"channelLinkTransients"` — float, 0-100%, default 75
  - `"channelLinkRelease"` — float, 0-100%, default 100
  - `"truePeakEnabled"` — bool, default true
  - `"oversamplingFactor"` — choice, 0-5 (Off/2x/4x/8x/16x/32x)
  - `"dcFilterEnabled"` — bool, default false
  - `"ditherEnabled"` — bool, default false
  - `"ditherBitDepth"` — choice (16/18/20/22/24)
  - `"ditherNoiseShaping"` — choice (Basic/Optimized/Weighted)
  - `"bypass"` — bool, default false
  - `"unityGainMode"` — bool, default false
  - `"delta"` — bool, default false
  - `"sidechainHPFreq"` — float, 20 to 2000 Hz, default 20 (off)
  - `"sidechainLPFreq"` — float, 2000 to 20000 Hz, default 20000 (off)
  - `"sidechainTilt"` — float, -6 to +6 dB, default 0
  - `"displayMode"` — choice, 0-4 (Fast/Slow/SlowDown/Infinite/Off), default 0

### LimiterEngineInterface
- File: `src/dsp/LimiterEngine.h`
- `class LimiterEngine`
- `void prepare(double sampleRate, int maxBlockSize, int numChannels)`
- `void process(juce::AudioBuffer<float>& buffer)` — runs full chain
- `void setAlgorithm(LimiterAlgorithm algo)`
- `void setInputGain(float dB)`
- `void setOutputCeiling(float dB)`
- `void setLookahead(float ms)`
- `void setAttack(float ms)`
- `void setRelease(float ms)`
- `void setChannelLinkTransients(float pct)`
- `void setChannelLinkRelease(float pct)`
- `void setOversamplingFactor(int factor)` — 0=off, 1=2x, 2=4x, etc.
- `void setTruePeakEnabled(bool enabled)`
- `void setDCFilterEnabled(bool enabled)`
- `void setDitherEnabled(bool enabled)`
- `void setDitherBitDepth(int bits)`
- `void setDitherNoiseShaping(int mode)`
- `float getGainReduction() const` — current GR in dB (atomic)
- `float getTruePeakL() const` / `float getTruePeakR() const`
- `int getLatencySamples() const` — total latency for host compensation

### TransientLimiterInterface
- File: `src/dsp/TransientLimiter.h`
- `class TransientLimiter`
- `void prepare(double sampleRate, int maxBlockSize, int numChannels)`
- `void process(float** channelData, int numChannels, int numSamples)`
- `void setLookahead(float ms)` — sets lookahead buffer size
- `void setChannelLink(float pct)` — 0-1 range
- `void setAlgorithmParams(const AlgorithmParams& params)`
- `float getGainReduction() const`

### LevelingLimiterInterface
- File: `src/dsp/LevelingLimiter.h`
- `class LevelingLimiter`
- `void prepare(double sampleRate, int maxBlockSize, int numChannels)`
- `void process(float** channelData, int numChannels, int numSamples)`
- `void setAttack(float ms)`
- `void setRelease(float ms)`
- `void setChannelLink(float pct)`
- `void setAlgorithmParams(const AlgorithmParams& params)`
- `float getGainReduction() const`

### AlgorithmDefinition
- File: `src/dsp/LimiterAlgorithm.h`
- `enum class LimiterAlgorithm { Transparent, Punchy, Dynamic, Aggressive, Allround, Bus, Safe, Modern }`
- `struct AlgorithmParams` — per-algorithm tuning:
  - `float transientAttackCoeff` — how aggressively transients are caught
  - `float releaseShape` — release curve shape (exponential factor)
  - `float saturationAmount` — soft clipping amount (0 = none, 1 = max)
  - `float dynamicEnhance` — transient enhancement before limiting
  - `float kneeWidth` — soft knee width in dB
  - `bool adaptiveRelease` — enable adaptive release behavior
- `AlgorithmParams getAlgorithmParams(LimiterAlgorithm algo)` — returns tuned params

### TruePeakDetectorInterface
- File: `src/dsp/TruePeakDetector.h`
- `class TruePeakDetector`
- `void prepare(double sampleRate)`
- `float processSample(float sample)` — returns true peak value
- `void processBlock(const float* input, int numSamples)` — batch processing
- `float getPeak() const` — current true peak level
- `void reset()`

### OversamplerInterface
- File: `src/dsp/Oversampler.h`
- `class Oversampler`
- `void prepare(double sampleRate, int maxBlockSize, int numChannels)`
- `juce::dsp::AudioBlock<float> upsample(juce::AudioBuffer<float>& buffer)`
- `void downsample(juce::AudioBuffer<float>& buffer)`
- `void setFactor(int factor)` — 0=off, 1=2x, ..., 5=32x
- `int getFactor() const`
- `float getLatencySamples() const`

### SidechainFilterInterface
- File: `src/dsp/SidechainFilter.h`
- `class SidechainFilter`
- `void prepare(double sampleRate, int maxBlockSize)`
- `void process(juce::AudioBuffer<float>& buffer)`
- `void setHighPassFreq(float hz)` — 20-2000 Hz
- `void setLowPassFreq(float hz)` — 2000-20000 Hz
- `void setTilt(float dB)` — -6 to +6 dB tilt

### DCFilterInterface
- File: `src/dsp/DCFilter.h`
- `class DCFilter`
- `void prepare(double sampleRate)`
- `void process(float* data, int numSamples)`
- `void reset()`

### DitherInterface
- File: `src/dsp/Dither.h`
- `class Dither`
- `void process(float* data, int numSamples)`
- `void setBitDepth(int bits)` — 16/18/20/22/24
- `void setNoiseShaping(int mode)` — 0=Basic, 1=Optimized, 2=Weighted

### LoudnessMeterInterface
- File: `src/dsp/LoudnessMeter.h`
- `class LoudnessMeter`
- `void prepare(double sampleRate, int numChannels)`
- `void processBlock(const juce::AudioBuffer<float>& buffer)`
- `float getMomentaryLUFS() const`
- `float getShortTermLUFS() const`
- `float getIntegratedLUFS() const`
- `float getLoudnessRange() const`
- `void resetIntegrated()`

### MeterDataInterface
- File: `src/dsp/MeterData.h` (standalone header — shared between DSP and UI)
- `struct MeterData`:
  - `float inputLevelL, inputLevelR` — input peak levels
  - `float outputLevelL, outputLevelR` — output peak levels
  - `float gainReduction` — current GR in dB
  - `float truePeakL, truePeakR` — true peak levels
  - `std::array<float, 512> waveformBuffer` — waveform snapshot for display
  - `int waveformSize` — valid samples in buffer
- `template<typename T> class LockFreeFIFO` — single-producer single-consumer FIFO

### EditorCore
- File: `src/PluginEditor.h`
- `class MLIMEditor : public juce::AudioProcessorEditor, public juce::Timer`
- `void paint(juce::Graphics& g) override`
- `void resized() override`
- `void timerCallback() override` — 60fps meter/waveform update
- Layout: TopBar (presets/AB/undo) | InputMeter | WaveformDisplay | GRMeter + OutputMeter | ControlStrip (knobs)
- Input level meter on LEFT of waveform, output level meter + GR meter on RIGHT (Pro-L 2 parity)
- Waveform display should occupy ~70-75% of horizontal space between meters
- Default size: 900x500, resizable with aspect ratio constraint

### WaveformDisplayInterface
- File: `src/ui/WaveformDisplay.h`
- `class WaveformDisplay : public juce::Component, public juce::Timer`
- `void pushMeterData(const MeterData& data)` — called from editor timer
- `void paint(juce::Graphics& g) override` — draws layered waveform
- Display modes: Fast, Slow, SlowDown, Infinite, Off
- Layers: gain reduction (red), input level (cyan), output level (dark blue), envelope curves
- Peak markers: yellow labels showing dB values at GR peaks
- Vertical dB scale on right edge

### LevelMeterInterface
- File: `src/ui/LevelMeter.h`
- `class LevelMeter : public juce::Component`
- `void setLevel(float leftDB, float rightDB)`
- `void setPeakHold(float leftDB, float rightDB)`
- `void paint(juce::Graphics& g) override`
- Vertical stereo bar meter with blue/yellow/red zones

### GainReductionMeterInterface
- File: `src/ui/GainReductionMeter.h`
- `class GainReductionMeter : public juce::Component`
- `void setGainReduction(float dB)` — current GR value
- `void setPeakGR(float dB)` — peak hold GR value
- `void paint(juce::Graphics& g) override`
- Vertical bar meter showing gain reduction with peak hold indicator and numeric dB label
- Grows downward from 0 dB (red fill)

### LoudnessPanelInterface
- File: `src/ui/LoudnessPanel.h`
- `class LoudnessPanel : public juce::Component`
- `void setMomentary(float lufs)`
- `void setShortTerm(float lufs)`
- `void setIntegrated(float lufs)`
- `void setLoudnessRange(float lu)`
- `void setTruePeak(float dBTP)`
- `void paint(juce::Graphics& g) override`
- Shows numeric readouts + small bar meters, collapsible

### RotaryKnobInterface
- File: `src/ui/RotaryKnob.h`
- `class RotaryKnob : public juce::Component`
- `void setRange(float min, float max, float step)`
- `void setValue(float val)`
- `void setLabel(const juce::String& label)`
- `void setSuffix(const juce::String& suffix)`
- `std::function<void(float)> onValueChange`
- `void paint(juce::Graphics& g) override` — dark circle, light blue arc, white pointer
- Attaches to `juce::AudioProcessorValueTreeState` via `SliderAttachment`

### AlgorithmSelectorInterface
- File: `src/ui/AlgorithmSelector.h`
- `class AlgorithmSelector : public juce::Component`
- `void setAlgorithm(int index)`
- `int getAlgorithm() const`
- `std::function<void(int)> onAlgorithmChanged`
- `void paint(juce::Graphics& g) override` — styled dropdown with algorithm name
- Attaches to APVTS choice parameter

### TopBarInterface
- File: `src/ui/TopBar.h`
- `class TopBar : public juce::Component`
- Contains: preset name display, prev/next arrows, A/B toggle, copy A↔B, undo/redo buttons
- `void setPresetName(const juce::String& name)`
- `std::function<void()> onUndo, onRedo, onABToggle, onABCopy`

### ControlStripInterface
- File: `src/ui/ControlStrip.h`
- `class ControlStrip : public juce::Component`
- Contains: AlgorithmSelector, Lookahead knob, Attack knob, Release knob, Channel Link knobs
- Bottom row: TP toggle, Oversampling selector, Dither settings, DC toggle, Bypass, Unity, Delta
- All controls attached to APVTS parameters

### ColoursDefinition
- File: `src/ui/Colours.h`
- `namespace MLIMColours`:
  - `background` = #1E1E1E (dark charcoal)
  - `displayBackground` = #141414 (near black)
  - `inputWaveform` = #4FC3F7 (light cyan, alpha 0.6)
  - `outputWaveform` = #1565C0 (dark blue, alpha 0.5)
  - `gainReduction` = #FF4444 (bright red)
  - `peakLabel` = #FFD700 (gold/yellow)
  - `knobFace` = #3A3A3A (medium gray)
  - `knobArc` = #4FC3F7 (light cyan)
  - `knobPointer` = #FFFFFF (white)
  - `textPrimary` = #E0E0E0 (light gray)
  - `textSecondary` = #9E9E9E (medium gray)
  - `meterSafe` = #4FC3F7 (blue)
  - `meterWarning` = #FFD54F (yellow)
  - `meterDanger` = #FF5252 (red)
  - `accentBlue` = #2196F3
  - `panelBorder` = #333333

### ABStateInterface
- File: `src/state/ABState.h`
- `class ABState`
- `void captureState(const juce::AudioProcessorValueTreeState& apvts)` — snapshot current
- `void restoreState(juce::AudioProcessorValueTreeState& apvts)` — restore snapshot
- `void toggle()` — swap A↔B
- `void copyAtoB()` / `void copyBtoA()`
- `bool isA() const` — which state is active

### PresetManagerInterface
- File: `src/state/PresetManager.h`
- `class PresetManager`
- `void savePreset(const juce::String& name, const juce::AudioProcessorValueTreeState& apvts)`
- `bool loadPreset(const juce::String& name, juce::AudioProcessorValueTreeState& apvts)`
- `juce::StringArray getPresetNames() const`
- `void nextPreset()` / `void previousPreset()`
- `juce::String getCurrentPresetName() const`
- Stores presets in user app data directory as XML

### UndoManagerInterface
- File: `src/state/UndoManager.h`
- Uses `juce::UndoManager` internally
- `void beginNewTransaction()`
- `bool undo()` / `bool redo()`
- `bool canUndo() const` / `bool canRedo() const`

### LookAndFeelDefinition
- File: `src/ui/LookAndFeel.h`
- `class MLIMLookAndFeel : public juce::LookAndFeel_V4`
- Overrides: `drawRotarySlider`, `drawLinearSlider`, `drawComboBox`, `drawButtonBackground`
- Sets default colors from `MLIMColours` namespace
- Font: system sans-serif at appropriate sizes

## Specialists

### DSPEngineer
You are an audio DSP specialist. Review all code in `src/dsp/` for: incorrect gain reduction math (should be multiply not add in linear domain), denormal float issues in IIR filters, incorrect sample rate handling in coefficient calculations, lookahead buffer not properly circular, true peak detector not using correct ITU-R BS.1770-4 FIR coefficients, oversampling latency not correctly reported to host, channel linking math producing stereo image artifacts, loudness meter not using correct K-weighting filter coefficients, SIMD optimization opportunities on hot paths (per-sample processing loops). For each issue found, create a task in `tasks/pending/` with the exact code location, the bug, and the correct DSP math. You may fix one-line coefficient errors directly but do not restructure algorithms.

### UIParityAuditor
You are a visual/UI parity specialist. Compare the plugin UI against `/reference-docs/reference-screenshots/` and `/reference-docs/video-frames/`. Check for: incorrect layout proportions (waveform display should be ~70% width), wrong colors (compare hex values against reference), knob styling mismatch (should have light blue arc, white pointer, dark face), missing visual elements (peak labels, dB scale, grid lines), incorrect meter color zones, text alignment and font size issues, missing animations or transitions. For each discrepancy, create a task in `tasks/pending/` with a specific visual description of what's wrong and what it should look like per the reference. You may adjust color constants directly in `Colours.h`.

### PluginArchitect
You are a plugin architecture specialist. Review: JUCE project structure and CMakeLists.txt for correct plugin format targets (VST3/AU/CLAP), parameter management thread safety (APVTS accessed from audio and UI threads), state save/load completeness (all parameters serialized), latency reporting accuracy (`setLatencySamples` matches actual DSP latency), audio thread real-time safety (no allocations, locks, or blocking calls in `processBlock`), correct use of `juce::dsp::ProcessSpec` and prepare/reset lifecycle, proper `AudioProcessorEditor` attachment/detachment lifecycle. For each issue, create a task in `tasks/pending/`.

### ProjectManager
You are a project manager responsible for plan integrity and delivery quality. You run **after** all other specialists in every sweep — your job is to consolidate their output into a clean, actionable task queue.

Every sweep, you must:
1. Read SPEC.md and compare to what's actually built — update the spec to reflect reality
2. Check tasks/done/ for missing artifacts (screenshots, test output) — create fix tasks if needed
3. **Consolidate specialist tasks** — deduplicate overlapping tasks, merge related small tasks, renumber pending tasks sequentially, set dependencies between tasks touching the same files
4. Remove unnecessary pending tasks, fix stale dependencies, re-scope tasks based on implementation learnings
5. Review overall project against the original goal — add tasks for uncovered gaps
6. Check for scope creep — remove or deprioritize tasks beyond what was asked
7. **Build duplication check** — if 3+ tasks build the same binary, consolidate into one build task with `artifact:<path>` in Produces/Consumes
8. **Artifact cleanup** — check for unneeded binary files in git (old screenshots, build outputs). Create cleanup tasks to `git rm` them and add `.gitignore` patterns
9. **Pre-flight validation** (when tasks/done/ is empty) — challenge the plan: right tech stack? Over-engineered? Tasks granular enough for parallelism? Fix issues directly

Do not write code. Focus on task files, SPEC.md, and coordination.

### PrincipalEngineer
You are a principal software engineer performing a code quality audit. Review the entire codebase for: poor abstractions, over-engineering, inconsistent patterns, repeated logic that should be extracted, functions that do too much, and structural issues that will cause pain as the project grows. For each issue found, create a task in `tasks/pending/` with specific file paths, what to change, and why. You may make trivial fixes directly (rename a constant, fix a comment) but do not refactor code, run builds, or run tests — that work belongs to workers via the task queue.

### QAEngineer
You are a QA engineer focused on test coverage and correctness. Review the test suite for: untested code paths, missing negative/error cases, tests that only verify the happy path, assertions that are too weak (e.g. checking existence rather than value), tests that are tightly coupled to implementation details and will break on refactors, and missing edge cases (empty input, max values, concurrent access, partial failures). For each gap, create a task in `tasks/pending/` describing what tests to add and why. Do not write test code or run test suites — workers handle implementation via the task queue.
