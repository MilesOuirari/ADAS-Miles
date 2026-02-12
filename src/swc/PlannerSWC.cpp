#include "swc/PlannerSWC.hpp"
#include <cmath>
#include <algorithm>

namespace adas {

// ═══════════════════════════════════════════════════════════════════════════
//  Construction / Lifecycle
// ═══════════════════════════════════════════════════════════════════════════

PlannerSWC::PlannerSWC()
    : ComponentBase("PlannerSWC") {}

bool PlannerSWC::onInit() {
    candidates_.reserve(6);
    return true;
}

bool PlannerSWC::onConfigure() {
    return env_model_ != nullptr;
}

void PlannerSWC::onStep(float /*dt*/) {
    if (!env_model_) return;

    const auto& ego = port_ego_state.read();
    const auto& traffic = env_model_->agents();
    TrafficLightState light = env_model_->lightState();
    float32 stop_y = env_model_->stopLineY();

    float32 ego_speed = ego.speed_kph / 3.6f;  // Convert to m/s
    float32 ego_lat   = ego.lane_x;
    float32 ego_y     = ego.world_y;

    candidates_.clear();

    // Identify current discrete lane (find closest lane center)
    int lane_idx = kEgoDefaultLane;
    float32 min_lane_dist = 999.0f;
    for (int l = 0; l < kNumLanes; ++l) {
        float32 d = std::abs(ego_lat - kLaneCenter[l]);
        if (d < min_lane_dist) { min_lane_dist = d; lane_idx = l; }
    }

    // Generate candidates — ego uses lanes 1, 2, 3 (right side)
    // Lane 0 avoided (oncoming/far-left, only used in emergencies)
    generateCandidate(lane_idx, ego_speed, ego_lat, ego_y, traffic, light, stop_y, "Keep Lane");
    if (lane_idx > 1)  // Don't go into lane 0 (far left)
        generateCandidate(lane_idx - 1, ego_speed, ego_lat, ego_y, traffic, light, stop_y, "Lane Change Left");
    if (lane_idx < kNumLanes - 1)
        generateCandidate(lane_idx + 1, ego_speed, ego_lat, ego_y, traffic, light, stop_y, "Lane Change Right");

    // Select best
    float32 min_cost = 1e9f;
    int best_idx = -1;
    for (size_t i = 0; i < candidates_.size(); ++i) {
        if (!candidates_[i].valid) continue;
        if (candidates_[i].cost < min_cost) {
            min_cost = candidates_[i].cost;
            best_idx = static_cast<int>(i);
        }
    }

    if (best_idx != -1) {
        best_path_ = candidates_[best_idx];
        decision_text_ = best_path_.description + " (Cost: " + std::to_string(static_cast<int>(best_path_.cost)) + ")";
    } else {
        decision_text_ = "EMERGENCY: No Valid Path";
    }

    // Publish
    port_best_trajectory.write(toOutputTrajectory(best_path_));
    port_decision_text.write(decision_text_);
}

void PlannerSWC::onShutdown() {
    candidates_.clear();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Trajectory Generation
// ═══════════════════════════════════════════════════════════════════════════

void PlannerSWC::generateCandidate(int target_lane_idx, float32 v_start, float32 lat_start,
                                    float32 y_start, const std::vector<TrafficAgent>& traffic,
                                    TrafficLightState light, float32 stop_y, const std::string& desc) {
    InternalTrajectory traj;
    traj.target_lane = target_lane_idx;
    traj.description = desc;
    traj.cost = 0;

    float32 lat_target = kLaneCenter[std::clamp(target_lane_idx, 0, kNumLanes - 1)];

    // Find leader in target lane
    float32 min_dist = 999.0f;
    float32 leader_v = 30.0f; // m/s

    for (const auto& car : traffic) {
        // Check if car is in target lane OR moving into target lane (cut-in prediction)
        bool relevant = std::abs(car.effectiveX() - lat_target) < 2.0f;
        if (!relevant && car.is_changing_lane) {
            float32 target_x = kLaneCenter[std::clamp(car.target_lane, 0, kNumLanes-1)];
            if (std::abs(target_x - lat_target) < 0.5f) relevant = true;
        }

        if (relevant) {
            float32 dist = car.world_y - y_start;
            
            // Leader handling (car ahead)
            if (dist > 0 && dist < min_dist) {
                min_dist = dist;
                leader_v = car.speed;
            } 
            // Rear handling (blind spot / collision check)
            else if (dist < 0 && dist > -60.0f) {
                // 1. Immediate collision check (overlap)
                if (dist > -12.0f && std::abs(lat_start - lat_target) > 0.1f) {
                    traj.cost += W_COLLISION; // Direct overlap
                    traj.valid = false;
                }
                
                // 2. Blind Spot TTC Check (closing speed)
                // Only relevant if we are changing lanes
                else if (std::abs(lat_start - lat_target) > 0.1f) {
                    float32 rel_speed = car.speed - v_start; // Positive if car is faster (closing in)
                    if (rel_speed > 2.0f) { // Approaching significantly faster
                        float32 ttc = std::abs(dist) / rel_speed;
                        if (ttc < 3.5f) { // Less than 3.5s to impact
                            traj.cost += W_COLLISION * 2.0f; // Unsafe lane change
                            traj.valid = false;
                        }
                    }
                }
            }
        }
    }

    // Red light check
    if (light != TrafficLightState::GREEN) {
        float32 dist_stop = stop_y - y_start;
        if (dist_stop > 0 && dist_stop < min_dist) {
            min_dist = dist_stop - 5.0f;
            leader_v = 0.0f;
        }
    }

    // Target speed calculation
    // 1. Base limit from scenario
    float32 limit_v = env_model_->getScenarioConfig().speed_limit / 3.6f;
    
    // 2. Curve adaptation (slow down for sharp turns)
    // Intensity 0.05 -> 0.875 * limit
    // Intensity 0.20 -> 0.500 * limit
    float32 curve_intensity = env_model_->getScenarioConfig().curve_intensity;
    float32 curve_factor = std::max(0.5f, 1.0f - curve_intensity * 2.5f);
    float32 safe_curve_v = limit_v * curve_factor;

    // 3. Traffic flow (ACC)
    float32 traffic_v = (min_dist < 40.0f) ? std::min(leader_v, safe_curve_v) : safe_curve_v;

    // Final desired speed
    float32 desired_v = std::min(limit_v, traffic_v);
    if (desired_v < 0) desired_v = 0;

    traj.target_speed = desired_v;
    traj.cost += W_EFFICIENCY * (limit_v - desired_v); // Penalty for being slow relative to limit

    // Lane change penalty
    if (std::abs(lat_start - lat_target) > 0.1f) traj.cost += W_LANE_CHANGE;

    // ─── Keep-Right Rule (autonomous driving principle) ─────────────
    // Penalize left lanes: lane 1 gets small penalty, lane 0 large
    // This ensures ego prefers lanes 2-3 (right side) unless overtaking
    if (target_lane_idx <= 1) traj.cost += 2.0f;  // Reduced from 5.0 to encourage overtaking
    if (target_lane_idx == 0) traj.cost += 10.0f; // Far-left gets heavy penalty

    // Generate trajectory points
    float32 T = 4.0f;
    float32 accel = (desired_v - v_start) / 2.0f;
    accel = std::clamp(accel, -5.0f, 3.0f);

    float32 lat_diff = lat_target - lat_start;

    for (int i = 0; i <= 40; ++i) {
        float32 t = i * DT_PLAN;
        float32 t_norm = t / T;
        float32 lat_factor = t_norm * t_norm * (3.0f - 2.0f * t_norm);
        float32 y_pos = v_start * t + 0.5f * accel * t * t;
        float32 x_pos = lat_start + lat_diff * lat_factor;
        traj.points_x.push_back(x_pos);
        traj.points_y.push_back(y_pos);
    }

    candidates_.push_back(traj);
}

Trajectory PlannerSWC::toOutputTrajectory(const InternalTrajectory& internal) const {
    Trajectory out;
    size_t count = std::min(internal.points_x.size(), kMaxTrajectoryPoints);
    for (size_t i = 0; i < count; ++i) {
        out.points[i] = {internal.points_x[i], internal.points_y[i]};
    }
    out.point_count  = count;
    out.cost         = internal.cost;
    out.target_lane  = internal.target_lane;
    out.target_speed = internal.target_speed;
    out.valid        = internal.valid;
    out.description  = internal.description;
    return out;
}

} // namespace adas
