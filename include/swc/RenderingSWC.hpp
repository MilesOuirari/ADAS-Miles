#ifndef ADAS_SWC_RENDERING_SWC_HPP
#define ADAS_SWC_RENDERING_SWC_HPP

/**
 * @file RenderingSWC.hpp
 * @brief Software Component that wraps the OpenGL 3D rendering pipeline.
 *
 * Receives all visualization data via ReceiverPorts and delegates
 * to the existing Scene/ShaderStore/MeshGenerators system.
 *
 * The HMI rendering primitives (Camera, MeshGenerators, ShaderStore, etc.)
 * remain header-only as they are GPU-bound utility code.
 */

#include "core/ComponentBase.hpp"
#include "core/Port.hpp"
#include "data/DataModels.hpp"

// Forward-declare the HMI Scene to avoid pulling all GL headers into every TU
namespace adas { namespace hmi { class Scene; } }

struct GLFWwindow;

namespace adas {

class RenderingSWC : public ComponentBase {
public:
    explicit RenderingSWC(GLFWwindow* window);
    ~RenderingSWC() override;

    // ─── Ports (Input) ─────────────────────────────────────────────────
    ReceiverPort<EgoState>         port_ego_state;
    ReceiverPort<ObjectList>       port_object_list;
    ReceiverPort<LaneNetwork>      port_lane_network;
    ReceiverPort<TrafficSignList>  port_traffic_signs;
    ReceiverPort<Trajectory>       port_best_trajectory;
    ReceiverPort<ScenarioConfig>   port_scenario_config;

protected:
    bool onInit() override;
    bool onConfigure() override;
    void onStep(float dt) override;
    void onShutdown() override;

private:
    GLFWwindow* window_;
    hmi::Scene* scene_ = nullptr;   ///< Owned raw pointer (created in onInit)
    float32 sim_time_ = 0.0f;
};

} // namespace adas

#endif // ADAS_SWC_RENDERING_SWC_HPP
