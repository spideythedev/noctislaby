add_library(nle_core STATIC
    error/error.cpp
    io/resource.cpp
    logging/logger.cpp
    memory/memory.cpp
    object/class_db.cpp
    object/object.cpp
    object/object_db.cpp
    string/string_name.cpp
    variant/variant.cpp
)

target_include_directories(nle_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/../generated"
)

set_target_properties(nle_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_features(nle_core PUBLIC cxx_std_20)

if(NLE_DOUBLE_PRECISION)
    target_compile_definitions(nle_core PUBLIC NLE_DOUBLE_PRECISION)
endif()

nle_configure_target(nle_core)
