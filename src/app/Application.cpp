#include "app/Application.hpp"

#include "swc/ScenarioSWC.hpp"
#include "swc/EnvironmentModelSWC.hpp"
#include "swc/PlannerSWC.hpp"
#include "swc/RenderingSWC.hpp"
#include "swc/HudSWC.hpp"
#include "data/DataModels.hpp"

#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

namespace adas {

// ═══════════════════════════════════════════════════════════════════════════
//  Construction / Destruction
// ═══════════════════════════════════════════════════════════════════════════

Application::Application() : log_("Application") {}

Application::~Application() {
    shutdown();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Initialization
// ═══════════════════════════════════════════════════════════════════════════

bool Application::init() {
    log_.info("═══════════════════════════════════════════════");
    log_.info("  ADAS-Miles Level 4 Autonomous Pilot");
    log_.info("  Architecture: AUTOSAR-inspired SWC Model");
    log_.info("═══════════════════════════════════════════════");

    // 1. Create Window
    if (!createWindow()) {
        log_.fatal("Failed to create GLFW window");
        return false;
    }

    // 2. Instantiate SWCs
    log_.info("Creating Software Components...");
    scenario_  = new ScenarioSWC();
    env_model_ = new EnvironmentModelSWC();
    planner_   = new PlannerSWC();
    rendering_ = new RenderingSWC(window_);
    hud_       = new HudSWC();

    // Dependency injection
    planner_->setEnvironmentModel(env_model_);

    // Execution order (respects data dependencies)
    components_ = { scenario_, env_model_, planner_, rendering_, hud_ };

    // 3. Init all
    log_.info("Phase: INIT");
    for (auto* comp : components_) {
        if (!comp->init()) {
            log_.fatal("Failed to init: " + comp->name());
            return false;
        }
    }

    // 4. Register sender ports on the bus
    registerPorts();

    // 5. Connect receiver ports
    connectPorts();

    // 6. Configure all
    log_.info("Phase: CONFIGURE");
    for (auto* comp : components_) {
        if (!comp->configure()) {
            log_.fatal("Failed to configure: " + comp->name());
            return false;
        }
    }

    log_.info("All components ready — entering RUN phase");
    log_.info("Controls: [N/Right] Next | [P/Left] Prev | [1-8] Select | [Space] Toggle Auto");

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Main Loop
// ═══════════════════════════════════════════════════════════════════════════

void Application::run() {
    const float32 dt = 0.016f;   // ~60 FPS fixed timestep
    int frame = 0;

    while (!glfwWindowShouldClose(window_)) {
        // Step all SWCs in dependency order
        for (auto* comp : components_) {
            comp->step(dt);
        }

        // --- Controller: apply planner output to ego state ---
        // Planner outputs in m/s. ScenarioSWC stores in km/h.
        const auto& traj = planner_->port_best_trajectory.slot()->read();
        if (traj.valid && traj.point_count > 1) {
            // Lateral control: smooth convergence toward planner's path
            float32 target_x = traj.points[5].x;
            float32 diff = target_x - scenario_->ego_lane_center_;
            scenario_->ego_lane_center_ += diff * 0.15f; // Stronger gain for decisive lane changes

            // Longitudinal control: target_speed is in m/s, ego_speed_ is in km/h
            float32 target_kph = traj.target_speed * 3.6f;
            float32 v_diff = target_kph - scenario_->ego_speed_;
            scenario_->ego_speed_ += v_diff * 0.02f;
            if (scenario_->ego_speed_ < 0) scenario_->ego_speed_ = 0;
        }

        // Note: ego_world_y_ is advanced inside ScenarioSWC::onStep()

        // Debug output (every 60 frames = ~1 per second)
        if (frame++ % 60 == 0) {
            const auto& decision = planner_->port_decision_text.slot()->read();
            log_.info("AI Decision: " + decision +
                      " | Speed: " + std::to_string(static_cast<int>(scenario_->ego_speed_)) + " km/h" +
                      " | Agents: " + std::to_string(env_model_->agents().size()));
        }

        glfwSwapBuffers(window_);
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Shutdown
// ═══════════════════════════════════════════════════════════════════════════

void Application::shutdown() {
    // Shutdown SWCs in reverse order
    log_.info("Phase: SHUTDOWN");
    for (auto it = components_.rbegin(); it != components_.rend(); ++it) {
        (*it)->shutdown();
    }

    // Delete SWCs
    delete hud_;       hud_ = nullptr;
    delete rendering_; rendering_ = nullptr;
    delete planner_;   planner_ = nullptr;
    delete env_model_; env_model_ = nullptr;
    delete scenario_;  scenario_ = nullptr;
    components_.clear();

    // Destroy window
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
    log_.info("Shutdown complete");
}

// ═══════════════════════════════════════════════════════════════════════════
//  Window Creation
// ═══════════════════════════════════════════════════════════════════════════

bool Application::createWindow() {
    if (!glfwInit()) return false;
    glfwSetErrorCallback(glfwErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(1280, 720,
        "Level 4 Autonomous Pilot (AUTOSAR Architecture)", nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    // Set user pointer so key callback can access the Application
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, glfwKeyCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return false;

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Port Wiring
// ═══════════════════════════════════════════════════════════════════════════

void Application::registerPorts() {
    log_.info("Registering sender ports on SignalBus...");

    bus_.registerSender<ScenarioConfig>(signal::SCENARIO_CONFIG,  scenario_->port_scenario_config);
    bus_.registerSender<EgoState>      (signal::EGO_STATE,        scenario_->port_ego_state);
    bus_.registerSender<ObjectList>    (signal::OBJECT_LIST,      env_model_->port_object_list);
    bus_.registerSender<LaneNetwork>   (signal::LANE_NETWORK,     env_model_->port_lane_network);
    bus_.registerSender<TrafficSignList>(signal::TRAFFIC_SIGNS,   env_model_->port_traffic_signs);
    bus_.registerSender<Trajectory>    (signal::BEST_TRAJECTORY,  planner_->port_best_trajectory);
    bus_.registerSender<std::string>   (signal::PLANNER_DECISION, planner_->port_decision_text);

    log_.info("Registered " + std::to_string(bus_.signalCount()) + " signals");
}

void Application::connectPorts() {
    log_.info("Connecting receiver ports...");

    // EnvironmentModelSWC reads ego state and scenario config
    bus_.connectReceiver<EgoState>      (signal::EGO_STATE,       env_model_->port_ego_state);
    bus_.connectReceiver<ScenarioConfig>(signal::SCENARIO_CONFIG, env_model_->port_scenario_config);

    // PlannerSWC reads ego state
    bus_.connectReceiver<EgoState>      (signal::EGO_STATE,       planner_->port_ego_state);

    // RenderingSWC reads everything
    bus_.connectReceiver<EgoState>         (signal::EGO_STATE,       rendering_->port_ego_state);
    bus_.connectReceiver<ObjectList>       (signal::OBJECT_LIST,     rendering_->port_object_list);
    bus_.connectReceiver<LaneNetwork>      (signal::LANE_NETWORK,    rendering_->port_lane_network);
    bus_.connectReceiver<TrafficSignList>  (signal::TRAFFIC_SIGNS,   rendering_->port_traffic_signs);
    bus_.connectReceiver<Trajectory>       (signal::BEST_TRAJECTORY, rendering_->port_best_trajectory);
    bus_.connectReceiver<ScenarioConfig>   (signal::SCENARIO_CONFIG, rendering_->port_scenario_config);

    // HudSWC reads ego state and objects
    bus_.connectReceiver<EgoState>    (signal::EGO_STATE,    hud_->port_ego_state);
    bus_.connectReceiver<ObjectList>  (signal::OBJECT_LIST,  hud_->port_object_list);

    log_.info("All ports connected");
}

// ═══════════════════════════════════════════════════════════════════════════
//  GLFW Callbacks
// ═══════════════════════════════════════════════════════════════════════════

void Application::glfwErrorCallback(int /*error*/, const char* description) {
    std::cerr << "GLFW Error: " << description << std::endl;
}

void Application::glfwKeyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app || !app->scenario_) return;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_N)
            app->scenario_->nextScenario();
        if (key == GLFW_KEY_LEFT || key == GLFW_KEY_P)
            app->scenario_->prevScenario();
        if (key == GLFW_KEY_SPACE)
            app->scenario_->toggleAutoCycle();
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8)
            app->scenario_->setScenario(key - GLFW_KEY_1);
    }
}

} // namespace adas
