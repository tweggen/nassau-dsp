cmake_minimum_required(VERSION 3.15)
project(NassauDSP VERSION 0.1.0 LANGUAGES CXX)

# nassau-dsp: shared, header-only DSP primitives of the Nassau family.
#
# Standard C++17 headers only (R2): no Qt, no IPlug2, no plugin SDK, and no
# find_package of anything. A consumer finds this directory through the
# locator cmake/bootstrap/NassauDSP.cmake (copied verbatim into it), which
# add_subdirectory()s it, and links the INTERFACE target nassau::dsp. It is
# never a nested submodule of a consumer; nassau-suite pins it once.
#
# The headers keep their nassau-analogue names (synth_dsp.h, synth_filter.h)
# so that nassau-analogue's own sources and tests include them unchanged.
#
# Two compile definitions change what the headers compile to and must agree
# across every translation unit of a consumer (an ODR matter, which is why a
# consumer sets them directory-wide, not here): NASSAU_DSP_FLOAT (nassau_real
# becomes float) and NASSAU_NO_SIMD (read by nassau-analogue's synth_simd.h).

add_library(nassau_dsp INTERFACE)
add_library(nassau::dsp ALIAS nassau_dsp)
target_include_directories(nassau_dsp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP)
target_compile_features(nassau_dsp INTERFACE cxx_std_17)

# Tests only when this is the top-level project: a consumer that adds the
# submodule gets the target and nothing else (no test executables in its
# build, no entries in its ctest).
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    enable_testing()
    add_subdirectory(Tests)
endif()
