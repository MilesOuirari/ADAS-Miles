#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>
#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "hud/DataModels.hpp"
#include "hmi/Scene.hpp"

void error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main() {
    if (!glfwInit()) return -1;
    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Tesla-Style ADAS HMI", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    // Initialize HMI Scene
    adas::hmi::Scene hmi_scene;
    hmi_scene.init();

    float sim_time = 0.0f;
    std::cout << "Tesla-Style HMI Initialized." << std::endl;

    while (!glfwWindowShouldClose(window)) {
        sim_time += 0.016f;

        // --- Simulation ---
        adas::hud::EgoState ego;
        ego.speed_kph = 60.0f + 10.0f * std::sin(sim_time * 0.2f);
        ego.speed_limit = 80.0f;

        // 1. Lanes (4 lines for 3 lanes)
        adas::hud::LaneNetwork lanes;
        lanes.count = 4; 
        for(int l=0; l<4; ++l) {
            float offset = (l - 1.5f) * 3.5f; // -5.25, -1.75, 1.75, 5.25 approx
            lanes.lanes[l].point_count = 50;
            for(int i=0; i<50; ++i) {
                float dist = i * 2.0f; 
                float curve = 10.0f * std::sin(dist * 0.01f + sim_time * 0.05f);
                lanes.lanes[l].points[i] = { offset + curve, dist };
            }
        }

        // 2. Objects (Traffic + Pedestrians)
        adas::hud::ObjectList obj_list;
        obj_list.count = 0;
        
        // Car Ahead (Same lane)
        obj_list.objects[obj_list.count++] = { 0.0f, 40.0f, 2.0f, 1.5f, 0, 0.0f };
        
        // Truck Right Lane
        obj_list.objects[obj_list.count++] = { 3.5f, 20.0f + sim_time*2.0f, 2.5f, 3.0f, 1, 0.0f };
        
        // Traffic Left Lane (Oncoming)
        obj_list.objects[obj_list.count++] = { -3.5f, 80.0f - (std::fmod(sim_time * 25.0f, 100.0f)), 2.0f, 1.5f, 0, 3.14f };

        // PEDESTRIAN CROSSING
        float pedY = 30.0f;
        float pedX = -10.0f + std::fmod(sim_time * 2.0f, 20.0f); // Walking Right
        if (pedX > -6.0f && pedX < 6.0f) {
             obj_list.objects[obj_list.count++] = { pedX, pedY, 0.5f, 1.7f, 2, 1.57f };
        }

        // 3. Signs
        adas::hud::TrafficSignList signs;
        signs.count = 0;
        // Periodic Stop Sign on right
        if (std::fmod(sim_time, 20.0f) > 15.0f) {
             float signDist = 50.0f - (std::fmod(sim_time, 20.0f) - 15.0f) * 10.0f; 
             signs.signs[signs.count++] = { 6.0f, signDist, 0, 1, 0 }; 
        }

        // --- Render Step ---
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        
        hmi_scene.update(ego, obj_list, lanes, signs);
        hmi_scene.render(width, height, ego, obj_list, signs);

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
