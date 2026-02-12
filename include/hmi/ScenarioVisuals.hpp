#ifndef HMI_SCENARIO_VISUALS_HPP
#define HMI_SCENARIO_VISUALS_HPP

#include <glm/glm.hpp>

namespace adas::hmi {

/**
 * Complete visual configuration for each scenario.
 * Controls all rendering parameters for unique look per scenario.
 */
struct ScenarioVisuals {
    // Sky / Background
    glm::vec3 skyColorTop;
    glm::vec3 skyColorBottom;
    
    // Ground / Road
    glm::vec3 roadColor;
    glm::vec3 laneLineColor;
    glm::vec3 laneAreaColor;
    float laneAreaAlpha;
    
    // Environment
    glm::vec3 buildingColor;
    glm::vec3 buildingWindowColor;
    float buildingDensity; // 0-1
    
    // Lighting
    glm::vec3 ambientLight;
    glm::vec3 sunDirection;
    float fogDensity; // 0 = clear, 1 = thick fog
    glm::vec3 fogColor;
    
    // Path
    glm::vec3 pathColor;
    float pathGlow;
    
    // Objects
    glm::vec3 vehicleHighlight;
    glm::vec3 accTargetColor;
    
    // Grid
    glm::vec3 gridColor;
    float gridAlpha;
    
