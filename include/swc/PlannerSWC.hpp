#ifndef ADAS_SWC_PLANNER_SWC_HPP
#define ADAS_SWC_PLANNER_SWC_HPP

/**
 * @file PlannerSWC.hpp
 * @brief Software Component for trajectory planning and decision making.
 *
 * Implements a multi-candidate trajectory planner with cost-based selection.
 * Receives EgoState + environment data, publishes the best Trajectory.
 */

#include "core/ComponentBase.hpp"
#include "core/Port.hpp"
#include "data/DataModels.hpp"
#include "swc/EnvironmentModelSWC.hpp"  // For TrafficAgent access
#include <vector>

namespace adas {

class PlannerSWC : public ComponentBase {
public:
    PlannerSWC();

    // ─── Ports ─────────────────────────────────────────────────────────
    SenderPort<Trajectory>     port_best_trajectory;
    SenderPort<std::string>    port_decision_text;
    ReceiverPort<EgoState>     port_ego_state;

    /// @brief Set reference to environment model for agent data access
    void setEnvironmentModel(EnvironmentModelSWC* env) { env_model_ = env; }

protected:
    bool onInit() override;
    bool onConfigure() override;
    void onStep(float dt) override;
    void onShutdown() override;

private:
    // Planning constants
    static constexpr float32 LANE_WIDTH          = 3.5f;
    static constexpr float32 PREDICTION_HORIZON  = 4.0f;
    static constexpr float32 DT_PLAN             = 0.1f;

    // Cost weights
    static constexpr float32 W_COLLISION         = 99999.0f;
    static constexpr float32 W_EFFICIENCY        = 1.0f;
    static constexpr float32 W_COMFORT           = 10.0f;
    static constexpr float32 W_LANE_CHANGE       = 2.0f;

    EnvironmentModelSWC* env_model_ = nullptr;

    struct InternalTrajectory {
        std::vector<float32> points_x;
        std::vector<float32> points_y;
        float32     cost         = 0.0f;
        int         target_lane  = 0;
        std::string description;
        float32     target_speed = 0.0f;
        boolean     valid        = true;
    };

    std::vector<InternalTrajectory> candidates_;
    InternalTrajectory best_path_;
    std::string decision_text_;

    void generateCandidate(int target_lane_idx, float32 v_start, float32 lat_start,
                           float32 y_start, const std::vector<TrafficAgent>& traffic,
                           TrafficLightState light, float32 stop_y, const std::string& desc);

    Trajectory toOutputTrajectory(const InternalTrajectory& internal) const;
};

} // namespace adas

#endif // ADAS_SWC_PLANNER_SWC_HPP
