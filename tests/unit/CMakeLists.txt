cmake_minimum_required(VERSION 3.14)
project(midi_controller_unit_tests CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(MAIN_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../main/include)
set(STUBS       ${CMAKE_CURRENT_SOURCE_DIR}/stubs)

add_executable(test_controller
    test_controller.cpp
    ../../main/src/controller.cpp
)

target_include_directories(test_controller PRIVATE
    ${MAIN_INCLUDE}
    ${STUBS}
)

target_link_libraries(test_controller PRIVATE GTest::gtest_main)

if(MSVC)
    target_compile_options(test_controller PRIVATE /utf-8)
endif()

include(GoogleTest)
gtest_discover_tests(test_controller)
