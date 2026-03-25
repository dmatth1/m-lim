# Task 001: Project Setup

## Description
Set up the JUCE CMake project skeleton for M-LIM plugin. Initialize git submodules for JUCE and clap-juce-extensions, create the directory structure, and configure CMakeLists.txt to build VST3/AU/CLAP targets. Create stub files for PluginProcessor and PluginEditor so the project compiles.

## Produces
Implements: `PluginProcessorCore`

## Consumes
None

## Relevant Files
Create: `M-LIM/CMakeLists.txt` — root CMake config with JUCE, plugin targets, test target
Create: `M-LIM/src/PluginProcessor.h` — stub AudioProcessor class
Create: `M-LIM/src/PluginProcessor.cpp` — stub implementation
Create: `M-LIM/src/PluginEditor.h` — stub AudioProcessorEditor
Create: `M-LIM/src/PluginEditor.cpp` — stub implementation
Create: `M-LIM/src/Parameters.h` — parameter layout declaration
Create: `M-LIM/src/Parameters.cpp` — createParameterLayout() with all parameters
Create: `.gitmodules` — JUCE and clap-juce-extensions submodules
Create: `.gitignore` — build/, .env, IDE files, OS files

## Acceptance Criteria
- [ ] Run: `cd M-LIM && cmake -B build -DCMAKE_BUILD_TYPE=Debug 2>&1 | tail -5` → Expected: CMake configures successfully with no errors
- [ ] Run: `cd M-LIM && cmake --build build --target MLIM_VST3 -j$(nproc) 2>&1 | tail -5` → Expected: VST3 target builds successfully
- [ ] Run: `ls M-LIM/src/dsp/ M-LIM/src/ui/ M-LIM/src/state/` → Expected: directories exist (may be empty)
- [ ] Run: `ls M-LIM/libs/JUCE/CMakeLists.txt` → Expected: file exists (submodule initialized)

## Tests
None (setup task)

## Technical Details
- Use JUCE 7.x from git: https://github.com/juce-framework/JUCE.git
- Use clap-juce-extensions: https://github.com/free-audio/clap-juce-extensions.git
- CMake minimum version 3.22
- C++17 standard
- Plugin name: "M-LIM", manufacturer: "MLIMaudio"
- Plugin code: "Mlim", manufacturer code: "Mlim"
- Install system dependencies: `sudo apt-get install -y build-essential cmake git libasound2-dev libcurl4-openssl-dev libfreetype6-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libwebkit2gtk-4.0-dev libglu1-mesa-dev mesa-common-dev pkg-config`
- Parameter layout must include ALL parameters from ParameterLayout interface in SPEC.md
- PluginProcessor must have `juce::AudioProcessorValueTreeState apvts` member initialized with createParameterLayout()
- Stub processBlock should just pass audio through unchanged

## Dependencies
None
