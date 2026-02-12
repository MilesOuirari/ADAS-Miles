#ifndef HMI_SCENARIO_MANAGER_HPP
#define HMI_SCENARIO_MANAGER_HPP

#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "TrafficManager.hpp"

namespace adas::hmi {

enum class ScenarioType {
    HIGHWAY_CRUISE,
    CITY_DRIVING,
    EMERGENCY_BRAKE,
    LANE_CHANGE,
    TRAFFIC_JAM,
    PEDESTRIAN_CROSSING,
    INTERSECTION,
    HIGHWAY_MERGE,
    COUNT // Total number of scenarios
};

struct ScenarioConfig {
    std::string name;
    ScenarioType type;
    float initial_speed;
    float speed_limit;
    int traffic_density; // 0-10
    bool has_intersection;
    float curve_intensity; // Road curvature
    std::string description;
};

class ScenarioManager {
public:
    std::vector<ScenarioConfig> scenarios;
    int current_scenario = 0;
    float scenario_timer = 0.0f;
    bool auto_cycle = true;
    float cycle_duration = 20.0f; // 20 seconds per scenario
    
    // Scenario-specific state
    bool emergency_brake_triggered = false;
    float emergency_brake_distance = 0.0f;
    int pedestrian_spawn_timer = 0;
    
