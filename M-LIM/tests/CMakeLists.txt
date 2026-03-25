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
    dsp/test_limiter_engine.cpp
    dsp/test_limiter_engine_modes.cpp
)

# Catch2 test executable — DSP sources come from MLIMLib
add_executable(MLIMTests
    test_main.cpp
    catch2/catch_amalgamated.cpp
    ${MLIM_TEST_CASES}
    dsp/test_limiter_algorithm.cpp
)

target_include_directories(MLIMTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/catch2
)

target_compile_definitions(MLIMTests
    PRIVATE
        CATCH_AMALGAMATED_CUSTOM_MAIN
)

target_link_libraries(MLIMTests
    PRIVATE
        MLIMLib
        asound
)

include(CTest)
add_test(NAME MLIMTests COMMAND MLIMTests)
