cmake_minimum_required(VERSION 3.16)
project(PVTBeatTracker VERSION 1.1.0 LANGUAGES C)
include(CTest)
add_library(pvt_btt STATIC src/BTT.c src/PVTOnset.c src/DFT.c src/Filter.c
    src/STFT.c src/Statistics.c src/fastsin.c)
target_compile_features(pvt_btt PUBLIC c_std_11)
target_include_directories(pvt_btt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(pvt_btt PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_definitions(pvt_btt PRIVATE _USE_MATH_DEFINES)
else()
    target_link_libraries(pvt_btt PUBLIC m)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # The retained statistics/window helpers use POSIX random() and M_PI.
        target_compile_definitions(pvt_btt PRIVATE _DEFAULT_SOURCE)
    endif()
endif()
if(BUILD_TESTING)
    add_executable(pvt_btt_tests tests/tracker_test.c)
    target_link_libraries(pvt_btt_tests PRIVATE pvt_btt)
    add_test(NAME pvt_btt COMMAND pvt_btt_tests)
endif()
