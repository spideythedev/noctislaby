add_executable(nle_tests
    test_main.cpp
)

target_link_libraries(nle_tests PRIVATE nle_core)
nle_configure_target(nle_tests)

add_test(NAME nle_core_tests COMMAND nle_tests)
