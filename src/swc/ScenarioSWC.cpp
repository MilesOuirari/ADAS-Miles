#include "swc/ScenarioSWC.hpp"
#include <cstdlib>
#include <cmath>

namespace adas {

// ═══════════════════════════════════════════════════════════════════════════
//  Construction
// ═══════════════════════════════════════════════════════════════════════════

ScenarioSWC::ScenarioSWC()
    : ComponentBase("ScenarioSWC") {}

// ═══════════════════════════════════════════════════════════════════════════
//  Lifecycle Hooks
// ═══════════════════════════════════════════════════════════════════════════

bool ScenarioSWC::onInit() {
    buildScenarios();
    log_.info("Loaded " + std::to_string(scenarios_.size()) + " scenarios");
    return true;
}

bool ScenarioSWC::onConfigure() {
    // Initial ego state output
    ego_speed_ = scenarios_[current_scenario].initial_speed;
    return true;
}

void ScenarioSWC::onStep(float dt) {
    // --- Scenario auto-cycle ---
    scenario_timer_ += dt;
    if (auto_cycle && scenario_timer_ > cycle_duration_) {
        nextScenario();
    }

    const auto& cfg = getCurrentConfig();

    // --- Ego physics update ---
    float32 speed_mps = ego_speed_ / 3.6f;
    ego_world_y_ += speed_mps * dt;

    // --- Publish ScenarioConfig ---
    port_scenario_config.write(cfg);

    // --- Publish EgoState ---
    EgoState ego;
    ego.speed_kph        = ego_speed_;
    ego.speed_limit      = cfg.speed_limit;
    ego.autopilot_engaged = true;
    ego.scenario_index   = current_scenario;
    ego.world_y          = ego_world_y_;
    ego.lane_x           = ego_lane_center_;
    port_ego_state.write(ego);
}

void ScenarioSWC::onShutdown() {
    scenarios_.clear();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Public Interface
// ═══════════════════════════════════════════════════════════════════════════

void ScenarioSWC::nextScenario() {
    current_scenario = (current_scenario + 1) % static_cast<int>(scenarios_.size());
    scenario_timer_ = 0.0f;
    log_.info(">>> Scenario: " + getCurrentConfig().name);
}

void ScenarioSWC::prevScenario() {
    current_scenario = (current_scenario - 1 + static_cast<int>(scenarios_.size()))
                       % static_cast<int>(scenarios_.size());
    scenario_timer_ = 0.0f;
    log_.info(">>> Scenario: " + getCurrentConfig().name);
}

void ScenarioSWC::setScenario(int idx) {
    if (idx >= 0 && idx < static_cast<int>(scenarios_.size())) {
        current_scenario = idx;
        scenario_timer_ = 0.0f;
        log_.info(">>> Scenario: " + getCurrentConfig().name);
    }
}

void ScenarioSWC::toggleAutoCycle() {
    auto_cycle = !auto_cycle;
    log_.info("Auto-cycle: " + std::string(auto_cycle ? "ON" : "OFF"));
}

const ScenarioConfig& ScenarioSWC::getCurrentConfig() const {
    return scenarios_[current_scenario];
}

// ═══════════════════════════════════════════════════════════════════════════
//  Ego State Modifiers (called by Application after planner output)
// ═══════════════════════════════════════════════════════════════════════════

// These are updated by the Application orchestrator after the Planner step:
// ego_speed_, ego_lane_center_ are public via port reads.

// ═══════════════════════════════════════════════════════════════════════════
//  Scenario Definitions
// ═══════════════════════════════════════════════════════════════════════════

void ScenarioSWC::buildScenarios() {
    scenarios_ = {
        {"Highway Cruise", ScenarioType::HIGHWAY_CRUISE,
         120.0f, 120.0f, 3, false, 0.05f,
         "High-speed highway driving with sparse traffic"},

        {"City Driving", ScenarioType::CITY_DRIVING,
         50.0f, 50.0f, 8, true, 0.2f,
         "Urban environment with dense traffic and intersections"},

        {"Emergency Brake", ScenarioType::EMERGENCY_BRAKE,
         80.0f, 80.0f, 4, false, 0.0f,
         "Lead vehicle suddenly brakes - AEB test"},

        {"Lane Change", ScenarioType::LANE_CHANGE,
         100.0f, 120.0f, 6, false, 0.1f,
         "Slow vehicle ahead requires lane change"},

        {"Traffic Jam", ScenarioType::TRAFFIC_JAM,
         15.0f, 50.0f, 10, false, 0.0f,
         "Stop-and-go traffic congestion"},

        {"Pedestrian Crossing", ScenarioType::PEDESTRIAN_CROSSING,
         40.0f, 50.0f, 5, false, 0.0f,
         "Pedestrian crosses the road ahead"},

        {"Intersection", ScenarioType::INTERSECTION,
         50.0f, 50.0f, 6, true, 0.0f,
         "Navigate through traffic light intersection"},

        {"Highway Merge", ScenarioType::HIGHWAY_MERGE,
         100.0f, 120.0f, 7, false, 0.15f,
         "Vehicles merging from on-ramp"}
    };
}

} // namespace adas
