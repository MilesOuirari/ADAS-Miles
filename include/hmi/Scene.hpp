#ifndef HMI_SCENE_HPP
#define HMI_SCENE_HPP

#include <vector>
#include <iostream> 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "stb/stb_image.h" // Required for texture loading

#include "ShaderStore.hpp"
#include "MeshGenerators.hpp"
#include "Camera.hpp"
#include "OBJLoader.hpp"
#include "ScenarioVisuals.hpp"
#include "../hud/DataModels.hpp"

namespace adas::hmi {

class Scene {
public:
    ShaderStore shaders;
    Mesh carMesh;
    Mesh busMesh; // NEW
    Mesh truckMesh; // NEW
    Mesh bikeMesh; // NEW
    Mesh buildingMesh; // NEW
    Mesh pedMesh;
    Mesh signPostMesh;
    Mesh signBoardMesh;
    Mesh gridMesh;
    Mesh pathRibbonMesh;
    Mesh quadMesh;
    Mesh bboxMesh; // NEW: Wireframe bounding box
    Mesh laneAreaMesh; // NEW: Ego lane filled polygon
    Mesh chevronMesh; // NEW: Turn arrow
    
    // Realistic Road Markings
    Mesh roadSurfaceMesh;
    Mesh dashedLaneLeft;
    Mesh dashedLaneRight;
    Mesh solidEdgeLeft;
    Mesh solidEdgeRight;
    
    std::vector<Mesh> laneMeshes;
    
    // Navigation State
    int currentManeuver = 0; // 0=Keep, 1=Left, 2=Right
    Camera camera;
    
    // Texture IDs
    GLuint texStop = 0;
    GLuint texLimit30 = 0;
    GLuint texLimit50 = 0;
    GLuint texLimit80 = 0;
    GLuint texLimit100 = 0;
    GLuint texLimit120 = 0;
    GLuint texLightRed = 0;
    
    // Object Label Textures
    GLuint texLabelCar = 0;
    GLuint texLabelTruck = 0;
    GLuint texLabelBus = 0;
    GLuint texLabelBike = 0;
    GLuint texLabelPerson = 0;
    // Maneuver Textures
    GLuint texManeuverKeep = 0;
    GLuint texManeuverLeft = 0;
    GLuint texManeuverRight = 0;
    
    // Scenario Name Textures
    GLuint texScenarios[8] = {0};

