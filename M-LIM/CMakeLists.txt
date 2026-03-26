cmake_minimum_required(VERSION 3.22)
project(MLIM VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional ccache support
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# JUCE
add_subdirectory(libs/JUCE)

# ALSA (Linux MIDI support)
if(UNIX AND NOT APPLE)
    find_package(ALSA QUIET)
endif()

# clap-juce-extensions (optional, skip if not present)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/clap-juce-extensions/CMakeLists.txt")
    add_subdirectory(libs/clap-juce-extensions)
    set(CLAP_SUPPORT ON)
else()
    set(CLAP_SUPPORT OFF)
endif()

# Plugin formats
set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()
if(CLAP_SUPPORT)
    list(APPEND PLUGIN_FORMATS CLAP)
endif()

# MLIMLib — static library of all DSP/state source files shared by plugin and tests
add_library(MLIMLib STATIC
    src/dsp/DCFilter.cpp
    src/dsp/Dither.cpp
    src/dsp/SidechainFilter.cpp
    src/dsp/Oversampler.cpp
    src/dsp/LoudnessMeter.cpp
    src/dsp/TransientLimiter.cpp
    src/dsp/LevelingLimiter.cpp
    src/dsp/TruePeakDetector.cpp
    src/dsp/LimiterEngine.cpp
    src/state/PresetManager.cpp
    src/state/ABState.cpp
)

target_include_directories(MLIMLib
    PUBLIC
        src
)

target_compile_definitions(MLIMLib
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

set_target_properties(MLIMLib PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(MLIMLib
    PUBLIC
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

juce_add_plugin(MLIM
    VERSION "1.0.0"
    COMPANY_NAME "MLIMaudio"
    COMPANY_WEBSITE ""
    COMPANY_EMAIL ""
    PLUGIN_MANUFACTURER_CODE "MlAu"
    PLUGIN_CODE "MLim"
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "M-LIM"
    PLUGIN_NAME "M-LIM"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(MLIM
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        src/Parameters.cpp
        src/ui/LookAndFeel.cpp
        src/ui/AlgorithmSelector.cpp
        src/ui/RotaryKnob.cpp
        src/ui/LevelMeter.cpp
        src/ui/ControlStrip.cpp
        src/ui/WaveformDisplay.cpp
        src/ui/LoudnessPanel.cpp
        src/ui/GainReductionMeter.cpp
        src/ui/TopBar.cpp
)

target_compile_definitions(MLIM
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_include_directories(MLIM
    PRIVATE
        src
)

target_link_libraries(MLIM
    PRIVATE
        MLIMLib
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        $<$<BOOL:${ALSA_FOUND}>:${ALSA_LIBRARIES}>
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(CLAP_SUPPORT)
    clap_juce_extensions_plugin(TARGET MLIM
        CLAP_ID "com.mlimaudio.mlim"
        CLAP_FEATURES audio-effect limiter mastering
    )
endif()

# Tests
enable_testing()
add_subdirectory(tests)
