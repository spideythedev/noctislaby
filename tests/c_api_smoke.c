#include "nle_api.h"

int main(void) {
    NLE_Version version = {0, 0, 0};

    if (nle_get_api_version() != NLE_C_API_VERSION) {
        return 1;
    }

    if (nle_get_engine_version(&version) != NLE_STATUS_OK) {
        return 2;
    }

    if (version.major != 0 || version.minor != 1 || version.patch != 0) {
        return 3;
    }

    if (nle_get_engine_version(0) != NLE_STATUS_INVALID_PARAMETER) {
        return 4;
    }

    if (nle_get_engine_name() == 0) {
        return 5;
    }

    return 0;
}
