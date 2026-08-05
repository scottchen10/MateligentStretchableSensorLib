cmake_minimum_required(VERSION 4.0)

option(BUILD_TESTS OFF)

project(mateligent_stretch_sensor_lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(mateligent_stretch_sensor_lib 
    STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/stretchable_sensor_parser.cpp
)

target_include_directories(mateligent_stretch_sensor_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(external/etl)

target_link_libraries(mateligent_stretch_sensor_lib
    PRIVATE
        etl::etl
    )

if (BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    include(GoogleTest)
    
    add_subdirectory(tests)
endif()