    void init() {
        shaders.init();
        
        // Load Textures
        texStop = loadTexture("assets/textures/stop_sign.jpg"); // JPG
        texLimit30 = loadTexture("assets/textures/limit_30.png");
        texLimit50 = loadTexture("assets/textures/limit_50.png");
        texLimit80 = loadTexture("assets/textures/limit_80.png");
        texLimit100 = loadTexture("assets/textures/limit_100.png");
        texLimit120 = loadTexture("assets/textures/limit_120.png");
        texLightRed = loadTexture("assets/textures/light_red.png");
        
        // Load Label Textures
        texLabelCar = loadTexture("assets/textures/labels/car.png");
        texLabelTruck = loadTexture("assets/textures/labels/truck.png");
        texLabelBus = loadTexture("assets/textures/labels/bus.png");
        texLabelBike = loadTexture("assets/textures/labels/bike.png");
        texLabelPerson = loadTexture("assets/textures/labels/person.png");
        
        // Load Maneuver Textures
        texManeuverKeep = loadTexture("assets/textures/maneuvers/keep_lane.png");
        texManeuverLeft = loadTexture("assets/textures/maneuvers/lane_left.png");
        texManeuverRight = loadTexture("assets/textures/maneuvers/lane_right.png");
        
        // Load Scenario Name Textures
        texScenarios[0] = loadTexture("assets/textures/scenarios/highway.png");
        texScenarios[1] = loadTexture("assets/textures/scenarios/city.png");
        texScenarios[2] = loadTexture("assets/textures/scenarios/emergency.png");
        texScenarios[3] = loadTexture("assets/textures/scenarios/lane_change.png");
        texScenarios[4] = loadTexture("assets/textures/scenarios/jam.png");
        texScenarios[5] = loadTexture("assets/textures/scenarios/pedestrian.png");
        texScenarios[6] = loadTexture("assets/textures/scenarios/intersection.png");
        texScenarios[7] = loadTexture("assets/textures/scenarios/merge.png");
        
        // Load High-Fidelity OBJ Models (Kenney Car Kit)
        carMesh = loadOBJ("assets/models/kenney_cars/Models/OBJ format/sedan.obj");
        truckMesh = loadOBJ("assets/models/kenney_cars/Models/OBJ format/truck.obj");
        busMesh = loadOBJ("assets/models/kenney_cars/Models/OBJ format/delivery.obj"); // Use delivery van as bus
        bikeMesh = MeshGenerators::createMotorcycle(); // Keep procedural for now
        buildingMesh = MeshGenerators::createBuilding();
        pedMesh = MeshGenerators::createPedestrian();
        signPostMesh = MeshGenerators::createSignPost();
        signBoardMesh = MeshGenerators::createSignBoard();
        gridMesh = MeshGenerators::createGrid(200.0f, 40);
        pathRibbonMesh = MeshGenerators::createDynamicRibbon(100);
        quadMesh = MeshGenerators::createQuad();
        bboxMesh = MeshGenerators::createBoundingBox();
        laneAreaMesh = MeshGenerators::createLaneArea(50);
        chevronMesh = MeshGenerators::createChevron(); 
        
        // Realistic Road Markings
        float roadLength = 300.0f;
        float roadWidth = 14.0f; // 4 lanes @ 3.5m each
        roadSurfaceMesh = MeshGenerators::createRoadSurface(roadWidth, roadLength);
        
        // Dashed lane dividers (3m dash, 6m gap, 15cm width)
        dashedLaneLeft = MeshGenerators::createDashedLaneMarking(roadLength, 3.0f, 6.0f, 0.15f);
        dashedLaneRight = MeshGenerators::createDashedLaneMarking(roadLength, 3.0f, 6.0f, 0.15f);
        
        // Solid edge lines (continuous, 20cm width)
        solidEdgeLeft = MeshGenerators::createSolidLaneMarking(roadLength, 0.20f);
        solidEdgeRight = MeshGenerators::createSolidLaneMarking(roadLength, 0.20f);
        
        for(int i=0; i<8; ++i) {
            laneMeshes.push_back(MeshGenerators::createDynamicLane(100));
        }
    }

    void update(const hud::EgoState& ego, const hud::ObjectList& objects, const hud::LaneNetwork& lanes, const hud::TrafficSignList& signs) {
        // Camera follows ego vehicle - convert world_y to Z (forward is -Z)
        glm::vec3 egoPos(ego.lane_x, 0.0f, -ego.world_y);
        camera.updateFollow(egoPos, 0.0f);
        updateLanes(lanes);
    }

    void render(int width, int height, const hud::EgoState& ego, const hud::ObjectList& objects, const hud::TrafficSignList& signs) {
        float aspect = (float)width / height;
        if (height == 0) aspect = 1.0f;

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(aspect);

        glViewport(0, 0, width, height);
        
        // ============ SCENARIO-BASED VISUALS ============
        ScenarioVisuals visuals = getScenarioVisuals(ego.scenario_index);
        
        // For Emergency Brake - add pulsing effect
        float pulseEffect = 1.0f;
        if (ego.scenario_index == 2) { // Emergency Brake
            pulseEffect = 0.7f + 0.3f * std::sin(glfwGetTime() * 8.0f);
        }
        
        // Sky color with scenario gradient
        glm::vec3 skyMix = glm::mix(visuals.skyColorBottom, visuals.skyColorTop, 0.5f);
        glClearColor(skyMix.r * pulseEffect, skyMix.g * pulseEffect, skyMix.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);

        // --- 3D Scene ---
        // Get camera Z position for road tiling
        float camZ = camera.position.z;
        
        // ========== REALISTIC ROAD RENDERING ==========
        glUseProgram(shaders.carProgram); // Use car shader for solid colors
        setMat4(shaders.carProgram, "view", view);
        setMat4(shaders.carProgram, "projection", proj);
        
        // Road Surface (dark asphalt)
        // Tile the road to follow the camera (camera looks at -Z)
        float roadLength = 300.0f;
        int roadTileZ = (int)(camZ / roadLength);
        
        for (int tile = -2; tile <= 1; ++tile) {
            float tileOffset = (roadTileZ + tile) * roadLength;
            glm::mat4 roadModel = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, tileOffset));
            setMat4(shaders.carProgram, "model", roadModel);
            // Asphalt color - dark grey with slight blue tint
            glUniform3f(glGetUniformLocation(shaders.carProgram, "color"), 
                        visuals.roadColor.r, visuals.roadColor.g, visuals.roadColor.b);
            roadSurfaceMesh.draw();
            