    void init() {
        // Define all scenarios
        scenarios = {
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
        
        std::cout << "Scenario Manager initialized with " << scenarios.size() << " scenarios." << std::endl;
    }
    
    void nextScenario() {
        current_scenario = (current_scenario + 1) % scenarios.size();
        resetScenario();
        std::cout << ">>> Scenario: " << getCurrentConfig().name << std::endl;
        std::cout << "    " << getCurrentConfig().description << std::endl;
    }
    
    void prevScenario() {
        current_scenario = (current_scenario - 1 + scenarios.size()) % scenarios.size();
        resetScenario();
        std::cout << ">>> Scenario: " << getCurrentConfig().name << std::endl;
    }
    
    void setScenario(int idx) {
        if (idx >= 0 && idx < (int)scenarios.size()) {
            current_scenario = idx;
            resetScenario();
        }
    }
    
    void resetScenario() {
        scenario_timer = 0.0f;
        emergency_brake_triggered = false;
        pedestrian_spawn_timer = 0;
    }
    
    const ScenarioConfig& getCurrentConfig() const {
        return scenarios[current_scenario];
    }
    
    void update(float dt) {
        scenario_timer += dt;
        
        // Auto-cycle scenarios
        if (auto_cycle && scenario_timer > cycle_duration) {
            nextScenario();
        }
    }
    
    // Configure traffic manager for current scenario
    void configureTraffic(TrafficManager& traffic, float ego_y) {
        const auto& cfg = getCurrentConfig();
        
        // Adjust traffic density
        int target_agents = cfg.traffic_density * 2;
        
        // Scenario-specific spawning
        switch (cfg.type) {
            case ScenarioType::HIGHWAY_CRUISE:
                configureHighway(traffic, ego_y, target_agents);
                break;
                
            case ScenarioType::CITY_DRIVING:
                configureCity(traffic, ego_y, target_agents);
                break;
                
            case ScenarioType::EMERGENCY_BRAKE:
                configureEmergencyBrake(traffic, ego_y);
                break;
                
            case ScenarioType::LANE_CHANGE:
                configureLaneChange(traffic, ego_y);
                break;
                
            case ScenarioType::TRAFFIC_JAM:
                configureTrafficJam(traffic, ego_y, target_agents);
                break;
                
            case ScenarioType::PEDESTRIAN_CROSSING:
                configurePedestrianCrossing(traffic, ego_y);
                break;
                
            case ScenarioType::INTERSECTION:
                // Use default intersection logic from TrafficManager
                break;
                
            case ScenarioType::HIGHWAY_MERGE:
                configureHighwayMerge(traffic, ego_y);
                break;
                
            default:
                break;
        }
    }
    
private:
    void configureHighway(TrafficManager& traffic, float ego_y, int target) {
        // Fast moving, spread out traffic
        while((int)traffic.agents.size() < target) {
            VehicleAgent a;
            a.id = traffic.next_id++;
            a.lane = rand() % 3;
            a.current_lat = (a.lane - 1.0f) * 3.5f;
            a.target_lat = a.current_lat;
            a.x = a.current_lat;
            a.y = ego_y + 50.0f + (rand() % 200);
            a.class_id = (rand() % 100 < 80) ? 0 : 2; // Mostly cars, some trucks
            a.length = (a.class_id == 2) ? 12.0f : 4.5f;
            a.speed = 90.0f + (rand() % 40);
            traffic.agents.push_back(a);
        }
    }
    
    void configureCity(TrafficManager& traffic, float ego_y, int target) {
        // Slower, mixed traffic
        while((int)traffic.agents.size() < target) {
            VehicleAgent a;
            a.id = traffic.next_id++;
            a.lane = rand() % 3;
            a.current_lat = (a.lane - 1.0f) * 3.5f;
            a.target_lat = a.current_lat;
            a.x = a.current_lat;
            a.y = ego_y + 20.0f + (rand() % 100);
            
            int r = rand() % 100;
            if (r < 50) a.class_id = 0; // Car
            else if (r < 70) a.class_id = 1; // Bus
            else if (r < 85) a.class_id = 3; // Bike
            else a.class_id = 4; // Pedestrian
            
            a.length = (a.class_id == 1) ? 10.0f : (a.class_id == 2 ? 12.0f : 4.5f);
            a.speed = 30.0f + (rand() % 30);
            
            if (a.class_id == 4) { // Pedestrian
                a.current_lat = (rand() % 2) ? -10.0f : 10.0f;
                a.x = a.current_lat;
                a.speed = 1.5f;
            }
            
            traffic.agents.push_back(a);
        }
    }
    
    void configureEmergencyBrake(TrafficManager& traffic, float ego_y) {
        if (!emergency_brake_triggered && scenario_timer > 2.0f) {
            // Spawn lead vehicle that will suddenly brake
            VehicleAgent a;
            a.id = traffic.next_id++;
            a.lane = 1; // Center lane
            a.current_lat = 0.0f;
            a.target_lat = 0.0f;
            a.x = 0.0f;
            a.y = ego_y + 60.0f;
            a.class_id = 0;
            a.length = 4.5f;
            a.speed = 80.0f;
            traffic.agents.push_back(a);
            
            emergency_brake_triggered = true;
            emergency_brake_distance = ego_y + 60.0f;
        }
        
        // Make the lead vehicle brake hard
        if (emergency_brake_triggered) {
            for (auto& agent : traffic.agents) {
                if (std::abs(agent.y - emergency_brake_distance) < 50.0f && agent.lane == 1) {
                    if (scenario_timer > 5.0f) {
                        agent.speed = std::max(0.0f, agent.speed - 5.0f); // Sudden brake
                    }
                }
            }
        }
    }
    
    void configureLaneChange(TrafficManager& traffic, float ego_y) {
        // Spawn slow vehicle in ego lane
        bool hasSlowVehicle = false;
        for (const auto& a : traffic.agents) {
            if (a.lane == 1 && a.y > ego_y && a.y < ego_y + 100.0f && a.speed < 50.0f) {
                hasSlowVehicle = true;
                break;
            }
        }
        
        if (!hasSlowVehicle && traffic.agents.size() < 10) {
            VehicleAgent a;
            a.id = traffic.next_id++;
            a.lane = 1;
            a.current_lat = 0.0f;
            a.target_lat = 0.0f;
            a.x = 0.0f;
            a.y = ego_y + 80.0f;
            a.class_id = 2; // Truck (slow)
            a.length = 12.0f;
            a.speed = 40.0f;
            traffic.agents.push_back(a);
        }
        
        // Add some faster traffic in adjacent lanes
        if (traffic.agents.size() < 8) {
            for (int lane = 0; lane <= 2; lane += 2) { // Left and right lanes
                if (rand() % 100 < 30) {
                    VehicleAgent a;
                    a.id = traffic.next_id++;
                    a.lane = lane;
                    a.current_lat = (lane - 1.0f) * 3.5f;
                    a.target_lat = a.current_lat;
                    a.x = a.current_lat;
                    a.y = ego_y + 50.0f + (rand() % 150);
                    a.class_id = 0;
                    a.length = 4.5f;
                    a.speed = 80.0f + (rand() % 30);
                    traffic.agents.push_back(a);
                }
            }
        }
    }
    
    void configureTrafficJam(TrafficManager& traffic, float ego_y, int target) {
        target = std::max(target, 15);
        
        while ((int)traffic.agents.size() < target) {
            VehicleAgent a;
            a.id = traffic.next_id++;
            a.lane = rand() % 3;
            a.current_lat = (a.lane - 1.0f) * 3.5f;
            a.target_lat = a.current_lat;
            a.x = a.current_lat;
            a.y = ego_y + 10.0f + (rand() % 100);
            a.class_id = 0;
            a.length = 4.5f;
            a.speed = 5.0f + (rand() % 20); // Very slow
            traffic.agents.push_back(a);
        }
        
        // Occasionally make traffic stop completely
        if ((int)(scenario_timer * 10) % 50 < 10) {
            for (auto& a : traffic.agents) {
                a.speed = std::max(0.0f, a.speed - 2.0f);
            }
        }
    }
    
    void configurePedestrianCrossing(TrafficManager& traffic, float ego_y) {
        pedestrian_spawn_timer++;
        
        if (pedestrian_spawn_timer > 180 && scenario_timer > 3.0f) { // Every ~3 seconds
            pedestrian_spawn_timer = 0;
            
            VehicleAgent ped;
            ped.id = traffic.next_id++;
            ped.class_id = 4;
            ped.lane = -1;
            ped.current_lat = -12.0f; // Start from left side
            ped.target_lat = 12.0f;   // Walk to right side
            ped.x = ped.current_lat;
            ped.y = ego_y + 50.0f + (rand() % 50);
            ped.length = 0.5f;
            ped.speed = 1.5f;
            ped.changing_lane = true; // Walking across
            traffic.agents.push_back(ped);
        }
        
        // Update pedestrian walking across
        for (auto& a : traffic.agents) {
            if (a.class_id == 4 && a.changing_lane) {
                float diff = a.target_lat - a.current_lat;
                if (std::abs(diff) > 0.5f) {
                    a.current_lat += (diff > 0 ? 1.0f : -1.0f) * 2.0f * 0.016f; // Walk speed
                    a.x = a.current_lat;
                } else {
                    a.changing_lane = false;
                }
            }
        }
    }
    
    void configureHighwayMerge(TrafficManager& traffic, float ego_y) {
        // Spawn vehicles from right lane (simulating on-ramp)
        if (rand() % 100 < 5 && traffic.agents.size() < 12) {
            VehicleAgent a;
            a.id = traffic.next_id++;
            a.lane = 2; // Right lane (merge lane)
            a.current_lat = (a.lane - 1.0f) * 3.5f + 5.0f; // Slightly outside
            a.target_lat = (a.lane - 1.0f) * 3.5f; // Target proper lane
            a.x = a.current_lat;
            a.y = ego_y + 80.0f + (rand() % 50);
            a.class_id = 0;
            a.length = 4.5f;
            a.speed = 70.0f + (rand() % 30);
            a.changing_lane = true; // Merging
            traffic.agents.push_back(a);
        }
        
        // Regular traffic in other lanes
        if (traffic.agents.size() < 8) {
            for (int lane = 0; lane < 2; ++lane) {
                if (rand() % 100 < 20) {
                    VehicleAgent a;
                    a.id = traffic.next_id++;
                    a.lane = lane;
                    a.current_lat = (lane - 1.0f) * 3.5f;
                    a.target_lat = a.current_lat;
                    a.x = a.current_lat;
                    a.y = ego_y + 50.0f + (rand() % 150);
                    a.class_id = 0;
                    a.length = 4.5f;
                    a.speed = 90.0f + (rand() % 30);
                    traffic.agents.push_back(a);
                }
            }
        }
    }
};

} // namespace adas::hmi

#endif // HMI_SCENARIO_MANAGER_HPP
