add_library(nle_scene STATIC
    2d/node_2d.cpp
    3d/node_3d.cpp
    main/node.cpp
    main/scene_tree.cpp
)

target_include_directories(nle_scene
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

target_compile_features(nle_scene PUBLIC cxx_std_20)
target_link_libraries(nle_scene PUBLIC nle_core)
nle_configure_target(nle_scene)
