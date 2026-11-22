#include "engine_main.h"

#include "nle/engine_config.h"
#include "logging/logger.h"
#include "scene/main/scene_tree.h"

#include <string>

namespace nle {

int EngineMain::run() {
    Logger::instance().info(std::string(NLE_ENGINE_NAME) + " " + NLE_VERSION_STRING);

    SceneTree scene_tree;
    if (!scene_tree.initialize()) {
        Logger::instance().error("Failed to initialize the scene tree.");
        return 1;
    }

    Logger::instance().info("Scene tree initialized.");
    scene_tree.shutdown();
    Logger::instance().info("Engine shutdown complete.");

    return 0;
}

} // namespace nle
