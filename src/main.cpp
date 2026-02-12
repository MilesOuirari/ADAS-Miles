#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION // Prevent re-inclusion in Scene.hpp

#include "hud/DataModels.hpp"
#include "hmi/Scene.hpp"
#include "hmi/TrafficManager.hpp" 
#include "hmi/ScenarioManager.hpp"
#include "hmi/Planner.hpp"

void error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

// Global ScenarioManager for keyboard callback access
adas::hmi::ScenarioManager* g_scenario_manager = nullptr;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    
    // Scenario Controls
    if (action == GLFW_PRESS && g_scenario_manager) {
        if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_N) {
            g_scenario_manager->nextScenario();
        }
        if (key == GLFW_KEY_LEFT || key == GLFW_KEY_P) {
            g_scenario_manager->prevScenario();
        }
        if (key == GLFW_KEY_SPACE) {
            g_scenario_manager->auto_cycle = !g_scenario_manager->auto_cycle;
            std::cout << "Auto-cycle: " << (g_scenario_manager->auto_cycle ? "ON" : "OFF") << std::endl;
        }
        // Number keys 1-8 for direct scenario selection
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) {
            g_scenario_manager->setScenario(key - GLFW_KEY_1);
        }
    }
}

int main() {
    if (!glfwInit()) return -1;
    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Level 4 Autonomous Pilot (AI Planner)", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    // Initialize HMI Scene
    adas::hmi::Scene hmi_scene;
    hmi_scene.init();
    
    // AI Components
    adas::hmi::TrafficManager traffic_manager; 
    traffic_manager.init();
    
    adas::hmi::ScenarioManager scenario_manager;
    scenario_manager.init();
    g_scenario_manager = &scenario_manager; // For keyboard callback
    
    adas::hmi::TrajectoryPlanner planner;

    float sim_time = 0.0f;
    float ego_world_y = 0.0f; 
    float ego_speed = 60.0f;
    float ego_lane_center = 0.0f; 
    
    std::cout << "Level 4 Autonomy Initialized." << std::endl;
    std::cout << "Controls: [N/Right] Next | [P/Left] Prev | [1-8] Select | [Space] Toggle Auto" << std::endl;
    std::cout << ">>> Starting: " << scenario_manager.getCurrentConfig().name << std::endl;

    while (!glfwWindowShouldClose(window)) {
        sim_time += 0.016f;
        
        // Scenario Management
        scenario_manager.update(0.016f);
        const auto& cfg = scenario_manager.getCurrentConfig();
        scenario_manager.configureTraffic(traffic_manager, ego_world_y);
        
        float curve_val = cfg.curve_intensity * 100.0f * std::sin(sim_time * 0.05f);
        
        // 1. Traffic Simulation
        traffic_manager.update(0.016f, ego_world_y, ego_lane_center);
        
        // 2. AI Planning (Think!)
        // Plan based on current state
        planner.plan(ego_speed, ego_lane_center, ego_world_y, traffic_manager.agents, traffic_manager.light_state, traffic_manager.stop_line_y);
        
        // 3. Execution (Drive!)
        // Simple controller to follow best path
        if (planner.best_path.points_x.size() > 1) {
             // Lateral Control: Move towards first few points of path
             float target_x = planner.best_path.points_x[5]; // Lookahead
             float diff = target_x - ego_lane_center;
             ego_lane_center += diff * 0.05f; // Converge
             
             // Longitudinal Control
             float target_v = planner.best_path.target_speed;
             float v_diff = target_v - ego_speed;
             ego_speed += v_diff * 0.02f; // Smooth accel
             
             // Clamp Speed
             if(ego_speed < 0) ego_speed = 0;
        }
        
        // Update Physics
        // ego_speed is in km/h, convert to m/s: divide by 3.6
        // dt = 0.016s (60fps)
        // world_y is in meters
        float dt = 0.016f;
        float speed_mps = ego_speed / 3.6f; // km/h to m/s
        ego_world_y += speed_mps * dt;

        // 4. Rendering Prep
        adas::hud::ObjectList obj_list;
        obj_list.count = 0;
        
        // Main Agents
        for(const auto& agent : traffic_manager.agents) {
            float rel_y = agent.y - ego_world_y;
            if (rel_y > -20.0f && rel_y < 200.0f) { 
                 float render_x = agent.x - ego_lane_center;
                 float curve_at_obj = curve_val * (rel_y / 100.0f);
                 render_x += curve_at_obj;
                 float yaw = std::atan(curve_val / 100.0f);
                 // Pass class_id correctly (x, y, w, h, class_id, yaw)
                 obj_list.objects[obj_list.count++] = { render_x, rel_y, 2.0f, agent.length, (uint32_t)agent.class_id, yaw };
                 if(obj_list.count >= 20) break;
            }
        }
        
        // Cross Traffic
        for(const auto& ct : traffic_manager.cross_traffic) {
            if(obj_list.count >= 20) break;
            float rel_y = ct.y - ego_world_y;
            if(rel_y > 0 && rel_y < 200.0f) {
                obj_list.objects[obj_list.count++] = { ct.x, rel_y, 2.0f, 1.5f, 0, 1.57f };
            }
        }
        
        adas::hud::EgoState ego;
        ego.speed_kph = ego_speed;
        ego.speed_limit = (traffic_manager.light_state == adas::hmi::TrafficLightState::RED) ? 0.0f : cfg.speed_limit;
        ego.autopilot_engaged = true; // Level 4 Active
        ego.scenario_index = scenario_manager.current_scenario;
        ego.world_y = ego_world_y;
        ego.lane_x = ego_lane_center;

        // Lanes 
        adas::hud::LaneNetwork lanes;
        lanes.count = 4; 
        for(int l=0; l<4; ++l) {
            float base = ((l - 1.5f) * 3.5f) - ego_lane_center; 
            lanes.lanes[l].point_count = 50;
            for(int i=0; i<50; ++i) {
                float dist = i * 2.0f; 
                float curve = curve_val * (dist/100.0f); 
                lanes.lanes[l].points[i] = { base + curve, dist };
            }
        }
        
        // Path Ribbon (Render Planning Candidates!)
        std::vector<float> pathPts;
        const auto& bp = planner.best_path;
        if (bp.points_x.size() > 0) {
            for(size_t i=0; i < bp.points_x.size(); ++i) {
                float dist = bp.points_y[i]; // Forward Dist
                // Transform to Render Space
                float world_x = bp.points_x[i];
                float render_x = world_x - ego_lane_center;
                
                float curve = curve_val * (dist/100.0f);
                pathPts.push_back(render_x + curve);
                pathPts.push_back(-dist);
            }
        }
        adas::hmi::MeshGenerators::updateRibbonMesh(hmi_scene.pathRibbonMesh, pathPts, 2.8f);
        
        // Signs (AI Decision + Traffic Light)
        adas::hud::TrafficSignList signs; signs.count = 0;
        
        // AI Debug Text (Hack: Output to console)
        static int frame = 0;
        if(frame++ % 60 == 0) std::cout << "AI Decision: " << planner.decision_text << std::endl;

        if (traffic_manager.light_state == adas::hmi::TrafficLightState::RED) {
            signs.signs[signs.count++] = { 5.0f, 20.0f, 0, 5, 0 }; // Red Light
        }
        
        // --- DATA SIMULATION FOR SHOWCASE ---
        static float sign_timer = 0.0f;
        static int current_limit = 50;
        static int show_stop = 0; // 0=None, >0=Frames
        
        sign_timer += 0.016f;
        
        // Change Speed Limit every 5 seconds
        if (sign_timer > 5.0f) {
            sign_timer = 0.0f;
            int limits[] = {30, 50, 80, 50, 100, 120};
            current_limit = limits[rand()%6];
            
            // Randomly flash a STOP sign
            if (rand()%3 == 0) show_stop = 120; // Show for 2 seconds (60fps)
        }
        
        if (show_stop > 0) {
            signs.signs[signs.count++] = { 10.0f, 20.0f, 0, 1, 0 }; // STOP Sign
            show_stop--;
        } else {
            // Always show current speed limit if no stop/red light
            if (signs.count < 8) {
                signs.signs[signs.count++] = { 15.0f, 20.0f, 0, 0, current_limit };
            }
        } 

        int w, h; glfwGetFramebufferSize(window, &w, &h);
        hmi_scene.update(ego, obj_list, lanes, signs);
        hmi_scene.render(w, h, ego, obj_list, signs);

        glfwSwapBuffers(window);
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
