# M-LIM — Professional Limiter Audio Plugin

## What This Is
A professional limiter audio plugin (VST3/AU/CLAP) built with JUCE C++ that replicates FabFilter Pro-L 2's features and UI. Includes 8 limiter algorithms, true peak limiting, loudness metering, and a dark-themed waveform display.

## Tech Stack
- Language: C++17
- Framework: JUCE 7.x (git submodule in `libs/JUCE/`)
- Build system: CMake 3.22+
- Plugin formats: VST3, AU, CLAP (via clap-juce-extensions)
- Test framework: Catch2 v3 (header-only in `tests/catch2/`)
- CLAP support: clap-juce-extensions (git submodule in `libs/clap-juce-extensions/`)

## Project Structure
```
M-LIM/
├── CMakeLists.txt              # Root build config
├── libs/JUCE/                  # JUCE framework submodule
├── libs/clap-juce-extensions/  # CLAP format support
├── src/
│   ├── PluginProcessor.h/.cpp  # AudioProcessor (entry point)
│   ├── PluginEditor.h/.cpp     # AudioProcessorEditor (main UI)
│   ├── Parameters.h/.cpp       # APVTS parameter layout
│   ├── dsp/                    # All DSP code
│   │   ├── LimiterEngine       # Top-level DSP orchestrator
│   │   ├── TransientLimiter    # Stage 1: fast peak limiter
│   │   ├── LevelingLimiter     # Stage 2: slow release limiter
│   │   ├── LimiterAlgorithm    # Algorithm enum + params
│   │   ├── TruePeakDetector    # ITU-R BS.1770-4 true peak
│   │   ├── Oversampler         # Up to 32x oversampling
│   │   ├── SidechainFilter     # HP/LP/Tilt filter
│   │   ├── DCFilter            # DC offset removal
│   │   ├── Dither              # TPDF + noise shaping
│   │   └── LoudnessMeter       # LUFS metering
│   ├── ui/                     # All UI components
│   │   ├── LookAndFeel         # Dark theme styling
│   │   ├── WaveformDisplay     # Real-time waveform + GR display
│   │   ├── LevelMeter          # Stereo level bars
│   │   ├── GainReductionMeter  # GR meter + peak labels
│   │   ├── LoudnessPanel       # LUFS readouts panel
│   │   ├── RotaryKnob          # Custom knob component
│   │   ├── AlgorithmSelector   # Style dropdown
│   │   ├── ControlStrip        # Bottom controls
│   │   ├── TopBar              # Presets, A/B, undo
│   │   └── Colours.h           # Color constants
│   └── state/                  # State management
│       ├── ABState             # A/B comparison
│       ├── PresetManager       # Preset system
│       └── UndoManager         # Undo/redo
├── presets/                    # Factory preset XMLs
├── tests/                      # Catch2 test suite
│   ├── dsp/                    # DSP unit tests
│   ├── state/                  # State management tests
│   └── integration/            # Plugin integration tests
└── resources/                  # Embedded resources
```

## Build & Run
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install -y build-essential cmake git libasound2-dev libcurl4-openssl-dev \
  libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev \
  libxinerama-dev libxrandr-dev libxrender-dev libwebkit2gtk-4.0-dev \
  libglu1-mesa-dev mesa-common-dev pkg-config

# Clone submodules
git submodule update --init --recursive

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)

# Run tests
cd build && ctest --output-on-failure
```

## Key Patterns
- **Thread safety**: Audio thread uses only atomics and lock-free FIFOs. No allocations or locks in processBlock.
- **Parameter access**: All parameters via `juce::AudioProcessorValueTreeState` (APVTS). UI attaches via `SliderAttachment`/`ButtonAttachment`.
- **Audio→UI data**: `LockFreeFIFO<MeterData>` pushes waveform/meter snapshots from audio thread. UI pops in 60fps timer.
- **DSP chain**: Input gain → Sidechain filter → Oversampling up → TransientLimiter → LevelingLimiter → Oversampling down → DC filter → Dither → Output ceiling
- **Limiter design**: Dual-stage — Stage 1 (TransientLimiter) catches peaks with lookahead, Stage 2 (LevelingLimiter) shapes release envelope.
- **Naming**: PascalCase for classes, camelCase for methods/variables, SCREAMING_SNAKE for constants.
- **File layout**: One class per .h/.cpp pair. Header has class declaration, cpp has implementation.

## Module Map
- `PluginProcessor` — JUCE entry point, owns LimiterEngine, pumps MeterData FIFO
- `PluginEditor` — Main UI window, owns all UI components, runs 60fps timer
- `Parameters` — Defines all automatable parameters and their ranges
- `LimiterEngine` — Orchestrates full DSP chain (oversample→limit→filter→dither)
- `TransientLimiter` — Fast peak limiter with lookahead buffer
- `LevelingLimiter` — Slow release limiter with attack/release envelope
- `LimiterAlgorithm` — Enum + parameter presets for 8 algorithms
- `TruePeakDetector` — ITU-R BS.1770-4 compliant inter-sample peak detection
- `Oversampler` — Wraps JUCE dsp::Oversampling (2x-32x)
- `LoudnessMeter` — K-weighted LUFS metering (momentary/short/integrated/range)
- `WaveformDisplay` — Real-time scrolling waveform with GR overlay and peak markers
- `LevelMeter` — Vertical stereo bar meter with color zones
- `LoudnessPanel` — Collapsible LUFS readout panel
- `ABState` — A/B comparison snapshot system
- `PresetManager` — XML preset load/save/browse
