#ifndef HMI_TRAFFIC_MANAGER_HPP
#define HMI_TRAFFIC_MANAGER_HPP

#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace adas::hmi {

enum class TrafficLightState { GREEN, YELLOW, RED };

struct VehicleAgent {
    int id;
    float x, y; // World Coordinates
    float speed;
    int lane; // 0, 1, 2
    int class_id = 0; // 0=Car, 1=Bus, 2=Truck, 3=Bike
    float target_lat;
    float current_lat;
    
    // FSM
    bool changing_lane = false;
    
    // Constants
    float length = 4.5f;

    // Safety Check: Is target lane safe to enter?
    bool isLaneSafe(const std::vector<VehicleAgent>& neighbors, float target_l, float my_y) {
        for(const auto& n : neighbors) {
            if (n.id == id) continue;
            // Check lateral overlap (target lane +/- 1.0m)
            if (std::abs(n.current_lat - target_l) < 2.0f) {
                // Check longitudinal gap (Safety Distance 15m front/back)
                if (std::abs(n.y - my_y) < 15.0f) return false;
            }
        }
        return true;
    }

    void update(float dt, const std::vector<VehicleAgent>& neighbors, float ego_y, float ego_lane, float stop_line_y, TrafficLightState light) {
        // 1. Perception: Find leader
        float min_dist = 999.0f;
        float leader_speed = 999.0f;
        
        // Check Ego collision if close
        if (std::abs(ego_lane - current_lat) < 2.0f && ego_y > y) {
             float d = ego_y - y;
             if (d < min_dist) { min_dist = d; leader_speed = 60.0f; } // Ego speed approx
        }
        
        for(const auto& n : neighbors) {
            if (n.id == id) continue;
            if (std::abs(n.current_lat - current_lat) < 2.0f && n.y > y) { // Same lane, ahead
                float d = n.y - y;
                if (d < min_dist) {
                    min_dist = d;
                    leader_speed = n.speed;
                }
            }
        }
        
        // Stop Line Logic (Treat as virtual car with speed 0)
        if (light == TrafficLightState::RED || light == TrafficLightState::YELLOW) {
            if (stop_line_y > y && stop_line_y - y < 100.0f) { // Approaching Red Light
                float d = stop_line_y - y - 2.0f; // Stop 2m before line
                if (d < min_dist) {
                    min_dist = d;
                    leader_speed = 0.0f;
                }
            }
        }

        // 2. IDM (Intelligent Driver Model)
        float target_speed = 100.0f; 
        float s0 = 2.0f; // Min gap (Stop distance)
        float T = 1.5f;  // Time gap
        float a = 3.0f;  // Max accel
        float b = 3.0f;  // Comfy brake
        
        float s_star = s0 + speed * T + (speed * (speed - leader_speed)) / (2.0f * std::sqrt(a*b));
        float accel = a * (1.0f - std::pow(speed / target_speed, 4.0f) - std::pow(s_star / min_dist, 2.0f));
        
        speed += accel * dt;
        if (speed < 0) speed = 0;
        
        // Convert speed (km/h) to m/s for position update
        float speed_mps = speed / 3.6f;
        y += speed_mps * dt;

        // 3. Lane Change Logic (MOBIL-lite + Safety)
        if (!changing_lane) {
            // Only change if leader slow AND safe
            if (min_dist < 30.0f && speed < target_speed * 0.7f) {
                 // Try Left
                 if (lane > 0 && isLaneSafe(neighbors, (lane-1.0f)*3.5f - 3.5f, y)) {
                     if ((rand()%100) < 5) { // 5% chance
                        lane--; target_lat = (lane - 1.0f) * 3.5f; changing_lane = true;
                     }
                 }
                 // Try Right
                 else if (lane < 2 && isLaneSafe(neighbors, (lane+1.0f)*3.5f - 3.5f, y)) {
                     if ((rand()%100) < 5) {
                        lane++; target_lat = (lane - 1.0f) * 3.5f; changing_lane = true;
                     }
                 }
            }
        } else {
            // Execute Change
            float diff = target_lat - current_lat;
            if (std::abs(diff) > 0.1f) current_lat += diff * dt * 2.0f; 
            else { current_lat = target_lat; changing_lane = false; }
        }
        
        x = current_lat;
    }
};

