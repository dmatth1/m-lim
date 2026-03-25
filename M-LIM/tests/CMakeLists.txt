cmake_minimum_required(VERSION 3.22)

# Collect plugin source files for testing (populated by each DSP task)
set(MLIM_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/dsp/DCFilter.cpp
)

# Collect test source files (populated by each test task)
set(MLIM_TEST_CASES
    dsp/test_dc_filter.cpp
)

# Catch2 test executable
add_executable(MLIMTests
    test_main.cpp
    catch2/catch_amalgamated.cpp
    ${MLIM_TEST_SOURCES}
    ${MLIM_TEST_CASES}
)

target_include_directories(MLIMTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/catch2
)

target_compile_definitions(MLIMTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        CATCH_AMALGAMATED_CUSTOM_MAIN
)

target_link_libraries(MLIMTests
    PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

include(CTest)
add_test(NAME MLIMTests COMMAND MLIMTests)
