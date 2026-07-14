add_executable(parse_measurements_tests 
    parse_measurements_test.cpp
)
target_link_libraries(parse_measurements_tests 
    GTest::gtest
    GTest::gtest_main
    mateligent_stretch_sensor_lib
)

add_test(NAME parse_measurements_tests COMMAND parse_measurements_tests)
gtest_discover_tests(parse_measurements_tests)