class TrafficManager {
public:
    std::vector<VehicleAgent> agents;
    std::vector<VehicleAgent> cross_traffic; // Render-only agents for visual effect
    int next_id = 999;
    
    // Intersection State
    float stop_line_y = 500.0f;
    TrafficLightState light_state = TrafficLightState::GREEN;
    float light_timer = 0.0f;
    
    void init() {
        for(int i=0; i<8; ++i) spawnRandom(i * 30.0f + 20.0f);
    }

    void spawnRandom(float y_pos) {
        VehicleAgent a;
        a.id = next_id++;
        a.lane = rand() % 3; // 0..2
        a.current_lat = (a.lane - 1.0f) * 3.5f; 
        a.target_lat = a.current_lat;
        a.x = a.current_lat;
        a.y = y_pos;
        
        // Randomize Type
        int r = rand() % 100;
        if (r < 70) {
            a.class_id = 0; // Car
            a.length = 4.5f;
            a.speed = 60.0f + (rand() % 30); 
        } else if (r < 85) {
            a.class_id = 2; // Truck
            a.length = 12.0f; // Much longer
            a.speed = 50.0f + (rand() % 20); // Slower
        } else if (r < 90) {
            a.class_id = 1; // Bus
            a.length = 10.0f; 
            a.speed = 55.0f + (rand() % 20);
        } else if (r < 95) {
            a.class_id = 3; // Bike
            a.length = 2.0f;
            a.speed = 70.0f + (rand() % 40); // Fast
        } else {
             // Pedestrian (Sidewalk)
             a.class_id = 4;
             a.length = 0.5f;
             a.speed = 1.5f;
             // Force to sidewalk
             a.current_lat = (rand() % 2 == 0) ? -10.0f : 10.0f;
             a.x = a.current_lat;
             a.lane = -1; // Specialized lane
        }
        
        agents.push_back(a);
    }
    
    void spawnCrossTraffic() {
        if (cross_traffic.size() < 10 && (rand()%100) < 10) {
            VehicleAgent a;
            a.id = next_id++;
            a.y = stop_line_y + 10.0f; // Middle of intersection
            a.x = -50.0f; // Start left
            a.speed = 40.0f; // Moving Right
            cross_traffic.push_back(a);
        }
    }

    void update(float dt, float ego_y, float ego_lane) {
        // Traffic Light Cycle
        light_timer += dt;
        if (light_state == TrafficLightState::GREEN && light_timer > 10.0f) {
            light_state = TrafficLightState::YELLOW; light_timer = 0;
        } else if (light_state == TrafficLightState::YELLOW && light_timer > 3.0f) {
            light_state = TrafficLightState::RED; light_timer = 0;
        } else if (light_state == TrafficLightState::RED && light_timer > 10.0f) {
            light_state = TrafficLightState::GREEN; light_timer = 0;
            // Move Intersection ahead so we encounter another one later
            stop_line_y += 1000.0f; 
            cross_traffic.clear();
        }

        // Update Standard Agents
        // If Green, stop_line is effectively infinite (ignored)
        float effective_stop = (light_state == TrafficLightState::GREEN) ? 99999.0f : stop_line_y;
        
        for(auto& a : agents) {
            a.update(dt, agents, ego_y, ego_lane, effective_stop, light_state);
        }
        
        // Update Cross Traffic
        if(light_state == TrafficLightState::RED) {
             spawnCrossTraffic();
             for(auto& c : cross_traffic) {
                 c.x += c.speed * dt; // Just move sideways
             }
             // Cleanup
             cross_traffic.erase(std::remove_if(cross_traffic.begin(), cross_traffic.end(), 
                [](const VehicleAgent& a){ return a.x > 50.0f; }), cross_traffic.end());
        }

        // Cleanup main agents
        agents.erase(std::remove_if(agents.begin(), agents.end(), 
            [ego_y](const VehicleAgent& a){ return a.y < ego_y - 50.0f; }), agents.end());
            
        // Spawn ahead
        float max_y = ego_y;
        for(const auto& a : agents) if(a.y > max_y) max_y = a.y;
        if (agents.size() < 15 && max_y < ego_y + 300.0f) {
            spawnRandom(max_y + 30.0f + (rand()%20));
        }
    }
};

} // namespace adas::hmi
#endif
