add_library(nle_servers STATIC
    rendering/rendering_server.cpp
    rendering/null/null_rendering_device.cpp
)

target_include_directories(nle_servers
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_features(nle_servers PUBLIC cxx_std_20)
target_link_libraries(nle_servers PUBLIC nle_core)
nle_configure_target(nle_servers)
