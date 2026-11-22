#include "engine_main.h"

#include "nle/engine_config.h"
#include "logging/logger.h"

#include <string>

namespace nle {

int EngineMain::run() {
    Logger::instance().info(std::string(NLE_ENGINE_NAME) + " " + NLE_VERSION_STRING);
    Logger::instance().info("Engine startup complete.");

    return 0;
}

} // namespace nle