            // Lane Markings - WHITE dashed center dividers
            float laneWidth = 3.5f;
            
            // Left lane divider (between left and center-left lanes)
            glm::mat4 lineModel = glm::translate(roadModel, glm::vec3(-laneWidth, 0, 0));
            setMat4(shaders.carProgram, "model", lineModel);
            glUniform3f(glGetUniformLocation(shaders.carProgram, "color"), 
                        visuals.laneLineColor.r, visuals.laneLineColor.g, visuals.laneLineColor.b);
            dashedLaneLeft.draw();
            
            // Right lane divider (between center-right and right lanes)
            lineModel = glm::translate(roadModel, glm::vec3(laneWidth, 0, 0));
            setMat4(shaders.carProgram, "model", lineModel);
            dashedLaneRight.draw();
            
            // Center divider (between left and right traffic) - YELLOW solid
            lineModel = glm::translate(roadModel, glm::vec3(0, 0.005f, 0));
            setMat4(shaders.carProgram, "model", lineModel);
            glUniform3f(glGetUniformLocation(shaders.carProgram, "color"), 0.9f, 0.8f, 0.1f); // Yellow
            solidEdgeLeft.draw(); // Reuse as center line
            
            // Edge lines - SOLID WHITE on shoulders
            lineModel = glm::translate(roadModel, glm::vec3(-7.0f, 0, 0));
            setMat4(shaders.carProgram, "model", lineModel);
            glUniform3f(glGetUniformLocation(shaders.carProgram, "color"), 1.0f, 1.0f, 1.0f); // White
            solidEdgeLeft.draw();
            
            lineModel = glm::translate(roadModel, glm::vec3(7.0f, 0, 0));
            setMat4(shaders.carProgram, "model", lineModel);
            solidEdgeRight.draw();
        }
        
        // Optional: Keep grid for peripheral areas
        glUseProgram(shaders.gridProgram);
        setMat4(shaders.gridProgram, "view", view);
        setMat4(shaders.gridProgram, "projection", proj);
        glUniform3f(glGetUniformLocation(shaders.gridProgram, "color"), 
                    visuals.gridColor.r * 0.3f, visuals.gridColor.g * 0.3f, visuals.gridColor.b * 0.3f);
        gridMesh.draw();
        
        // Old lane meshes (hidden - replaced by realistic markings)
        // for(const auto& m : laneMeshes) if(m.vertexCount > 0) m.draw();
        
