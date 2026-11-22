find_package(Threads REQUIRED)

add_executable(nle_tests
    test_main.cpp
)

target_link_libraries(nle_tests PRIVATE nle_scene nle_servers Threads::Threads)
nle_configure_target(nle_tests)

add_test(NAME nle_core_tests COMMAND nle_tests)

add_executable(nle_c_api_tests
    c_api_smoke.c
)

target_link_libraries(nle_c_api_tests PRIVATE nle_engine)
nle_configure_target(nle_c_api_tests)

add_test(NAME nle_c_api_tests COMMAND nle_c_api_tests)
