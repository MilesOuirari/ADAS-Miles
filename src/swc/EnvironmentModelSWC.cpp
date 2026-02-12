#include "swc/EnvironmentModelSWC.hpp"

namespace adas {

// ═══════════════════════════════════════════════════════════════════════════
//  Construction / Lifecycle
// ═══════════════════════════════════════════════════════════════════════════

EnvironmentModelSWC::EnvironmentModelSWC()
    : ComponentBase("EnvironmentModelSWC") {}

bool EnvironmentModelSWC::onInit() {
    agents_.reserve(64);
    return true;
}

bool EnvironmentModelSWC::onConfigure() {
    return true;
}

void EnvironmentModelSWC::onShutdown() {
    agents_.clear();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Main Simulation Step
// ═══════════════════════════════════════════════════════════════════════════

void EnvironmentModelSWC::onStep(float dt) {
    const auto& ego = port_ego_state.read();
    const auto& cfg = port_scenario_config.read();

    float32 ego_y     = ego.world_y;
    float32 ego_x     = ego.lane_x;
    float32 limit_mps = cfg.speed_limit / 3.6f;

    scenario_timer_ += dt;

    // On scenario change: reset traffic
    if (cfg.type != last_scenario_) {
        last_scenario_ = cfg.type;
        agents_.clear();
        scenario_timer_ = 0.0f;
        scenario_initialized_ = false;
        light_state_ = TrafficLightState::GREEN;
        light_timer_ = 0.0f;
    }

    // ─── 1. Traffic lights ─────────────────────────────────────────────
    updateTrafficLights(dt);

    // ─── 2. Scenario-specific modifiers (spawn special agents, etc.) ──
    applyScenarioModifiers(cfg, ego_y, dt);

    // ─── 3. IDM car-following ──────────────────────────────────────────
    updateIDM(dt, ego_y, ego_x);

    // ─── 4. MOBIL lane changes ─────────────────────────────────────────
    updateLaneChanges(dt);

    // ─── 5. Cleanup & persistent spawning ──────────────────────────────
    int target_count = cfg.traffic_density * 2 + 4;
    cleanupAndSpawn(ego_y, limit_mps);
    ensureMinimumTraffic(ego_y, limit_mps, target_count);

    // ─── 6. Update world_x from lane state ─────────────────────────────
    for (auto& a : agents_) {
        a.world_x = a.effectiveX();
    }

    // ─── 7. Build and publish outputs ──────────────────────────────────
    float32 curve_val = cfg.curve_intensity * 100.0f * std::sin(scenario_timer_ * 0.05f);
    buildObjectList(ego_y, ego_x, curve_val);
    buildLaneNetwork(ego_y, ego_x, curve_val);
    buildTrafficSigns();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Traffic Light Cycle
// ═══════════════════════════════════════════════════════════════════════════

void EnvironmentModelSWC::updateTrafficLights(float32 dt) {
    light_timer_ += dt;

    if (light_state_ == TrafficLightState::GREEN && light_timer_ > 15.0f) {
        light_state_ = TrafficLightState::YELLOW;
        light_timer_ = 0;
    } else if (light_state_ == TrafficLightState::YELLOW && light_timer_ > 3.0f) {
        light_state_ = TrafficLightState::RED;
        light_timer_ = 0;
    } else if (light_state_ == TrafficLightState::RED && light_timer_ > 12.0f) {
        light_state_ = TrafficLightState::GREEN;
        light_timer_ = 0;
        stop_line_y_ += 800.0f; // Next intersection
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  IDM Car-Following
// ═══════════════════════════════════════════════════════════════════════════

void EnvironmentModelSWC::updateIDM(float32 dt, float32 ego_y, float32 ego_x) {
    for (auto& agent : agents_) {
        // Skip pedestrians — they have their own movement
        if (agent.class_id == 4) continue;

        // Find the leader (nearest vehicle ahead in same effective lane)
        LeaderInfo leader = findLeader(agent, ego_y, 0.0f);

        // Also treat stop line as a virtual leader when RED/YELLOW
        if (light_state_ != TrafficLightState::GREEN) {
            float32 dist_to_stop = stop_line_y_ - agent.world_y;
            if (dist_to_stop > 0 && dist_to_stop < leader.gap) {
                leader.gap = dist_to_stop - 3.0f; // Stop 3m before line
                leader.speed = 0.0f;
                leader.found = true;
            }
        }

        // IDM acceleration
        float32 gap = leader.found ? leader.gap : 999.0f;
        float32 accel = agent.idmAcceleration(gap, leader.found ? leader.speed : agent.desired_speed);

        // Clamp acceleration for realism
        accel = std::clamp(accel, -8.0f, agent.idm_a);

        // Update speed and position
        agent.speed += accel * dt;
        agent.speed = std::max(0.0f, agent.speed);
        agent.world_y += agent.speed * dt;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Leader Finding
// ═══════════════════════════════════════════════════════════════════════════

EnvironmentModelSWC::LeaderInfo
EnvironmentModelSWC::findLeader(const TrafficAgent& agent, float32 ego_y, float32 ego_speed) const {
    LeaderInfo best;
    float32 agent_x = agent.effectiveX();

    for (const auto& other : agents_) {
        if (other.id == agent.id) continue;

        float32 other_x = other.effectiveX();

        // Same lane check: lateral distance < half lane width
        if (std::abs(other_x - agent_x) > kLaneWidth * 0.6f) continue;

        // Must be ahead
        float32 gap = other.world_y - agent.world_y - other.length;
        if (gap < 0) continue;

        if (gap < best.gap) {
            best.gap   = gap;
            best.speed = other.speed;
            best.found = true;
        }
    }

    // Also check ego as potential leader
    float32 ego_x_eff = 0.0f; // Ego lateral (approximate)
    if (std::abs(ego_x_eff - agent_x) < kLaneWidth * 0.6f) {
        float32 gap = ego_y - agent.world_y - 4.5f;
        if (gap > 0 && gap < best.gap) {
            best.gap   = gap;
            best.speed = ego_speed;
            best.found = true;
        }
    }

    return best;
}

// ═══════════════════════════════════════════════════════════════════════════
//  MOBIL Lane Change
// ═══════════════════════════════════════════════════════════════════════════

void EnvironmentModelSWC::updateLaneChanges(float32 dt) {
    for (auto& agent : agents_) {
        if (agent.class_id == 4) continue; // Skip pedestrians

        if (agent.is_changing_lane) {
            // Execute the transition
            agent.lane_change_progress += dt / 3.0f; // 3 seconds to complete
            if (agent.lane_change_progress >= 1.0f) {
                agent.lane = agent.target_lane;
                agent.is_changing_lane = false;
                agent.lane_change_progress = 0.0f;
            }
        } else {
            // Check if lane change is desirable (every ~1s, randomized)
            if (rand() % 60 != 0) continue;

            // Try left first, then right
            if (agent.lane > 0 && shouldChangeLane(agent, agent.lane - 1, 0, 0)) {
                agent.target_lane = agent.lane - 1;
                agent.is_changing_lane = true;
                agent.lane_change_progress = 0.0f;
            } else if (agent.lane < kNumLanes - 1 && shouldChangeLane(agent, agent.lane + 1, 0, 0)) {
                agent.target_lane = agent.lane + 1;
                agent.is_changing_lane = true;
                agent.lane_change_progress = 0.0f;
            }
        }
    }
}

boolean EnvironmentModelSWC::shouldChangeLane(const TrafficAgent& agent, int new_lane,
                                               float32 /*ego_y*/, float32 /*ego_speed*/) const {
    float32 new_lane_x = kLaneCenter[new_lane];

    // ─── Safety check: is there a gap? ─────────────────────────────────
    for (const auto& other : agents_) {
        if (other.id == agent.id) continue;
        float32 other_x = other.effectiveX();

        // Check if other vehicle is in the target lane
        if (std::abs(other_x - new_lane_x) > kLaneWidth * 0.5f) continue;

        // Check longitudinal gap (need 15m front and back)
        float32 gap = std::abs(other.world_y - agent.world_y);
        if (gap < 15.0f) return false;
    }

    // ─── Incentive check: is current lane slow? ────────────────────────
    // Find leader in current lane
    float32 current_leader_gap = 999.0f;
    for (const auto& other : agents_) {
        if (other.id == agent.id) continue;
        float32 other_x = other.effectiveX();
        if (std::abs(other_x - kLaneCenter[agent.lane]) > kLaneWidth * 0.5f) continue;
        float32 gap = other.world_y - agent.world_y;
        if (gap > 0 && gap < current_leader_gap) current_leader_gap = gap;
    }

    // Only change if the current lane leader is too close
    return current_leader_gap < 30.0f;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Cleanup & Persistent Spawning
// ═══════════════════════════════════════════════════════════════════════════

void EnvironmentModelSWC::cleanupAndSpawn(float32 ego_y, float32 speed_limit_mps) {
    // ─── Remove agents that have fallen behind ─────────────────────────
    // Tight window behind (50m) so overtaken cars are removed quickly,
    // which triggers fresh spawns on the horizon.
    agents_.erase(
        std::remove_if(agents_.begin(), agents_.end(),
            [ego_y](const TrafficAgent& a) {
                return a.world_y < ego_y - 50.0f || a.world_y > ego_y + 500.0f;
            }),
        agents_.end());

    // ─── Continuous horizon spawning ───────────────────────────────────
    // Every few frames, inject a new car at the far end of visible range
    // (200-300m ahead). These cars are typically SLOWER than ego, so the
    // ego will gradually approach and pass them — creating an infinite
    // stream of traffic from the distance.
    spawn_timer_++;
    if (spawn_timer_ >= 10 && agents_.size() < 30) {  // every ~10 frames
        spawn_timer_ = 0;

        float32 spawn_y = ego_y + 200.0f + static_cast<float32>(rand() % 100);

        // Check no overlap with existing agents
        boolean ok = true;
        for (const auto& a : agents_) {
            if (std::abs(a.world_y - spawn_y) < 15.0f) {
                ok = false;
                break;
            }
        }

        if (ok) {
            TrafficAgent car = spawnAgent(spawn_y, speed_limit_mps);
            // Horizon cars are slightly slower so ego catches up to them
            car.desired_speed *= 0.75f + (rand() % 100) * 0.002f; // 75-95% of limit
            car.speed = car.desired_speed;
            agents_.push_back(car);
        }
    }
}

void EnvironmentModelSWC::ensureMinimumTraffic(float32 ego_y, float32 speed_limit_mps, int target_count) {
    // Safety net: if traffic drops below minimum, fill immediately.
    // This covers scenario changes, initial startup, etc.

    int attempts = 0;
    while (static_cast<int>(agents_.size()) < target_count && attempts < 50) {
        attempts++;

        // Spread across the full visible range: ego_y +20 to ego_y +280
        float32 spawn_y = ego_y + 20.0f + static_cast<float32>(rand() % 260);

        boolean too_close = false;
        for (const auto& a : agents_) {
            if (std::abs(a.world_y - spawn_y) < 12.0f) {
                too_close = true;
                break;
            }
        }
        if (too_close) continue;

        agents_.push_back(spawnAgent(spawn_y, speed_limit_mps));
    }
}

TrafficAgent EnvironmentModelSWC::spawnAgent(float32 y_pos, float32 speed_limit_mps) {
    TrafficAgent a;
    a.id = next_id_++;
    a.lane = rand() % kNumLanes;
    a.target_lane = a.lane;
    a.world_x = kLaneCenter[a.lane];
    a.world_y = y_pos;

    // Vehicle type distribution
    int r = rand() % 100;
    if (r < 65) {
        a.class_id = 0; // Car (65%)
        a.length = 4.5f;
        a.idm_a = 1.4f + (rand() % 100) * 0.006f; // 1.4 – 2.0
        a.idm_b = 1.8f + (rand() % 100) * 0.004f;
        a.idm_T = 1.2f + (rand() % 100) * 0.008f; // 1.2 – 2.0s
    } else if (r < 80) {
        a.class_id = 2; // Truck (15%)
        a.length = 12.0f;
        a.idm_a = 0.8f;  // Slower acceleration
        a.idm_b = 1.5f;
        a.idm_T = 2.0f;  // Larger headway
    } else if (r < 88) {
        a.class_id = 1; // Bus (8%)
        a.length = 10.0f;
        a.idm_a = 1.0f;
        a.idm_b = 1.5f;
        a.idm_T = 1.8f;
    } else if (r < 95) {
        a.class_id = 3; // Bike (7%)
        a.length = 2.0f;
        a.idm_a = 2.0f;  // Quick acceleration
        a.idm_b = 3.0f;
        a.idm_T = 1.0f;  // Shorter headway
    } else {
        a.class_id = 4; // Pedestrian (5%)
        a.length = 0.5f;
        a.lane = -1;
        a.world_x = (rand() % 2 == 0) ? -10.0f : 10.0f; // Sidewalk
        a.speed = 1.2f + (rand() % 100) * 0.008f; // 1.2 – 2.0 m/s
        a.desired_speed = a.speed;
        return a;
    }

    // Set desired speed based on scenario speed limit with ±15% variation
    float32 variation = 0.85f + (rand() % 100) * 0.003f; // 0.85 – 1.15
    a.desired_speed = speed_limit_mps * variation;

    // Trucks and buses are inherently slower
    if (a.class_id == 2) a.desired_speed *= 0.85f;
    if (a.class_id == 1) a.desired_speed *= 0.90f;

    // Spawn at flow speed (not zero — they're already driving)
    a.speed = a.desired_speed * (0.9f + (rand() % 100) * 0.002f);

    return a;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Scenario-Specific Modifiers
// ═══════════════════════════════════════════════════════════════════════════

void EnvironmentModelSWC::applyScenarioModifiers(const ScenarioConfig& cfg, float32 ego_y, float32 dt) {
    switch (cfg.type) {
        case ScenarioType::EMERGENCY_BRAKE: {
            // Inject a sudden-braking car in ego lane after 2 seconds
            if (!scenario_initialized_ && scenario_timer_ > 2.0f) {
                TrafficAgent brake_car;
                brake_car.id = next_id_++;
                brake_car.class_id = 0;
                brake_car.lane = 1;
                brake_car.target_lane = 1;
                brake_car.world_x = kLaneCenter[1];
                brake_car.world_y = ego_y + 60.0f;
                brake_car.speed = cfg.speed_limit / 3.6f;
                brake_car.desired_speed = 0.0f; // Will decelerate to stop!
                brake_car.length = 4.5f;
                brake_car.idm_a = 1.0f;
                brake_car.idm_b = 6.0f; // Hard brake capability
                agents_.push_back(brake_car);
                scenario_initialized_ = true;
            }
            break;
        }

        case ScenarioType::LANE_CHANGE: {
            // Ensure there's always a slow truck in center lane
            boolean has_slow_center = false;
            for (const auto& a : agents_) {
                if (a.class_id == 2 && a.lane == 1 &&
                    a.world_y > ego_y && a.world_y < ego_y + 120.0f) {
                    has_slow_center = true;
                    break;
                }
            }
            if (!has_slow_center) {
                TrafficAgent truck;
                truck.id = next_id_++;
                truck.class_id = 2;
                truck.lane = 1;
                truck.target_lane = 1;
                truck.world_x = kLaneCenter[1];
                truck.world_y = ego_y + 80.0f;
                truck.speed = cfg.speed_limit / 3.6f * 0.5f;
                truck.desired_speed = truck.speed; // Stays slow
                truck.length = 12.0f;
                truck.idm_a = 0.6f;
                truck.idm_b = 1.5f;
                truck.idm_T = 2.5f;
                agents_.push_back(truck);
            }
            break;
        }

        case ScenarioType::TRAFFIC_JAM: {
            // Override all agents to have very low desired speed
            for (auto& a : agents_) {
                if (a.class_id != 4) {
                    a.desired_speed = std::min(a.desired_speed, 5.0f); // 5 m/s = 18 km/h
                }
            }
            break;
        }

        case ScenarioType::PEDESTRIAN_CROSSING: {
            // Periodically spawn crossing pedestrians
            static int ped_timer = 0;
            ped_timer++;
            if (ped_timer > 180) { // Every 3 seconds
                ped_timer = 0;
                TrafficAgent ped;
                ped.id = next_id_++;
                ped.class_id = 4;
                ped.lane = -1;
                ped.world_x = -12.0f;
                ped.world_y = ego_y + 40.0f + static_cast<float32>(rand() % 30);
                ped.speed = 1.5f;
                ped.desired_speed = 1.5f;
                ped.length = 0.5f;
                // Pedestrian movement is handled specially: world_x drifts right
                agents_.push_back(ped);
            }
            // Move pedestrians laterally
            for (auto& a : agents_) {
                if (a.class_id == 4 && a.world_x < 12.0f) {
                    a.world_x += 2.0f * dt; // Walk across
                }
            }
            break;
        }

        case ScenarioType::HIGHWAY_MERGE: {
            // Spawn merging vehicles from the right
            if (rand() % 120 == 0 && agents_.size() < 30) {
                TrafficAgent merger;
                merger.id = next_id_++;
                merger.class_id = 0;
                merger.lane = 2;
                merger.target_lane = 1; // Merging into center
                merger.is_changing_lane = true;
                merger.lane_change_progress = 0.0f;
                merger.world_x = kLaneCenter[2] + 5.0f; // Start from on-ramp
                merger.world_y = ego_y + 60.0f + static_cast<float32>(rand() % 50);
                merger.speed = cfg.speed_limit / 3.6f * 0.8f;
                merger.desired_speed = cfg.speed_limit / 3.6f;
                merger.length = 4.5f;
                agents_.push_back(merger);
            }
            break;
        }

        default:
            break;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Output: ObjectList
// ═══════════════════════════════════════════════════════════════════════════

void EnvironmentModelSWC::buildObjectList(float32 ego_y, float32 ego_x, float32 curve_val) {
    ObjectList obj_list;
    obj_list.count = 0;

    for (const auto& agent : agents_) {
        float32 rel_y = agent.world_y - ego_y;

        // Only include agents within visible range
        if (rel_y < -20.0f || rel_y > 200.0f) continue;

        float32 render_x = agent.world_x - ego_x;
        float32 curve_at_obj = curve_val * (rel_y / 100.0f);
        render_x += curve_at_obj;
        float32 yaw = std::atan2(curve_val, 100.0f);

        obj_list.objects[obj_list.count++] = {
            render_x, rel_y, 2.0f, agent.length,
            static_cast<uint32>(agent.class_id), yaw
        };

        if (obj_list.count >= kMaxObjects) break;
    }

    obj_list.is_valid = true;
    port_object_list.write(obj_list);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Output: Lane Network
// ═══════════════════════════════════════════════════════════════════════════

void EnvironmentModelSWC::buildLaneNetwork(float32 ego_y, float32 ego_x, float32 curve_val) {
    LaneNetwork lanes;
    lanes.count = 4;
    for (int l = 0; l < 4; ++l) {
        float32 base = (l - 1.5f) * kLaneWidth;  // absolute lane X position
        lanes.lanes[l].point_count = 50;
        for (int i = 0; i < 50; ++i) {
            float32 dist = i * 2.0f;
            float32 curve = curve_val * (dist / 100.0f);
            lanes.lanes[l].points[i] = { base + curve, ego_y + dist };
        }
    }
    port_lane_network.write(lanes);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Output: Traffic Signs
// ═══════════════════════════════════════════════════════════════════════════

void EnvironmentModelSWC::buildTrafficSigns() {
    TrafficSignList signs;
    signs.count = 0;

    // Red light indicator
    if (light_state_ == TrafficLightState::RED) {
        signs.signs[signs.count++] = { 5.0f, 20.0f, 0, 5, 0 };
    }

    // Cycling speed limit signs
    static float32 sign_timer = 0.0f;
    static int current_limit = 50;
    static int show_stop = 0;

    sign_timer += 0.016f;
    if (sign_timer > 5.0f) {
        sign_timer = 0.0f;
        int limits[] = { 30, 50, 80, 50, 100, 120 };
        current_limit = limits[rand() % 6];
        if (rand() % 3 == 0) show_stop = 120;
    }

    if (show_stop > 0) {
        signs.signs[signs.count++] = { 10.0f, 20.0f, 0, 1, 0 };
        show_stop--;
    } else if (signs.count < 8) {
        signs.signs[signs.count++] = { 15.0f, 20.0f, 0, 0, current_limit };
    }

    port_traffic_signs.write(signs);
}

} // namespace adas
