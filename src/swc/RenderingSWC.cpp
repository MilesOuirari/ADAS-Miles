#include "swc/RenderingSWC.hpp"

// Pull in the actual HMI Scene (still header-only — GPU utility code)
#include "hmi/Scene.hpp"
#include "hmi/MeshGenerators.hpp"

#include <GLFW/glfw3.h>
#include <cmath>

namespace adas {

// ═══════════════════════════════════════════════════════════════════════════
//  Construction
// ═══════════════════════════════════════════════════════════════════════════

RenderingSWC::RenderingSWC(GLFWwindow* window)
    : ComponentBase("RenderingSWC"), window_(window) {}

RenderingSWC::~RenderingSWC() {
    delete scene_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Lifecycle
// ═══════════════════════════════════════════════════════════════════════════

bool RenderingSWC::onInit() {
    scene_ = new hmi::Scene();
    scene_->init();
    log_.info("OpenGL Scene initialized");
    return true;
}

bool RenderingSWC::onConfigure() {
    return scene_ != nullptr;
}

void RenderingSWC::onStep(float dt) {
    sim_time_ += dt;

    // --- Read inputs from ports ---
    const auto& ego   = port_ego_state.read();
    const auto& objs  = port_object_list.read();
    const auto& lanes = port_lane_network.read();
    const auto& signs = port_traffic_signs.read();
    const auto& traj  = port_best_trajectory.read();
    const auto& cfg   = port_scenario_config.read();

    // --- Convert new DataModels to legacy hud:: types for Scene ---
    // (Scene still uses the old hud:: types internally)
    adas::hud::EgoState legacy_ego;
    legacy_ego.speed_kph        = ego.speed_kph;
    legacy_ego.speed_limit      = ego.speed_limit;
    legacy_ego.autopilot_engaged = ego.autopilot_engaged;
    legacy_ego.scenario_index   = ego.scenario_index;
    legacy_ego.world_y          = ego.world_y;
    legacy_ego.lane_x           = ego.lane_x;

    adas::hud::ObjectList legacy_objs;
    legacy_objs.count = objs.count;
    legacy_objs.is_valid = objs.is_valid;
    for (size_t i = 0; i < objs.count && i < adas::hud::kMaxObjects; ++i) {
        legacy_objs.objects[i] = {
            objs.objects[i].x, objs.objects[i].y,
            objs.objects[i].width, objs.objects[i].height,
            objs.objects[i].class_id, objs.objects[i].yaw
        };
    }

    adas::hud::LaneNetwork legacy_lanes;
    legacy_lanes.count = lanes.count;
    for (size_t l = 0; l < lanes.count && l < 4; ++l) {
        legacy_lanes.lanes[l].point_count = lanes.lanes[l].point_count;
        legacy_lanes.lanes[l].type = lanes.lanes[l].type;
        for (int c = 0; c < 3; ++c)
            legacy_lanes.lanes[l].color[c] = lanes.lanes[l].color[c];
        for (size_t p = 0; p < lanes.lanes[l].point_count && p < adas::hud::kMaxLanePoints; ++p) {
            legacy_lanes.lanes[l].points[p] = {
                lanes.lanes[l].points[p].x,
                lanes.lanes[l].points[p].y
            };
        }
    }

    adas::hud::TrafficSignList legacy_signs;
    legacy_signs.count = signs.count;
    for (size_t i = 0; i < signs.count && i < 8; ++i) {
        legacy_signs.signs[i] = {
            signs.signs[i].x, signs.signs[i].y, signs.signs[i].z,
            signs.signs[i].type, signs.signs[i].value
        };
    }

    // --- Build path ribbon from trajectory (absolute world coords) ---
    std::vector<float> pathPts;
    float32 curve_val = cfg.curve_intensity * 100.0f * std::sin(sim_time_ * 0.05f);
    if (traj.valid && traj.point_count > 0) {
        for (size_t i = 0; i < traj.point_count; ++i) {
            float32 dist = traj.points[i].y;  // relative distance ahead
            float32 render_x = traj.points[i].x;  // lateral position
            float32 curve = curve_val * (dist / 100.0f);
            pathPts.push_back(ego.lane_x + render_x + curve);
            pathPts.push_back(-(ego.world_y + dist));  // absolute Z
        }
    }
    hmi::MeshGenerators::updateRibbonMesh(scene_->pathRibbonMesh, pathPts, 2.8f);

    // --- Render ---
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    scene_->update(legacy_ego, legacy_objs, legacy_lanes, legacy_signs);
    scene_->render(w, h, legacy_ego, legacy_objs, legacy_signs);
}

void RenderingSWC::onShutdown() {
    delete scene_;
    scene_ = nullptr;
    log_.info("OpenGL Scene destroyed");
}

} // namespace adas
