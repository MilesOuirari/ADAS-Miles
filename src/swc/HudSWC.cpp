#include "swc/HudSWC.hpp"

namespace adas {

HudSWC::HudSWC()
    : ComponentBase("HudSWC") {}

bool HudSWC::onInit() {
    e2e_checker_.reset();
    current_vehicle_state_ = {};
    current_status_ = {};
    return true;
}

bool HudSWC::onConfigure() {
    return true;
}

void HudSWC::onStep(float /*dt*/) {
    const auto& ego = port_ego_state.read();

    // Update internal vehicle state from ego
    current_vehicle_state_.speed_kph = ego.speed_kph;

    // Determine ADAS state
    if (ego.autopilot_engaged) {
        current_status_.state = AdasState::ACC_ACTIVE;
    } else {
        current_status_.state = AdasState::INACTIVE;
    }

    // HUD rendering is handled by RenderingSWC (which calls Scene::render
    // which includes the HUD overlay). This SWC manages the state/logic
    // for HUD decisions (e.g., when to flash AEB warning).
    // In a full implementation, this would drive a separate HUD overlay renderer.
}

void HudSWC::onShutdown() {
    e2e_checker_.reset();
}

} // namespace adas
