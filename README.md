# M-LIM

A free, professional limiter plugin with 8 algorithms, true peak limiting, loudness metering, and a real-time waveform display.

Works in any DAW on macOS. VST3, AU, and Standalone formats.

## Features

- **8 limiting algorithms** — Transparent, Punchy, Dynamic, Allround, Aggressive, Modern, Bus, and Safe
- **True peak limiting** — prevents inter-sample peaks from exceeding your ceiling (ITU-R BS.1770-4 compliant)
- **Loudness metering** — Momentary, Short Term, and Integrated LUFS with LRA measurement
- **Real-time waveform display** — scrolling input/output waveform with gain reduction overlay and peak labels
- **Up to 32x oversampling** — reduces aliasing for cleaner limiting
- **Adjustable lookahead, attack, and release** — shape the limiter's response
- **Channel linking** — separate transient and release linking controls
- **Sidechain filter** — HP/LP/Tilt filter on the detection path
- **Dithering** — TPDF with noise shaping (Basic, Optimized, Weighted)
- **DC offset filter** — removes DC bias from the signal
- **External sidechain** — trigger limiting from a separate audio source
- **Unity gain mode** — listen to the effect of limiting at matched volume
- **Audition limiting** — hear only the gain reduction (delta signal)
- **A/B comparison** — switch between two parameter snapshots
- **Undo/redo** — full parameter history
- **Presets** — factory presets organized by genre, plus save your own
- **Multiple meter scales** — 16dB, 32dB, 48dB, K-12, K-14, K-20, Loudness

---

## Download & Install

*Coming soon — macOS installer will be available here.*

<!--
**[Download M-LIM Installer (macOS)](https://github.com/dmatth1/m-lim/releases/latest/download/M-LIM-macOS-Installer.pkg)**

1. Download the installer
2. Right-click → Open (required for unsigned installer)
3. Click Install (installs both VST3 and AU)
4. Rescan plugins in your DAW
-->

---

## How to Use

### Basic Limiting

1. Insert M-LIM on your master bus or individual track
2. Drag the **Gain** slider up to push into the limiter
3. Set the **Output Level** ceiling (typically -0.1 to -1.0 dBTP)
4. Choose a **Style** that fits your material

### Limiting Styles

| Style | Character | Best for |
|-------|-----------|----------|
| **Transparent** | Clean, minimal coloring | Rock, pop, acoustic |
| **Punchy** | Adds punch when pushed | Vocals, bass, beat-oriented |
| **Dynamic** | Enhances transients | Rock, drums |
| **Allround** | Balanced loudness/transparency | General purpose |
| **Aggressive** | Near-clipping limiting | EDM, metal, pop |
| **Modern** | Very high loudness, clean | All-purpose mastering |
| **Bus** | Glue, pump, squash | Drum bus, individual tracks |
| **Safe** | Zero distortion | Classical, acoustic, delicate |

### Controls

- **Gain** — input gain, pushes signal into the limiter
- **Style** — selects limiting algorithm
- **Lookahead** — 0-5ms, longer = safer, shorter = more transient punch
- **Attack** — how quickly the release envelope engages
- **Release** — how quickly gain reduction recovers
- **Channel Linking** — separate Transient and Release knobs (0-100%)
- **Output Level** — maximum output ceiling

### Metering

- **TP** button — toggles true peak metering (green = within ceiling, orange/red = exceeding)
- **Loudness** button — opens the loudness panel with LUFS readouts
- **Meter Scale** — switch between dB scales and K-System (K-12, K-14, K-20)

### Output Options

- **DC Filter** — removes DC offset from input
- **Sidechain** — trigger limiting from external audio
- **Unity Gain** — match output to input level for A/B comparison
- **Audition** — hear only the gain reduction

---

## DAW Setup

| DAW | Steps |
|-----|-------|
| **Logic Pro** | Preferences → Plug-In Manager → find M-LIM under SG → insert as Audio FX |
| **Ableton Live** | Preferences → Plug-Ins → verify VST3 folder → find M-LIM in browser |
| **Reaper** | Options → Preferences → Plug-ins → VST → add VST3 folder → Re-scan |
| **Bitwig** | Settings → Plug-ins → Locations → add VST3 folder → Rescan |
| **GarageBand** | Install AU version → M-LIM appears automatically in Audio Units |

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Plugin not showing in DAW | Rescan plugins in DAW preferences |
| AU not visible in Logic | Run `sudo killall -9 coreaudiod` in Terminal to clear AU cache |
| High CPU usage | Reduce oversampling (8x is a good balance) |

---

## Building from Source

Requires CMake 3.22+, Xcode 14+ (macOS), or GCC 10+ (Linux). JUCE is included as a git submodule.

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## License

MIT License — see [LICENSE](LICENSE).
