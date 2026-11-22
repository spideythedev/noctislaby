add_library(nle_core STATIC
    error/error.cpp
    logging/logger.cpp
    object/object.cpp
    object/object_db.cpp
    string/string_name.cpp
)

target_include_directories(nle_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/../generated"
)

target_compile_features(nle_core PUBLIC cxx_std_20)
nle_configure_target(nle_core)
