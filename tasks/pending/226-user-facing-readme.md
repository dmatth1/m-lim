# Task 226: User-Facing README

## Description
Create a comprehensive `README.md` at the root of the M-LIM project (`/workspace/M-LIM/README.md`). This is the first thing developers and users see on GitHub. It must cover:

1. **Plugin overview**: What M-LIM is, key features (8 limiter algorithms, true peak, LUFS metering, waveform display, A/B comparison, presets)
2. **Build instructions for Linux** (Ubuntu/Debian): exact apt-get packages, cmake commands
3. **Build instructions for macOS**: Xcode/Homebrew dependencies, cmake commands with AU format note
4. **Running tests**: ctest command
5. **DAW compatibility**: VST3 (all major DAWs), AU (macOS only: Logic, GarageBand, MainStage), CLAP (Reaper, Bitwig, etc.)
6. **Plugin installation**: where to copy the .vst3/.component files on each platform
7. **Algorithm descriptions**: brief one-liner for each of the 8 limiter algorithms
8. **Parameter reference**: table of all key parameters with ranges and defaults
9. **Project structure**: brief directory tree with descriptions
10. **License section** (placeholder if not yet decided)

Read the existing source files to get accurate parameter names, ranges, and algorithm names before writing the README.

## Produces
None

## Consumes
None

## Relevant Files
Read: `src/Parameters.h` — parameter names and ranges
Read: `src/Parameters.cpp` — parameter definitions
Read: `src/dsp/LimiterAlgorithm.h` — algorithm enum and names
Read: `CMakeLists.txt` — version, plugin metadata
Read: `presets/` — factory preset names for examples
Create: `README.md` in `/workspace/M-LIM/` — user-facing documentation

## Acceptance Criteria
- [ ] Run: `test -f /workspace/M-LIM/README.md` → Expected: file exists, exit 0
- [ ] Run: `wc -l /workspace/M-LIM/README.md` → Expected: at least 100 lines
- [ ] Run: `grep -c "## " /workspace/M-LIM/README.md` → Expected: at least 6 sections
- [ ] Run: `grep -i "vst3\|au\|clap" /workspace/M-LIM/README.md` → Expected: all three formats mentioned

## Tests
None

## Technical Details
- Use standard GitHub-flavored Markdown
- Code blocks must use triple backticks with language hints (```bash, ```cpp)
- macOS AU format requires Xcode and `-GXcode` or standard cmake (JUCE handles AU automatically on Apple)
- VST3 install path Linux: `~/.vst3/` or `/usr/lib/vst3/`
- VST3 install path macOS: `~/Library/Audio/Plug-Ins/VST3/`
- AU install path macOS: `~/Library/Audio/Plug-Ins/Components/`

## Dependencies
Requires task 224