        // --- Ego Lane Area Shading ---
        // Build left and right edge points for ego lane (lane index 1 and 2 in laneMeshes)
        if (laneMeshes.size() >= 4) {
            std::vector<float> leftPts, rightPts;
            // Lane 1 = left edge of ego, Lane 2 = right edge of ego
            const auto& leftLane = laneMeshes[1]; // Index 1
            const auto& rightLane = laneMeshes[2]; // Index 2
            
            // Generate points from lane data (simplified: use fixed width around center)
            for(int i = 0; i < 50; ++i) {
                float z = i * 2.0f;
                leftPts.push_back(-1.75f); leftPts.push_back(0.02f); leftPts.push_back(z);
                rightPts.push_back(1.75f); rightPts.push_back(0.02f); rightPts.push_back(z);
            }
            MeshGenerators::updateLaneArea(laneAreaMesh, leftPts, rightPts);
            
            glUseProgram(shaders.glassProgram);
            setMat4(shaders.glassProgram, "view", view);
            setMat4(shaders.glassProgram, "projection", proj);
            glm::mat4 laneModel = glm::mat4(1.0f);
            setMat4(shaders.glassProgram, "model", laneModel);
            // Use scenario lane area color
            glUniform4f(glGetUniformLocation(shaders.glassProgram, "uColor"), 
                        visuals.laneAreaColor.r, visuals.laneAreaColor.g, visuals.laneAreaColor.b, visuals.laneAreaAlpha);
            glUniform1f(glGetUniformLocation(shaders.glassProgram, "uBlur"), visuals.fogDensity * 0.5f);
            laneAreaMesh.draw();
        }
        
        glDepthMask(GL_FALSE); 
        glUseProgram(shaders.pathProgram);
        setMat4(shaders.pathProgram, "view", view);
        setMat4(shaders.pathProgram, "projection", proj);
        glUniform1f(glGetUniformLocation(shaders.pathProgram, "time"), glfwGetTime() * visuals.pathGlow);
        glUniform3f(glGetUniformLocation(shaders.pathProgram, "color"), 
                    visuals.pathColor.r, visuals.pathColor.g, visuals.pathColor.b);
        pathRibbonMesh.draw();
        glDepthMask(GL_TRUE);
        
        // --- Render Turn Arrows (Chevrons on Path) ---
        glUseProgram(shaders.carProgram); // Reuse car shader for simple colored mesh
        setMat4(shaders.carProgram, "view", view);
        setMat4(shaders.carProgram, "projection", proj);
        // Chevrons use path color
        glUniform3f(glGetUniformLocation(shaders.carProgram, "color"), 
                    visuals.pathColor.r * 0.9f, visuals.pathColor.g * 0.9f, visuals.pathColor.b * 0.9f);
        
        // Animate chevrons scrolling forward (in absolute world coords)
        float arrowScroll = std::fmod(glfwGetTime() * 5.0f, 10.0f);
        for(int i = 0; i < 8; ++i) {
            float chevDist = i * 10.0f + arrowScroll;
            glm::mat4 chevModel = glm::translate(glm::mat4(1.0f),
                glm::vec3(ego.lane_x, 0.1f, -(ego.world_y + chevDist)));
            chevModel = glm::rotate(chevModel, 3.14159f, glm::vec3(0,1,0));
            setMat4(shaders.carProgram, "model", chevModel);
            chevronMesh.draw();
        }

        glUseProgram(shaders.carProgram);
        setMat4(shaders.carProgram, "view", view);
        setMat4(shaders.carProgram, "projection", proj);
        glUniform3f(glGetUniformLocation(shaders.carProgram, "viewPos"), camera.position.x, camera.position.y, camera.position.z);
        
