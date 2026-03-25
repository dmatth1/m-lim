cmake_minimum_required(VERSION 3.22)

# Collect test source files
set(MLIM_TEST_CASES
    dsp/test_dc_filter.cpp
    dsp/test_dither.cpp
    dsp/test_sidechain_filter.cpp
    dsp/test_oversampler.cpp
    dsp/test_loudness_meter.cpp
    dsp/test_transient_limiter.cpp
    dsp/test_leveling_limiter.cpp
    dsp/test_true_peak.cpp
    dsp/test_loudness_meter_accuracy.cpp
    dsp/test_dsp_edge_cases.cpp
    integration/test_dsp_components.cpp
    integration/test_parameter_state.cpp
    integration/test_plugin_processor.cpp
    integration/test_processor_stress.cpp
    dsp/test_limiter_engine.cpp
    dsp/test_limiter_engine_modes.cpp
    dsp/test_lockfree_fifo.cpp
    state/test_undo_manager.cpp
    state/test_preset_manager.cpp
    state/test_preset_manager_errors.cpp
    state/test_ab_state.cpp
)

# Catch2 test executable — DSP sources come from MLIMLib
add_executable(MLIMTests
    test_main.cpp
    catch2/catch_amalgamated.cpp
    ${MLIM_TEST_CASES}
    dsp/test_limiter_algorithm.cpp
    # Parameters.cpp is not in MLIMLib (it lives in the plugin target), so
    # include it directly here for the parameter-state integration tests.
    ../src/Parameters.cpp
    # PluginProcessor and PluginEditor — needed for processor integration tests
    ../src/PluginProcessor.cpp
    ../src/PluginEditor.cpp
)

target_include_directories(MLIMTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/catch2
)

target_compile_definitions(MLIMTests
    PRIVATE
        CATCH_AMALGAMATED_CUSTOM_MAIN
        MLIM_PRESETS_DIR="${CMAKE_SOURCE_DIR}/presets"
        # JucePlugin_Name is normally provided by juce_add_plugin; define it
        # for the standalone test binary so PluginProcessor.cpp compiles.
        JucePlugin_Name="M-LIM"
)

target_link_libraries(MLIMTests
    PRIVATE
        MLIMLib
        asound
)

include(CTest)
add_test(NAME MLIMTests COMMAND MLIMTests)
