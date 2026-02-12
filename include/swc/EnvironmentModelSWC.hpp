#ifndef ADAS_SWC_ENVIRONMENT_MODEL_SWC_HPP
#define ADAS_SWC_ENVIRONMENT_MODEL_SWC_HPP

/**
 * @file EnvironmentModelSWC.hpp
 * @brief Realistic traffic simulation using IDM + MOBIL models.
 *
 * Core physics:
 *   - IDM (Intelligent Driver Model) for car-following
 *   - MOBIL for lane-change decision making
 *   - Persistent traffic zone management
 *   - All internal units in SI (meters, m/s)
 */

#include "core/ComponentBase.hpp"
#include "core/Port.hpp"
#include "data/DataModels.hpp"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace adas {

// ═══════════════════════════════════════════════════════════════════════════
//  Road Geometry Constants
// ═══════════════════════════════════════════════════════════════════════════

constexpr float32 kLaneWidth      = 3.5f;   ///< Meters per lane
constexpr int     kNumLanes       = 4;
/// 4 lanes on 14m road (from -7 to +7):
///   Lane 0 (far left):  -5.25
///   Lane 1 (center-L):  -1.75
///   Lane 2 (center-R):   1.75   ← ego default (keep-right)
///   Lane 3 (far right):  5.25
constexpr float32 kLaneCenter[4]  = { -5.25f, -1.75f, 1.75f, 5.25f };
constexpr int     kEgoDefaultLane = 2;  ///< Ego stays on right side

// ═══════════════════════════════════════════════════════════════════════════
//  Traffic Agent — the fundamental simulated vehicle
// ═══════════════════════════════════════════════════════════════════════════

struct TrafficAgent {
    int     id          = 0;

    // --- Position (absolute world coordinates, meters) ---
    float32 world_x     = 0.0f;   ///< Lateral position
    float32 world_y     = 0.0f;   ///< Longitudinal position (forward)

    // --- Dynamics (all in m/s) ---
    float32 speed       = 0.0f;   ///< Current speed (m/s)
    float32 desired_speed = 0.0f; ///< Free-flow target speed (m/s)

    // --- IDM parameters (per-agent for realism) ---
    float32 idm_s0      = 2.0f;   ///< Minimum gap (m)
    float32 idm_T       = 1.5f;   ///< Desired time headway (s)
    float32 idm_a       = 1.4f;   ///< Maximum acceleration (m/s²)
    float32 idm_b       = 2.0f;   ///< Comfortable deceleration (m/s²)
    static constexpr float32 IDM_DELTA = 4.0f; ///< Acceleration exponent

    // --- Lane state ---
    int     lane        = kEgoDefaultLane;  ///< Current lane index (0-3)
    int     target_lane = kEgoDefaultLane;  ///< Lane being merged into
    float32 lane_change_progress = 0.0f;    ///< 0.0 = start, 1.0 = complete
    boolean is_changing_lane = false;

    // --- Vehicle type ---
    int     class_id    = 0;   ///< 0=Car, 1=Bus, 2=Truck, 3=Bike, 4=Pedestrian
    float32 length      = 4.5f;

    // ─── IDM: Compute acceleration given gap and leader speed ───────────
    float32 idmAcceleration(float32 gap, float32 leader_speed) const {
        // Free-road term: want to reach desired_speed
        float32 v_ratio = speed / std::max(desired_speed, 0.1f);
        float32 free_road = 1.0f - std::pow(v_ratio, IDM_DELTA);

        // Interaction term: want to maintain safe distance
        float32 delta_v = speed - leader_speed;
        float32 s_star = idm_s0
                       + std::max(0.0f, speed * idm_T + (speed * delta_v) / (2.0f * std::sqrt(idm_a * idm_b)));
        float32 interaction = (gap > 0.1f) ? std::pow(s_star / gap, 2.0f) : 1.0f;

        return idm_a * (free_road - interaction);
    }

    // ─── Get current world_x based on lane and transition ──────────────
    float32 effectiveX() const {
        int l = std::clamp(lane, 0, kNumLanes - 1);
        if (!is_changing_lane) return kLaneCenter[l];
        int tl = std::clamp(target_lane, 0, kNumLanes - 1);
        float32 t = lane_change_progress;
        float32 smooth = t * t * (3.0f - 2.0f * t);
        return kLaneCenter[l] + (kLaneCenter[tl] - kLaneCenter[l]) * smooth;
    }
};

class EnvironmentModelSWC : public ComponentBase {
public:
    EnvironmentModelSWC();

    // ─── Ports ─────────────────────────────────────────────────────────
    SenderPort<ObjectList>       port_object_list;
    SenderPort<LaneNetwork>      port_lane_network;
    SenderPort<TrafficSignList>  port_traffic_signs;
    ReceiverPort<EgoState>       port_ego_state;
    ReceiverPort<ScenarioConfig> port_scenario_config;

    // ─── Public accessors for Planner ──────────────────────────────────
    const std::vector<TrafficAgent>& agents() const { return agents_; }
    TrafficLightState lightState() const { return light_state_; }
    float32 stopLineY() const { return stop_line_y_; }
    const ScenarioConfig& getScenarioConfig() { return port_scenario_config.read(); }

protected:
    bool onInit() override;
    bool onConfigure() override;
    void onStep(float dt) override;
    void onShutdown() override;

private:
    std::vector<TrafficAgent> agents_;
    int next_id_ = 1;

    // Traffic light
    float32 stop_line_y_  = 500.0f;
    float32 light_timer_  = 0.0f;
    TrafficLightState light_state_ = TrafficLightState::GREEN;

    // Scenario tracking
    float32 scenario_timer_ = 0.0f;
    ScenarioType last_scenario_ = ScenarioType::HIGHWAY_CRUISE;
    boolean scenario_initialized_ = false;
    int spawn_timer_ = 0;   ///< Counter for continuous horizon spawning

    // ─── Core simulation steps ─────────────────────────────────────────
    void updateTrafficLights(float32 dt);
    void updateIDM(float32 dt, float32 ego_y, float32 ego_x);
    void updateLaneChanges(float32 dt);
    void cleanupAndSpawn(float32 ego_y, float32 speed_limit_mps);
    void applyScenarioModifiers(const ScenarioConfig& cfg, float32 ego_y, float32 dt);

    // ─── Spawning ──────────────────────────────────────────────────────
    TrafficAgent spawnAgent(float32 y_pos, float32 speed_limit_mps);
    void ensureMinimumTraffic(float32 ego_y, float32 speed_limit_mps, int target_count);

    // ─── Leader finding ────────────────────────────────────────────────
    struct LeaderInfo {
        float32 gap    = 999.0f;
        float32 speed  = 30.0f;   ///< m/s
        boolean found  = false;
    };
    LeaderInfo findLeader(const TrafficAgent& agent, float32 ego_y, float32 ego_speed) const;

    // ─── MOBIL lane change logic ───────────────────────────────────────
    boolean shouldChangeLane(const TrafficAgent& agent, int new_lane,
                             float32 ego_y, float32 ego_speed) const;

    // ─── Output builders ───────────────────────────────────────────────
    void buildObjectList(float32 ego_y, float32 ego_x, float32 curve_val);
    void buildLaneNetwork(float32 ego_y, float32 ego_x, float32 curve_val);
    void buildTrafficSigns();
};

} // namespace adas

#endif // ADAS_SWC_ENVIRONMENT_MODEL_SWC_HPP
