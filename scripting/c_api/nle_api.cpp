#include "nle_api.h"

#include "nle/engine_config.h"

extern "C" {

uint32_t nle_get_api_version(void) {
    return NLE_C_API_VERSION;
}

NLE_Status nle_get_engine_version(NLE_Version* version) {
    if (version == nullptr) {
        return NLE_STATUS_INVALID_PARAMETER;
    }

    version->major = NLE_VERSION_MAJOR;
    version->minor = NLE_VERSION_MINOR;
    version->patch = NLE_VERSION_PATCH;

    return NLE_STATUS_OK;
}

const char* nle_get_engine_name(void) {
    return NLE_ENGINE_NAME;
}

}
