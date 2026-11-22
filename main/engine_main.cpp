#include "engine_main.h"

#include "nle/engine_config.h"
#include "logging/logger.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/null/null_rendering_device.h"
#include "servers/rendering/rendering_server.h"

#include <memory>
#include <string>

namespace nle {

int EngineMain::run() {
    Logger::instance().info(std::string(NLE_ENGINE_NAME) + " " + NLE_VERSION_STRING);

    RenderingServer rendering_server;
    const ErrorCode rendering_error = rendering_server.initialize(
        std::make_unique<NullRenderingDevice>()
    );
    if (rendering_error != ErrorCode::Ok) {
        Logger::instance().error("Failed to initialize rendering device.");
        return 1;
    }

    Logger::instance().info(
        "Rendering device: " + std::string(rendering_server.get_device_info().name)
    );

    SceneTree scene_tree;
    if (!scene_tree.initialize()) {
        Logger::instance().error("Failed to initialize the scene tree.");
        rendering_server.shutdown();
        return 1;
    }

    Logger::instance().info("Scene tree initialized.");
    scene_tree.shutdown();
    rendering_server.shutdown();
    Logger::instance().info("Engine shutdown complete.");

    return 0;
}

} // namespace nle