    // HUD accent
    glm::vec3 hudAccentColor;
};

/**
 * Get visual configuration for each scenario type.
 */
inline ScenarioVisuals getScenarioVisuals(int scenarioIndex) {
    ScenarioVisuals v;
    
    switch(scenarioIndex) {
        case 0: // Highway Cruise
            v.skyColorTop = glm::vec3(0.1f, 0.3f, 0.6f);    // Clear blue
            v.skyColorBottom = glm::vec3(0.4f, 0.6f, 0.9f);
            v.roadColor = glm::vec3(0.15f, 0.15f, 0.18f);   // Dark asphalt
            v.laneLineColor = glm::vec3(1.0f, 1.0f, 1.0f);  // White lanes
            v.laneAreaColor = glm::vec3(0.0f, 0.6f, 0.9f);  // Cyan path
            v.laneAreaAlpha = 0.2f;
            v.buildingColor = glm::vec3(0.3f, 0.35f, 0.4f); // Grey industry
            v.buildingWindowColor = glm::vec3(0.5f, 0.8f, 1.0f);
            v.buildingDensity = 0.3f;
            v.ambientLight = glm::vec3(1.0f, 0.95f, 0.9f);  // Warm sun
            v.sunDirection = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
            v.fogDensity = 0.0f;
            v.fogColor = glm::vec3(0.7f, 0.8f, 0.9f);
            v.pathColor = glm::vec3(0.0f, 0.9f, 1.0f);      // Cyan
            v.pathGlow = 0.8f;
            v.vehicleHighlight = glm::vec3(0.3f, 0.5f, 0.7f);
            v.accTargetColor = glm::vec3(0.2f, 0.6f, 1.0f); // Bright blue
            v.gridColor = glm::vec3(0.2f, 0.3f, 0.4f);
            v.gridAlpha = 0.3f;
            v.hudAccentColor = glm::vec3(0.0f, 0.8f, 1.0f); // Cyan
            break;
            
        case 1: // City Driving
            v.skyColorTop = glm::vec3(0.3f, 0.35f, 0.45f);  // Overcast
            v.skyColorBottom = glm::vec3(0.5f, 0.55f, 0.6f);
            v.roadColor = glm::vec3(0.12f, 0.12f, 0.14f);   // Urban asphalt
            v.laneLineColor = glm::vec3(0.9f, 0.9f, 0.5f);  // Yellow city lanes
            v.laneAreaColor = glm::vec3(0.3f, 0.7f, 0.3f);  // Green eco
            v.laneAreaAlpha = 0.25f;
            v.buildingColor = glm::vec3(0.45f, 0.4f, 0.35f); // Brick brown
            v.buildingWindowColor = glm::vec3(0.9f, 0.85f, 0.3f); // Warm lights
            v.buildingDensity = 0.9f;
            v.ambientLight = glm::vec3(0.8f, 0.8f, 0.85f);  // Cool overcast
            v.sunDirection = glm::normalize(glm::vec3(0.2f, 0.8f, 0.1f));
            v.fogDensity = 0.1f;
            v.fogColor = glm::vec3(0.5f, 0.5f, 0.55f);
            v.pathColor = glm::vec3(0.2f, 0.9f, 0.3f);      // Green
            v.pathGlow = 0.6f;
            v.vehicleHighlight = glm::vec3(0.4f, 0.5f, 0.4f);
            v.accTargetColor = glm::vec3(0.3f, 0.8f, 0.3f); // Green
            v.gridColor = glm::vec3(0.25f, 0.25f, 0.2f);
            v.gridAlpha = 0.4f;
            v.hudAccentColor = glm::vec3(0.3f, 0.9f, 0.4f); // Green
            break;
            
        case 2: // Emergency Brake
            v.skyColorTop = glm::vec3(0.5f, 0.1f, 0.1f);    // Red alert sky
            v.skyColorBottom = glm::vec3(0.3f, 0.15f, 0.1f);
            v.roadColor = glm::vec3(0.15f, 0.1f, 0.1f);     // Danger tint
            v.laneLineColor = glm::vec3(1.0f, 0.3f, 0.2f);  // Warning red
            v.laneAreaColor = glm::vec3(1.0f, 0.2f, 0.1f);  // Red danger zone
            v.laneAreaAlpha = 0.4f;
            v.buildingColor = glm::vec3(0.3f, 0.25f, 0.25f);
            v.buildingWindowColor = glm::vec3(1.0f, 0.5f, 0.3f);
            v.buildingDensity = 0.4f;
            v.ambientLight = glm::vec3(1.0f, 0.6f, 0.5f);   // Red warning light
            v.sunDirection = glm::normalize(glm::vec3(0.0f, 1.0f, -0.3f));
            v.fogDensity = 0.0f;
            v.fogColor = glm::vec3(0.8f, 0.4f, 0.3f);
            v.pathColor = glm::vec3(1.0f, 0.3f, 0.1f);      // Red warning
            v.pathGlow = 1.0f; // Bright flashing
            v.vehicleHighlight = glm::vec3(0.8f, 0.2f, 0.1f);
            v.accTargetColor = glm::vec3(1.0f, 0.1f, 0.1f); // Danger red
            v.gridColor = glm::vec3(0.4f, 0.15f, 0.1f);
            v.gridAlpha = 0.5f;
            v.hudAccentColor = glm::vec3(1.0f, 0.2f, 0.1f); // Red
            break;
            
        case 3: // Lane Change
            v.skyColorTop = glm::vec3(0.15f, 0.25f, 0.5f);  // Evening blue
            v.skyColorBottom = glm::vec3(0.4f, 0.45f, 0.7f);
            v.roadColor = glm::vec3(0.13f, 0.13f, 0.16f);
            v.laneLineColor = glm::vec3(1.0f, 0.9f, 0.4f);  // Yellow guidance
            v.laneAreaColor = glm::vec3(1.0f, 0.7f, 0.0f);  // Orange direction
            v.laneAreaAlpha = 0.3f;
            v.buildingColor = glm::vec3(0.35f, 0.35f, 0.4f);
            v.buildingWindowColor = glm::vec3(0.9f, 0.7f, 0.4f);
            v.buildingDensity = 0.5f;
            v.ambientLight = glm::vec3(0.9f, 0.85f, 0.8f);
            v.sunDirection = glm::normalize(glm::vec3(-0.3f, 0.8f, 0.2f));
            v.fogDensity = 0.05f;
            v.fogColor = glm::vec3(0.6f, 0.6f, 0.7f);
            v.pathColor = glm::vec3(1.0f, 0.6f, 0.0f);      // Orange
            v.pathGlow = 0.7f;
            v.vehicleHighlight = glm::vec3(0.6f, 0.5f, 0.3f);
            v.accTargetColor = glm::vec3(1.0f, 0.5f, 0.0f); // Orange target
            v.gridColor = glm::vec3(0.3f, 0.25f, 0.2f);
            v.gridAlpha = 0.35f;
            v.hudAccentColor = glm::vec3(1.0f, 0.6f, 0.0f); // Orange
            break;
            
        case 4: // Traffic Jam
            v.skyColorTop = glm::vec3(0.25f, 0.25f, 0.3f);  // Smoggy grey
            v.skyColorBottom = glm::vec3(0.35f, 0.35f, 0.4f);
            v.roadColor = glm::vec3(0.1f, 0.1f, 0.12f);     // Dark worn road
            v.laneLineColor = glm::vec3(0.6f, 0.6f, 0.5f);  // Faded lines
            v.laneAreaColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Grey queue
            v.laneAreaAlpha = 0.15f;
            v.buildingColor = glm::vec3(0.3f, 0.3f, 0.32f);
            v.buildingWindowColor = glm::vec3(0.8f, 0.75f, 0.6f);
            v.buildingDensity = 0.7f;
            v.ambientLight = glm::vec3(0.7f, 0.7f, 0.72f);  // Flat overcast
            v.sunDirection = glm::normalize(glm::vec3(0.1f, 1.0f, 0.0f));
            v.fogDensity = 0.3f; // Smog
            v.fogColor = glm::vec3(0.45f, 0.45f, 0.5f);
            v.pathColor = glm::vec3(0.6f, 0.6f, 0.7f);      // Muted
            v.pathGlow = 0.3f;
            v.vehicleHighlight = glm::vec3(0.4f, 0.4f, 0.45f);
            v.accTargetColor = glm::vec3(0.8f, 0.4f, 0.1f); // Warning orange
            v.gridColor = glm::vec3(0.2f, 0.2f, 0.22f);
            v.gridAlpha = 0.2f;
            v.hudAccentColor = glm::vec3(0.7f, 0.5f, 0.2f); // Amber
            break;
            
        case 5: // Pedestrian Crossing
            v.skyColorTop = glm::vec3(0.2f, 0.35f, 0.5f);   // Clear morning
            v.skyColorBottom = glm::vec3(0.5f, 0.65f, 0.8f);
            v.roadColor = glm::vec3(0.14f, 0.14f, 0.15f);
            v.laneLineColor = glm::vec3(1.0f, 1.0f, 1.0f);  // Zebra crossing
            v.laneAreaColor = glm::vec3(1.0f, 0.9f, 0.0f);  // Yellow caution
            v.laneAreaAlpha = 0.35f;
            v.buildingColor = glm::vec3(0.5f, 0.45f, 0.4f); // Warm residential
            v.buildingWindowColor = glm::vec3(0.95f, 0.9f, 0.8f);
            v.buildingDensity = 0.6f;
            v.ambientLight = glm::vec3(1.0f, 0.98f, 0.95f);
            v.sunDirection = glm::normalize(glm::vec3(0.3f, 1.0f, 0.4f));
            v.fogDensity = 0.0f;
            v.fogColor = glm::vec3(0.8f, 0.85f, 0.9f);
            v.pathColor = glm::vec3(1.0f, 0.8f, 0.0f);      // Yellow warning
            v.pathGlow = 0.6f;
            v.vehicleHighlight = glm::vec3(0.5f, 0.5f, 0.4f);
            v.accTargetColor = glm::vec3(1.0f, 0.8f, 0.2f); // Yellow
            v.gridColor = glm::vec3(0.25f, 0.25f, 0.2f);
            v.gridAlpha = 0.3f;
            v.hudAccentColor = glm::vec3(1.0f, 0.9f, 0.2f); // Yellow
            break;
            
        case 6: // Intersection
            v.skyColorTop = glm::vec3(0.15f, 0.2f, 0.4f);   // Dusk
            v.skyColorBottom = glm::vec3(0.4f, 0.35f, 0.5f);
            v.roadColor = glm::vec3(0.12f, 0.12f, 0.14f);
            v.laneLineColor = glm::vec3(0.9f, 0.9f, 0.9f);
            v.laneAreaColor = glm::vec3(0.4f, 0.6f, 0.9f);  // Blue safe zone
            v.laneAreaAlpha = 0.25f;
            v.buildingColor = glm::vec3(0.35f, 0.35f, 0.4f);
            v.buildingWindowColor = glm::vec3(1.0f, 0.9f, 0.5f); // Evening lights
            v.buildingDensity = 0.8f;
            v.ambientLight = glm::vec3(0.75f, 0.7f, 0.8f);  // Dusk purple
            v.sunDirection = glm::normalize(glm::vec3(-0.5f, 0.5f, 0.2f));
            v.fogDensity = 0.05f;
            v.fogColor = glm::vec3(0.5f, 0.45f, 0.55f);
            v.pathColor = glm::vec3(0.4f, 0.7f, 1.0f);      // Blue
            v.pathGlow = 0.5f;
            v.vehicleHighlight = glm::vec3(0.4f, 0.45f, 0.5f);
            v.accTargetColor = glm::vec3(0.3f, 0.6f, 1.0f);
            v.gridColor = glm::vec3(0.2f, 0.2f, 0.25f);
            v.gridAlpha = 0.35f;
            v.hudAccentColor = glm::vec3(0.5f, 0.7f, 1.0f); // Light blue
            break;
            
        case 7: // Highway Merge
            v.skyColorTop = glm::vec3(0.05f, 0.15f, 0.35f); // Deep blue dusk
            v.skyColorBottom = glm::vec3(0.3f, 0.4f, 0.6f);
            v.roadColor = glm::vec3(0.14f, 0.14f, 0.17f);
            v.laneLineColor = glm::vec3(0.9f, 0.95f, 1.0f);
            v.laneAreaColor = glm::vec3(0.6f, 0.3f, 0.9f);  // Purple merge indicator
            v.laneAreaAlpha = 0.3f;
            v.buildingColor = glm::vec3(0.25f, 0.28f, 0.35f);
            v.buildingWindowColor = glm::vec3(0.6f, 0.7f, 1.0f);
            v.buildingDensity = 0.2f;
            v.ambientLight = glm::vec3(0.7f, 0.75f, 0.9f);
            v.sunDirection = glm::normalize(glm::vec3(0.6f, 0.6f, -0.3f));
            v.fogDensity = 0.02f;
            v.fogColor = glm::vec3(0.5f, 0.55f, 0.7f);
            v.pathColor = glm::vec3(0.7f, 0.4f, 1.0f);      // Purple
            v.pathGlow = 0.8f;
            v.vehicleHighlight = glm::vec3(0.4f, 0.35f, 0.55f);
            v.accTargetColor = glm::vec3(0.8f, 0.4f, 1.0f); // Purple
            v.gridColor = glm::vec3(0.2f, 0.2f, 0.3f);
            v.gridAlpha = 0.3f;
            v.hudAccentColor = glm::vec3(0.8f, 0.5f, 1.0f); // Purple
            break;
            
        default:
            // Fallback - similar to Highway
            v = getScenarioVisuals(0);
            break;
    }
    
    return v;
}

} // namespace adas::hmi

#endif // HMI_SCENARIO_VISUALS_HPP
