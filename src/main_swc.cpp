/**
 * @file main_swc.cpp
 * @brief Application entry point — AUTOSAR-inspired SWC architecture.
 *
 * This is the new entry point that uses the Application orchestrator.
 * The original main.cpp is preserved as main_legacy.cpp for reference.
 */

#include "app/Application.hpp"
#include "core/Logger.hpp"

int main() {
    // Optional: set log level
    // adas::Logger::setGlobalLevel(adas::LogLevel::DEBUG);

    adas::Application app;

    if (!app.init()) {
        return -1;
    }

    app.run();
    app.shutdown();

    return 0;
}
