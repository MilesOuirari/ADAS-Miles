#ifndef ADAS_SWC_SCENARIO_SWC_HPP
#define ADAS_SWC_SCENARIO_SWC_HPP

/**
 * @file ScenarioSWC.hpp
 * @brief Software Component for driving scenario management.
 *
 * Manages a set of predefined driving scenarios (highway, city, emergency, etc.)
 * and publishes the active ScenarioConfig via SenderPort.
 */

#include "core/ComponentBase.hpp"
#include "core/Port.hpp"
#include "data/DataModels.hpp"
#include <vector>

namespace adas {

class ScenarioSWC : public ComponentBase {
public:
    ScenarioSWC();

    // ─── Ports ─────────────────────────────────────────────────────────
    SenderPort<ScenarioConfig>   port_scenario_config;   ///< Current scenario output
    SenderPort<EgoState>         port_ego_state;         ///< Ego vehicle state output

    // ─── Public Interface ──────────────────────────────────────────────
    void nextScenario();
    void prevScenario();
    void setScenario(int idx);
    void toggleAutoCycle();

    /// @return Current scenario configuration
    const ScenarioConfig& getCurrentConfig() const;

    // ─── Public State (for keyboard callback + controller access) ─────
    int  current_scenario = 0;
    bool auto_cycle       = true;

    // Ego vehicle physics state (written by Application controller)
    float32 ego_world_y_     = 0.0f;
    float32 ego_speed_       = 60.0f;
    float32 ego_lane_center_ = 1.75f;   // Lane 2 (right side, keep-right rule)

protected:
    bool onInit() override;
    bool onConfigure() override;
    void onStep(float dt) override;
    void onShutdown() override;

private:
    std::vector<ScenarioConfig> scenarios_;
    float32 scenario_timer_  = 0.0f;
    float32 cycle_duration_  = 20.0f;

    void buildScenarios();
};

} // namespace adas

#endif // ADAS_SWC_SCENARIO_SWC_HPP
