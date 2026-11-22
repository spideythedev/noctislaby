#pragma once

#include <stdint.h>

#if defined(_WIN32)
    #if defined(NLE_BUILDING_LIBRARY)
        #define NLE_API __declspec(dllexport)
    #else
        #define NLE_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define NLE_API __attribute__((visibility("default")))
#else
    #define NLE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NLE_C_API_VERSION 1u

typedef struct NLE_Version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} NLE_Version;

typedef enum NLE_Status {
    NLE_STATUS_OK = 0,
    NLE_STATUS_INVALID_PARAMETER = 1
} NLE_Status;

NLE_API uint32_t nle_get_api_version(void);
NLE_API NLE_Status nle_get_engine_version(NLE_Version* version);
NLE_API const char* nle_get_engine_name(void);

#ifdef __cplusplus
}
#endif
