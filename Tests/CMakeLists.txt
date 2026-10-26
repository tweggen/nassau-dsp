# nassau-dsp's own suite (gate X1.2): the headers build and test with no Qt,
# no IPlug2 and no SDK. Hand-rolled harness, plain int main, soft checks.

if(MSVC)
    set(NASSAU_DSP_TEST_WARNINGS /W4)
    # Release defaults; no /fp:fast (R7).
else()
    set(NASSAU_DSP_TEST_WARNINGS -Wall -Wextra -Wpedantic)
    # -O3 in Release; no -ffast-math, no -march=native (R7).
    set(NASSAU_DSP_TEST_OPT $<$<CONFIG:Release>:-O3>)
endif()

function(nassau_dsp_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE nassau::dsp)
    target_compile_options(${name} PRIVATE ${NASSAU_DSP_TEST_WARNINGS} ${NASSAU_DSP_TEST_OPT})
endfunction()

# synth_dsp.h: nassau-analogue's G1 battery, moved with the header.
nassau_dsp_test(dsp_tests)
add_test(NAME DspTests COMMAND dsp_tests)

# synth_filter.h: slopes, resonance, determinism, the process() regions.
nassau_dsp_test(filter_tests)
target_compile_definitions(filter_tests PRIVATE
    NASSAU_FILTER_H_PATH="${PROJECT_SOURCE_DIR}/Source/DSP/synth_filter.h")
add_test(NAME FilterTests COMMAND filter_tests)

# Byte-level parity of filters + shaper against a checked-in fixture.
# Regenerate with `filter_golden generate <abs path>`: a deliberate act.
nassau_dsp_test(filter_golden)
add_test(NAME FilterGolden
         COMMAND filter_golden verify "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/filter_golden.bin")
