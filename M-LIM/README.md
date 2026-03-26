# M-LIM — Professional Limiter Audio Plugin

A professional-grade limiter plugin for VST3, AU, and CLAP formats, built with JUCE C++. M-LIM delivers 8 distinct limiting algorithms, true peak limiting (ITU-R BS.1770-4), real-time LUFS metering, a scrolling waveform/gain-reduction display, A/B comparison, and a full preset system — designed for mastering, mixing, and broadcast workflows.

---

## Features

- **8 Limiter Algorithms** — from transparent mastering to aggressive loudness maximizing
- **True Peak Limiting** — ITU-R BS.1770-4 inter-sample peak detection (up to 4x oversampled)
- **LUFS Metering** — momentary, short-term, integrated, and loudness range (EBU R128 / ITU-R BS.1770-4)
- **Up to 32x Oversampling** — minimizes aliasing artifacts at high-ceiling settings
- **Real-Time Waveform Display** — scrolling input waveform with gain reduction overlay and peak markers
- **Dual-Stage Limiting** — fast transient stage (Stage 1) + slow leveling stage (Stage 2)
- **Lookahead** — up to 5 ms look-ahead for zero-overshoot limiting
- **Sidechain Filtering** — HP / LP / Tilt filters on the detection signal
- **Dithering** — TPDF with Basic / Optimized / Weighted noise shaping at 16–24 bit
- **A/B Comparison** — snapshot and compare two settings instantly
- **Preset System** — factory presets for mastering, mixing, and broadcast

---

## Algorithms

| Algorithm | Description |
|-----------|-------------|
| **Transparent** | Minimal coloration: slow attack, wide knee, adaptive release. Ideal for mastering. |
| **Punchy** | Retains transient punch: fast attack, slight saturation and transient boost, tight knee. |
| **Dynamic** | Preserves dynamics: medium attack, slower release, strong transient enhancement. |
| **Aggressive** | Maximum loudness: very fast attack, heavy saturation, hard knee. |
| **Allround** | Balanced all-purpose: moderate attack/release, some saturation and enhancement. |
| **Bus** | Mix bus glue: medium attack, fast release, heavy saturation, wide knee. |
| **Safe** | Conservative limiting: very slow attack/release, no saturation, wide knee. |
| **Modern** | Contemporary loudness: fast attack, light saturation, adaptive release. |

---

## Parameter Reference

| Parameter | Range | Default | Unit | Description |
|-----------|-------|---------|------|-------------|
| Input Gain | -12 to +36 | 0 | dB | Pre-limiter gain boost |
| Output Ceiling | -30 to 0 | -0.1 | dBTP | Maximum output level (true peak) |
| Algorithm | 0–7 | Transparent | — | Limiting character (see table above) |
| Lookahead | 0–5 | 1.0 | ms | Look-ahead buffer for zero-overshoot |
| Attack | 0–100 | 0 | ms | Stage 2 leveling attack time |
| Release | 10–1000 | 100 | ms | Stage 2 leveling release time |
| Transient Link | 0–100 | 75 | % | Channel link amount for Stage 1 (transients) |
| Release Link | 0–100 | 100 | % | Channel link amount for Stage 2 (release) |
| True Peak | on/off | on | — | Enable ITU-R BS.1770-4 inter-sample peak detection |
| Oversampling | Off/2x–32x | Off | — | Oversample ratio for alias-free processing |
| DC Filter | on/off | off | — | Remove DC offset from output |
| Dither | on/off | off | — | Enable output dithering |
| Dither Bit Depth | 16/18/20/22/24 | 16 | bit | Target bit depth for dithering |
| Noise Shaping | Basic/Optimized/Weighted | Basic | — | Dither noise shaping algorithm |
| Bypass | on/off | off | — | Hard bypass |
| Unity Gain | on/off | off | — | Compensate output gain to match input loudness |
| SC HP Freq | 20–2000 | 20 | Hz | Sidechain high-pass filter cutoff |
| SC LP Freq | 2000–20000 | 20000 | Hz | Sidechain low-pass filter cutoff |
| SC Tilt | -6 to +6 | 0 | dB | Sidechain tilt EQ (shelving) |
| Delta | on/off | off | — | Monitor the gain reduction signal only |
| Display Mode | Fast/Slow/SlowDown/Infinite/Off | Fast | — | Waveform display scroll speed |
| Loudness Target | -9/-14/-23/-24 LUFS / Custom | -14 LUFS | — | Reference loudness target for metering |

---

## Build Instructions

### Linux (Ubuntu / Debian)

**1. Install system dependencies**

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake git \
  libasound2-dev libcurl4-openssl-dev \
  libfreetype6-dev libx11-dev libxcomposite-dev \
  libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev \
  libwebkit2gtk-4.0-dev \
  libglu1-mesa-dev mesa-common-dev pkg-config
