#ifndef ADAS_APP_APPLICATION_HPP
#define ADAS_APP_APPLICATION_HPP

/**
 * @file Application.hpp
 * @brief AUTOSAR RTE-inspired Application Orchestrator.
 *
 * Owns all SWCs, the SignalBus, and the main loop.
 * Manages lifecycle transitions: init → configure → run → shutdown.
 */

#include "core/IComponent.hpp"
#include "core/SignalBus.hpp"
#include "core/Logger.hpp"

#include <vector>
#include <memory>
#include <string>

struct GLFWwindow;

namespace adas {

// Forward declarations
class ScenarioSWC;
class EnvironmentModelSWC;
class PlannerSWC;
class RenderingSWC;
class HudSWC;

class Application {
public:
    Application();
    ~Application();

    // Non-copyable, non-movable
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * @brief Initialize GLFW, create window, instantiate all SWCs.
     * @return true on success.
     */
    bool init();

    /**
     * @brief Run the main event loop (blocking).
     *
     * Calls step() on each SWC in dependency order every frame.
     * Returns when the window is closed.
     */
    void run();

    /**
     * @brief Shutdown all SWCs and destroy the window.
     */
    void shutdown();

private:
    // ─── Window ────────────────────────────────────────────────────────
    GLFWwindow* window_ = nullptr;

    // ─── Signal Bus ────────────────────────────────────────────────────
    SignalBus bus_;

    // ─── SWCs (typed pointers for direct access during wiring) ────────
    ScenarioSWC*          scenario_   = nullptr;
    EnvironmentModelSWC*  env_model_  = nullptr;
    PlannerSWC*           planner_    = nullptr;
    RenderingSWC*         rendering_  = nullptr;
    HudSWC*               hud_        = nullptr;

    // ─── Component Registry ────────────────────────────────────────────
    std::vector<IComponent*> components_;  ///< Execution order

    // ─── Infrastructure ────────────────────────────────────────────────
    Logger log_;

    // ─── Internal ──────────────────────────────────────────────────────
    bool createWindow();
    void registerPorts();
    void connectPorts();

    static void glfwErrorCallback(int error, const char* description);
    static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};

} // namespace adas

#endif // ADAS_APP_APPLICATION_HPP
