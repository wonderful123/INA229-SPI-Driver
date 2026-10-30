cmake_minimum_required(VERSION 3.14)
project(INA229_SPI_Driver LANGUAGES CXX)

add_library(ina229_lib src/INA229.cpp src/Config.cpp)

target_include_directories(ina229_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# The portable driver requires C++17; tests also exercise the C++20 API.
target_compile_features(ina229_lib PUBLIC cxx_std_17)

add_library(INA229::Driver ALIAS ina229_lib)

include(CTest)
if(BUILD_TESTING)
    find_package(GTest 1.10 CONFIG QUIET)
    if(NOT TARGET GTest::gmock)
        include(FetchContent)
        FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723 # v1.17.0
        )
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    add_executable(ina229_tests
        tests/test_main.cpp
        tests/test_INA229_Config.cpp
        tests/test_INA229_DataUtility.cpp
        tests/test_INA229_Measurements.cpp
        tests/test_INA229_Regression.cpp
        tests/test_ArduinoSPI.cpp
    )
    target_include_directories(ina229_tests PRIVATE tests/arduino)
    target_compile_features(ina229_tests PRIVATE cxx_std_20)
    target_link_libraries(ina229_tests PRIVATE INA229::Driver GTest::gmock)
    add_executable(ina229_cxx17_smoke tests/test_Cxx17.cpp)
    target_link_libraries(ina229_cxx17_smoke PRIVATE INA229::Driver)
    set_target_properties(ina229_cxx17_smoke PROPERTIES
        CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    add_test(NAME Cxx17Compatibility COMMAND ina229_cxx17_smoke)
    include(GoogleTest)
    gtest_discover_tests(ina229_tests)
endif()