        // Ego car at its absolute world position
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
            glm::vec3(ego.lane_x, 0.0f, -ego.world_y));
        setMat4(shaders.carProgram, "model", model);
        glUniform3f(glGetUniformLocation(shaders.carProgram, "color"), 0.8f, 0.9f, 0.95f); 
        carMesh.draw();

        // --- Render Infinite Buildings ---
        // Render based on scenario building density
        float spacing = 40.0f / std::max(0.3f, visuals.buildingDensity);
        int camGridZ = (int)(camera.position.z / spacing);
        int buildingCount = (int)(15 * visuals.buildingDensity);
        
        for(int i = -3; i <= buildingCount; ++i) {
            float z = (camGridZ + i) * spacing;
            
            // Left Side
            glm::mat4 bModel = glm::translate(glm::mat4(1.0f), glm::vec3(-25.0f, 0.0f, z));
            setMat4(shaders.carProgram, "model", bModel);
            glUniform3f(glGetUniformLocation(shaders.carProgram, "color"), 
                        visuals.buildingColor.r, visuals.buildingColor.g, visuals.buildingColor.b);
            buildingMesh.draw();
            
            // Right Side
            bModel = glm::translate(glm::mat4(1.0f), glm::vec3(25.0f, 0.0f, z));
            setMat4(shaders.carProgram, "model", bModel);
            // Slightly varied color
            glUniform3f(glGetUniformLocation(shaders.carProgram, "color"), 
                        visuals.buildingColor.r * 0.95f, visuals.buildingColor.g * 1.05f, visuals.buildingColor.b);
            buildingMesh.draw();
        }

        // --- Detect ACC Target (Lead Vehicle in Ego Lane) ---
        int acc_target_idx = -1;
        float min_lead_dist = 999.0f;
        for(size_t i=0; i<objects.count; ++i) {
            const auto& obj = objects.objects[i];
            // Ego lane is roughly -2.0f to 2.0f lateral
            if (std::abs(obj.x) < 2.5f && obj.y > 5.0f && obj.y < min_lead_dist) {
                min_lead_dist = obj.y;
                acc_target_idx = (int)i;
            }
        }

        // --- Render Traffic Agents ---
        for(size_t i=0; i<objects.count; ++i) {
            const auto& obj = objects.objects[i];
            // obj.x/y are RELATIVE to ego; offset by ego pos for absolute rendering
            glm::mat4 objModel = glm::mat4(1.0f);
            objModel = glm::translate(objModel,
                glm::vec3(ego.lane_x + obj.x, 0.0f, -(ego.world_y + obj.y)));
            objModel = glm::rotate(objModel, obj.yaw, glm::vec3(0,1,0));
            setMat4(shaders.carProgram, "model", objModel);
            
            // Color based on role with scenario-specific highlights
            glm::vec3 c = visuals.vehicleHighlight;
            
            // Override for specific types
            if (obj.class_id == 1) c = glm::vec3(0.9f, 0.7f, 0.1f); // Bus (Yellow/Orange)
            if (obj.class_id == 2) c = glm::vec3(0.2f, 0.2f, 0.2f); // Truck (Dark)
            if (obj.class_id == 3) c = glm::vec3(0.1f, 0.8f, 0.1f); // Bike (Green-ish)
            if (obj.class_id == 4) c = glm::vec3(0.8f, 0.5f, 0.5f); // Pedestrian
            
            // ACC Target Override: Use scenario accent color
            if ((int)i == acc_target_idx) {
                c = visuals.accTargetColor;
            }
            
            glUniform3f(glGetUniformLocation(shaders.carProgram, "color"), c.x, c.y, c.z);
            
            // Draw correct mesh
            if (obj.class_id == 0) carMesh.draw();
            else if (obj.class_id == 1) busMesh.draw();
            else if (obj.class_id == 2) truckMesh.draw();
            else if (obj.class_id == 3) bikeMesh.draw();
            else if (obj.class_id == 4) pedMesh.draw();
            else carMesh.draw(); // Fallback
        }
        
        // --- Render Bounding Boxes (Detection Visualization) ---
        glUseProgram(shaders.bboxProgram);
        setMat4(shaders.bboxProgram, "view", view);
        setMat4(shaders.bboxProgram, "projection", proj);
        glUniform3f(glGetUniformLocation(shaders.bboxProgram, "camPos"), 
                    camera.position.x, camera.position.y, camera.position.z);
        
        for(size_t i=0; i<objects.count; ++i) {
            const auto& obj = objects.objects[i];
            glm::mat4 bboxModel = glm::mat4(1.0f);
            // Absolute position with half-dimension offsets
            bboxModel = glm::translate(bboxModel,
                glm::vec3(ego.lane_x + obj.x - obj.width/2, 0.0f,
                          -(ego.world_y + obj.y) - obj.height/2));
            bboxModel = glm::rotate(bboxModel, obj.yaw, glm::vec3(0,1,0));
            float h = (obj.class_id == 0) ? 1.5f : ((obj.class_id == 4) ? 1.8f : 3.0f);
            bboxModel = glm::scale(bboxModel, glm::vec3(obj.width, h, obj.height));
            setMat4(shaders.bboxProgram, "model", bboxModel);
            
            // Color: Use scenario accent for ACC target, HUD accent for others
            if ((int)i == acc_target_idx) {
                glUniform3f(glGetUniformLocation(shaders.bboxProgram, "color"), 
                            visuals.accTargetColor.r, visuals.accTargetColor.g, visuals.accTargetColor.b);
            } else {
                glUniform3f(glGetUniformLocation(shaders.bboxProgram, "color"), 
                            visuals.hudAccentColor.r * 0.7f, visuals.hudAccentColor.g * 0.7f, visuals.hudAccentColor.b * 0.7f);
            }
            bboxMesh.draw();
        }
        
        // --- Render Object Labels (Billboard) ---
        glUseProgram(shaders.textureProgram);
        setMat4(shaders.textureProgram, "projection", proj);
        setMat4(shaders.textureProgram, "model", glm::mat4(1.0f)); // Will be overridden
        glUniform1i(glGetUniformLocation(shaders.textureProgram, "uTex"), 0);
        glUniform4f(glGetUniformLocation(shaders.textureProgram, "uTint"), 1,1,1,1);
        glActiveTexture(GL_TEXTURE0);
        
        for(size_t i=0; i<objects.count; ++i) {
            const auto& obj = objects.objects[i];
            
            // Select label texture based on class_id
            GLuint labelTex = texLabelCar;
            float labelHeight = 2.0f; // Height above object
            if (obj.class_id == 0) { labelTex = texLabelCar; labelHeight = 1.8f; }
            else if (obj.class_id == 1) { labelTex = texLabelBus; labelHeight = 3.5f; }
            else if (obj.class_id == 2) { labelTex = texLabelTruck; labelHeight = 4.0f; }
            else if (obj.class_id == 3) { labelTex = texLabelBike; labelHeight = 1.5f; }
            else if (obj.class_id == 4) { labelTex = texLabelPerson; labelHeight = 2.0f; }
            
            // Billboard: Face camera
            float absX = ego.lane_x + obj.x;
            float absZ = -(ego.world_y + obj.y);
            glm::mat4 labelModel = glm::mat4(1.0f);
            labelModel = glm::translate(labelModel, glm::vec3(absX - 0.5f, labelHeight, absZ));
            // Extract camera right and up vectors for billboard effect
            glm::vec3 camRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
            glm::vec3 camUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
            // Billboard matrix
            labelModel[0] = glm::vec4(camRight * 1.0f, 0);
            labelModel[1] = glm::vec4(camUp * 0.25f, 0);
            labelModel[2] = glm::vec4(glm::cross(camRight, camUp), 0);
            labelModel[3] = glm::vec4(absX - 0.5f, labelHeight, absZ, 1);
            
            setMat4(shaders.textureProgram, "model", labelModel);
            setMat4(shaders.textureProgram, "view", view);
            glBindTexture(GL_TEXTURE_2D, labelTex);
            quadMesh.draw();
        }
        
        // --- 2D Glass UI Overlay ---
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE); 
        glUseProgram(shaders.glassProgram);
        
        glm::mat4 ortho = glm::ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
        setMat4(shaders.glassProgram, "projection", ortho);
        
        // Speed Panel (Bottom Center)
        glm::mat4 uiModel = glm::translate(glm::mat4(1.0f), glm::vec3(width/2 - 150, 20, 0));
        uiModel = glm::scale(uiModel, glm::vec3(300, 80, 1)); 
        setMat4(shaders.glassProgram, "model", uiModel);
        glUniform4f(glGetUniformLocation(shaders.glassProgram, "uColor"), 0.1f, 0.1f, 0.2f, 0.8f); // Dark Glass
        glUniform1f(glGetUniformLocation(shaders.glassProgram, "uBlur"), 1.0f);
        quadMesh.draw(); 
        
        // --- AUTOPILOT STATUS BAR (Top Center) ---
        glm::mat4 statusModel = glm::translate(glm::mat4(1.0f), glm::vec3(width/2 - 100, height - 50, 0));
        statusModel = glm::scale(statusModel, glm::vec3(200, 35, 1));
        setMat4(shaders.glassProgram, "model", statusModel);
        
        // Color: Use scenario HUD accent color if Autopilot, Grey if Manual
        if (ego.autopilot_engaged) {
            glUniform4f(glGetUniformLocation(shaders.glassProgram, "uColor"), 
                        visuals.hudAccentColor.r * 0.5f, visuals.hudAccentColor.g * 0.5f, visuals.hudAccentColor.b * 0.5f, 0.9f);
        } else {
            glUniform4f(glGetUniformLocation(shaders.glassProgram, "uColor"), 0.3f, 0.3f, 0.3f, 0.7f); // Grey Glass
        }
        glUniform1f(glGetUniformLocation(shaders.glassProgram, "uBlur"), 0.5f);
        quadMesh.draw();
        
        // --- VELOCITY BAR (High Tech) ---
        float speedRatio = ego.speed_kph / 120.0f; // Max 120
        if(speedRatio > 1.0f) speedRatio = 1.0f;
        
        // Bar Container (Darker Slot) - We just draw the High Tech Bar on top
        // The shader itself handles "Inactive" segments as dark grey
        
        glUseProgram(shaders.barProgram);
        glm::mat4 barModel = glm::translate(glm::mat4(1.0f), glm::vec3(width/2 - 130, 45, 0));
        barModel = glm::scale(barModel, glm::vec3(260, 20, 1));
        
        setMat4(shaders.barProgram, "projection", ortho);
        setMat4(shaders.barProgram, "model", barModel);
        glUniform1f(glGetUniformLocation(shaders.barProgram, "uProgress"), speedRatio);
        glUniform1f(glGetUniformLocation(shaders.barProgram, "uTime"), glfwGetTime());
        
        quadMesh.draw();
        
        // --- MANEUVER TEXT OVERLAY (Below Status Bar) ---
        glUseProgram(shaders.textureProgram);
        setMat4(shaders.textureProgram, "projection", ortho);
        glUniform1i(glGetUniformLocation(shaders.textureProgram, "uTex"), 0);
        glUniform4f(glGetUniformLocation(shaders.textureProgram, "uTint"), 1,1,1,1);
        glActiveTexture(GL_TEXTURE0);
        
        // Select maneuver texture based on state (cycle for demo)
        int maneuverDemo = (int)(glfwGetTime() / 3.0f) % 3;
        GLuint maneuverTex = texManeuverKeep;
        if (maneuverDemo == 1) maneuverTex = texManeuverLeft;
        else if (maneuverDemo == 2) maneuverTex = texManeuverRight;
        
        glBindTexture(GL_TEXTURE_2D, maneuverTex);
        glm::mat4 maneuverModel = glm::translate(glm::mat4(1.0f), glm::vec3(width/2 - 80, height - 95, 0));
        maneuverModel = glm::scale(maneuverModel, glm::vec3(160, 32, 1));
        setMat4(shaders.textureProgram, "model", maneuverModel);
        setMat4(shaders.textureProgram, "view", glm::mat4(1.0f)); // 2D - no view transform
        quadMesh.draw(); 
        
        // --- SCENARIO NAME DISPLAY (Bottom Left) ---
        int scenIdx = ego.scenario_index;
        if (scenIdx >= 0 && scenIdx < 8 && texScenarios[scenIdx] != 0) {
            glBindTexture(GL_TEXTURE_2D, texScenarios[scenIdx]);
            glm::mat4 scenModel = glm::translate(glm::mat4(1.0f), glm::vec3(20, height - 35, 0));
            scenModel = glm::scale(scenModel, glm::vec3(200, 28, 1));
            setMat4(shaders.textureProgram, "model", scenModel);
            // Tint with scenario accent color
            glUniform4f(glGetUniformLocation(shaders.textureProgram, "uTint"), 
                        visuals.hudAccentColor.r, visuals.hudAccentColor.g, visuals.hudAccentColor.b, 1.0f);
            quadMesh.draw();
            // Reset tint
            glUniform4f(glGetUniformLocation(shaders.textureProgram, "uTint"), 1,1,1,1);
        } 
        
        // traffic sign logic
        int signType = -1;
        int signValue = 0;
        
        // Priority: RedLight (5) > Stop (1) > SpeedLimit (0)
        for(size_t i=0; i<signs.count; ++i) {
             if(signs.signs[i].type == 5) { signType = 5; signValue = 0; break; } // Red Light
             if(signs.signs[i].type == 1) { signType = 1; signValue = 0; }
             if(signs.signs[i].type == 0 && signType == -1) { signType = 0; signValue = signs.signs[i].value; }
        }
        
        // Background Panel (Glass)
        glUseProgram(shaders.glassProgram);
        uiModel = glm::translate(glm::mat4(1.0f), glm::vec3(20, height-120, 0));
        uiModel = glm::scale(uiModel, glm::vec3(100, 100, 1));
        setMat4(shaders.glassProgram, "model", uiModel);
        glUniform4f(glGetUniformLocation(shaders.glassProgram, "uColor"), 0.1f, 0.1f, 0.1f, 0.6f); // Grey Glass
        quadMesh.draw();
        
        // Sign Icon (Texture)
        GLuint texToBind = 0;
        
        if (signType == 5) texToBind = texLightRed;
        else if (signType == 1) texToBind = texStop;
        else if (signType == 0) {
            if(signValue <= 30) texToBind = texLimit30;
            else if(signValue <= 50) texToBind = texLimit50;
            else if(signValue <= 80) texToBind = texLimit80;
            else if(signValue <= 100) texToBind = texLimit100;
            else texToBind = texLimit120;
        }
        
        // FORCE FALLBACK: Always draw a sign for debugging
        if (texToBind == 0) {
            texToBind = texLimit50; // Default to 50 km/h sign
        }
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glUseProgram(shaders.textureProgram);
        // Center the icon in the panel (padding 10px)
        glm::mat4 iconModel = glm::translate(glm::mat4(1.0f), glm::vec3(30, height-110, 0));
        iconModel = glm::scale(iconModel, glm::vec3(80, 80, 1));
        
        setMat4(shaders.textureProgram, "projection", ortho);
        setMat4(shaders.textureProgram, "model", iconModel);
        glUniform4f(glGetUniformLocation(shaders.textureProgram, "uTint"), 1.0f, 1.0f, 1.0f, 1.0f);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texToBind);
        glUniform1i(glGetUniformLocation(shaders.textureProgram, "uTex"), 0);
        
        quadMesh.draw();
        
        glEnable(GL_CULL_FACE); 
    }

private:
    void setMat4(GLuint prog, const char* name, const glm::mat4& m) {
        glUniformMatrix4fv(glGetUniformLocation(prog, name), 1, GL_FALSE, glm::value_ptr(m));
    }
    
    GLuint loadTexture(const char* path) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        
        int width, height, nrComponents;
        unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ")" << std::endl;
        } else {
            std::cout << "Texture failed to load: " << path << std::endl;
        }
        return textureID;
    }

    void updateLanes(const hud::LaneNetwork& network) {
        for(auto& m : laneMeshes) m.vertexCount = 0;
        for(size_t i=0; i<network.count && i < laneMeshes.size(); ++i) {
            const auto& l = network.lanes[i];
             std::vector<float> pts;
             for(size_t j=0; j<l.point_count; ++j) {
                 pts.push_back(l.points[j].x); pts.push_back(0.05f); pts.push_back(-l.points[j].y);
             }
             MeshGenerators::updateLaneMesh(laneMeshes[i], pts);
        }
    }
};

} // namespace adas::hmi
#endif
