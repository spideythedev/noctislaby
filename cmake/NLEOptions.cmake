option(NLE_BUILD_TESTS "Build NoctisLaby tests" ON)
option(NLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(NLE_ENABLE_ASSERTS "Enable engine runtime assertions" ON)

message(STATUS "NoctisLaby Engine ${PROJECT_VERSION}")
message(STATUS "Build tests: ${NLE_BUILD_TESTS}")
message(STATUS "Warnings as errors: ${NLE_WARNINGS_AS_ERRORS}")

option(NLE_DOUBLE_PRECISION "Use double precision engine math" OFF)
