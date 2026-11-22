add_executable(nle_runtime
    main.cpp
    engine_main.cpp
)

target_link_libraries(nle_runtime PRIVATE nle_engine nle_scene nle_servers)

target_include_directories(nle_runtime
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/../generated"
)

nle_configure_target(nle_runtime)
