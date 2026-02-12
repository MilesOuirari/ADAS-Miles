#ifndef HMI_PLANNER_HPP
#define HMI_PLANNER_HPP

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "hmi/TrafficManager.hpp" 

namespace adas::hmi {

struct Trajectory {
    std::vector<float> points_x;
    std::vector<float> points_y;
    float cost;
    int target_lane;
    std::string description;
    float target_speed;
    bool valid = true;
};

class TrajectoryPlanner {
public:
    // Constants
    const float LANE_WIDTH = 3.5f;
    const float PREDICTION_HORIZON = 4.0f; // Look ahead 4 seconds
    const float DT_PLAN = 0.1f; // Step size for points
    
    // Weights
    const float W_COLLISION = 99999.0f;
    const float W_EFFICIENCY = 1.0f;
    const float W_COMFORT = 10.0f;
    const float W_LANE_CHANGE = 5.0f; // Slight penalty to discourage jitter

    // State
    int current_lane_idx = 1; // Start in Center (0=Left, 1=Center, 2=Right in Logic? No, World is -3.5, 0, 3.5)
                              // Let's stick to World Coords: Lane 0 (-3.5), Lane 1 (0), Lane 2 (3.5)
                              // Wait, previous logic: Lane 0 = Center? 
                              // TrafficManager: lane 0 (-3.5), 1 (0), 2 (3.5).
                              // Ego starting at 0 means Lane 1.
    
    std::vector<Trajectory> candidates;
    Trajectory best_path;
    std::string decision_text;

    // Helper: Quintic Polynomial (Jerk Minimizing) 
    // s(t) = a0 + a1*t + a2*t^2 + a3*t^3 + a4*t^4 + a5*t^5
    // Simplified: Just use Cubic for lateral, Constant Accel for longitudinal for this demo
    
    float cubic(float a0, float a1, float a2, float a3, float t) {
        return a0 + a1*t + a2*t*t + a3*t*t*t;
    }

    void plan(float ego_speed, float ego_lat, float ego_y, const std::vector<VehicleAgent>& traffic, TrafficLightState light, float stop_line_y) {
        candidates.clear();
        
        // Identify current discrete lane
        int lane_idx = (int)std::round((ego_lat + 3.5f) / 3.5f); 
        // Logic: -3.5 -> 0, 0 -> 1, 3.5 -> 2
        
        // 1. Generate Candidates
        // Option A: Keep Lane
        generateTrajectory(lane_idx, ego_speed, ego_lat, ego_y, traffic, light, stop_line_y, "Keep Lane");
        
        // Option B: Left Lane (if exists)
        if(lane_idx > 0) 
            generateTrajectory(lane_idx - 1, ego_speed, ego_lat, ego_y, traffic, light, stop_line_y, "Lane Change Left");
            
        // Option C: Right Lane (if exists)
        if(lane_idx < 2) 
            generateTrajectory(lane_idx + 1, ego_speed, ego_lat, ego_y, traffic, light, stop_line_y, "Lane Change Right");

        // 2. Select Best
        float min_cost = 1e9;
        int best_idx = -1;
        
        for(size_t i=0; i<candidates.size(); ++i) {
            if (!candidates[i].valid) continue;
            if (candidates[i].cost < min_cost) {
                min_cost = candidates[i].cost;
                best_idx = i;
            }
        }
        
        if(best_idx != -1) {
            best_path = candidates[best_idx];
            decision_text = best_path.description + " (Cost: " + std::to_string((int)best_path.cost) + ")";
        } else {
            // Fallback emergency brake?
            decision_text = "EMERGENCY: No Valid Path";
        }
    }

    void generateTrajectory(int target_lane_idx, float v_start, float lat_start, float y_start, 
                            const std::vector<VehicleAgent>& traffic, TrafficLightState light, float stop_y, std::string desc) {
        
        Trajectory traj;
        traj.target_lane = target_lane_idx;
        traj.description = desc;
        traj.cost = 0;
        
        float lat_target = (target_lane_idx - 1.0f) * 3.5f;
        
        // Longitudinal Planning (ACC Logic embedded in trajectory)
        // Find leader in TARGET lane
        float min_dist = 999.0f;
        float leader_v = 100.0f;
        
        // Check Traffic
        for(const auto& car : traffic) {
             // Is car in target lane?
             if (std::abs(car.current_lat - lat_target) < 1.0f) {
                 float dist = car.y - y_start;
                 if (dist > 0 && dist < min_dist) {
                     min_dist = dist;
                     leader_v = car.speed;
                 } else if (dist > -10 && dist < 0) {
                     // Check Rear Collision (for Lane Change)
                     if (std::abs(lat_start - lat_target) > 0.1f) {
                         // Changing lane into existing car!
                         traj.cost += W_COLLISION; 
                         traj.valid = false;
                     }
                 }
             }
        }
        
        // Check Red Light
        if (light != TrafficLightState::GREEN) {
             float dist_stop = stop_y - y_start;
             if (dist_stop > 0 && dist_stop < min_dist) {
                 min_dist = dist_stop - 5.0f; // Stop 5m before
                 leader_v = 0.0f;
             }
        }
        
        // Target Speed Cost
        float desired_v = (min_dist < 40.0f) ? std::min(leader_v, 80.0f) : 100.0f; // Target 100kph or match leader
        if (desired_v < 0) desired_v = 0;
        
        traj.target_speed = desired_v;
        traj.cost += W_EFFICIENCY * (100.0f - desired_v); // Penalty for being slow
        
        // Lateral Cost (Change Lane Penalty)
        if (std::abs(lat_start - lat_target) > 0.1f) traj.cost += W_LANE_CHANGE;
        
        // Generate Points (for visualization & cost check)
        // Simple Cubic blend for lateral, Constant Accel for linear
        float T = 4.0f; // 4 second manuever
        float accel = (desired_v - v_start) / 2.0f; // Simple ramp
        if(accel > 3.0f) accel = 3.0f; if(accel < -5.0f) accel = -5.0f;

        float lat_diff = lat_target - lat_start;
        
        for(int i=0; i<=40; ++i) { // 40 points (0.1s each = 4s)
            float t = i * DT_PLAN;
            float t_norm = t / T; 
            
            // Lateral: 3t^2 - 2t^3 (SmoothStep)
            float lat_factor = t_norm * t_norm * (3.0f - 2.0f * t_norm);
            float y_pos = v_start * t + 0.5f * accel * t * t; // Relative Y
            float x_pos = lat_start + lat_diff * lat_factor;
            
            traj.points_x.push_back(x_pos);
            traj.points_y.push_back(y_pos); // Relative to Ego Start
        }
        
        candidates.push_back(traj);
    }
};

}
#endif
