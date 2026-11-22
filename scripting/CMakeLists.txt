add_library(nle_engine SHARED
    c_api/nle_api.cpp
)

target_include_directories(nle_engine
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/c_api"
        "${CMAKE_CURRENT_BINARY_DIR}/../generated"
)

target_compile_features(nle_engine PUBLIC cxx_std_20)
target_compile_definitions(nle_engine PRIVATE NLE_BUILDING_LIBRARY)
target_link_libraries(nle_engine PUBLIC nle_core)

set_target_properties(nle_engine PROPERTIES
    OUTPUT_NAME noctislaby
)

nle_configure_target(nle_engine)