```

**2. Clone the repository and submodules**

```bash
git clone <repo-url> M-LIM
cd M-LIM
git submodule update --init --recursive
```

**3. Build**

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
```

Outputs are in `build/M-LIM_artefacts/Release/`:
- `VST3/M-LIM.vst3` — VST3 plugin bundle
- `Standalone/M-LIM` — standalone application

---

### macOS

**1. Install dependencies**

```bash
xcode-select --install
brew install cmake
```

**2. Clone and configure**

```bash
git clone <repo-url> M-LIM
cd M-LIM
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release -G Xcode
```

**3. Build**

```bash
cmake --build build --config Release -j$(nproc)
```

Outputs include VST3 and AU (Audio Unit) formats.

---

## Running Tests

```bash
cd build
ctest --output-on-failure -j$(nproc)
```

The test suite covers DSP unit tests (TransientLimiter, LevelingLimiter, TruePeakDetector, LoudnessMeter, Oversampler, SidechainFilter, DCFilter, Dither, LimiterEngine) and integration tests (parameter state round-trip, plugin lifecycle).

---

## Plugin Installation

### Linux

```bash
# VST3 (user)
cp -r build/M-LIM_artefacts/Release/VST3/M-LIM.vst3 ~/.vst3/

# VST3 (system-wide)
sudo cp -r build/M-LIM_artefacts/Release/VST3/M-LIM.vst3 /usr/lib/vst3/
```

### macOS

```bash
# VST3 (user)
cp -r build/M-LIM_artefacts/Release/VST3/M-LIM.vst3 ~/Library/Audio/Plug-Ins/VST3/

# AU (user) — Logic, GarageBand, MainStage
cp -r build/M-LIM_artefacts/Release/AU/M-LIM.component ~/Library/Audio/Plug-Ins/Components/
```

After installation, rescan plugins in your DAW.

---

## DAW Compatibility

| Format | Compatibility |
|--------|--------------|
| **VST3** | Ableton Live, Cubase, Nuendo, Reaper, Bitwig, FL Studio, Studio One, Wavelab, and all VST3-capable hosts |
| **AU** | Logic Pro X, GarageBand, MainStage, Final Cut Pro (macOS only) |
| **CLAP** | Reaper 6.74+, Bitwig Studio 5+, and other CLAP-capable hosts |
| **Standalone** | Runs as a standalone application for testing and live use |

---

## Factory Presets

| Category | Presets |
|----------|---------|
| **Mastering** | Transparent Master, Dynamic Master, Loud Master, Wall of Sound |
| **Mixing** | Bus Glue, Drum Bus, Vocal Limiter |
| **Broadcast** | Streaming (-14 LUFS), EBU R128 (-23 LUFS), ATSC A/85 (-24 LUFS), Podcast |

---

## Project Structure

```
M-LIM/
├── CMakeLists.txt              # Build configuration
├── libs/
│   ├── JUCE/                   # JUCE framework (submodule)
│   └── clap-juce-extensions/   # CLAP format support (submodule)
├── src/
│   ├── PluginProcessor.h/.cpp  # AudioProcessor entry point
│   ├── PluginEditor.h/.cpp     # Main UI window
│   ├── Parameters.h/.cpp       # All automatable parameters
│   ├── dsp/                    # DSP engine components
│   │   ├── LimiterEngine       # DSP chain orchestrator
│   │   ├── TransientLimiter    # Stage 1: fast peak limiter
│   │   ├── LevelingLimiter     # Stage 2: slow release limiter
│   │   ├── LimiterAlgorithm    # Algorithm enum + parameter sets
│   │   ├── TruePeakDetector    # ITU-R BS.1770-4 inter-sample peaks
│   │   ├── Oversampler         # Up to 32x oversampling
│   │   ├── SidechainFilter     # HP/LP/Tilt detection filter
│   │   ├── DCFilter            # DC offset removal
│   │   ├── Dither              # TPDF + noise shaping
│   │   └── LoudnessMeter       # LUFS metering
│   ├── ui/                     # UI components
│   │   ├── WaveformDisplay     # Real-time waveform + GR display
│   │   ├── LevelMeter          # Stereo level bars
│   │   ├── GainReductionMeter  # GR meter + peak labels
│   │   ├── LoudnessPanel       # LUFS readout panel
│   │   ├── RotaryKnob          # Custom knob component
│   │   ├── AlgorithmSelector   # Algorithm dropdown
│   │   ├── ControlStrip        # Bottom controls bar
│   │   ├── TopBar              # Presets, A/B, undo toolbar
│   │   ├── LookAndFeel         # Dark theme styling
│   │   └── Colours.h           # Color constants
│   └── state/
│       ├── ABState             # A/B comparison snapshots
│       ├── PresetManager       # Preset load/save/browse
│       └── UndoManager         # Undo/redo stack
├── presets/                    # Factory preset XML files
├── tests/                      # Catch2 test suite
└── resources/                  # Embedded resources
```

---

## License

License TBD. All rights reserved until a license is specified.
