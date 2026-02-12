#ifndef ADAS_SWC_HUD_SWC_HPP
#define ADAS_SWC_HUD_SWC_HPP

/**
 * @file HudSWC.hpp
 * @brief Software Component for the HUD overlay rendering.
 *
 * Consumes vehicle state and object data (with E2E protection)
 * and renders HUD elements (speedometer, TTC warnings, etc.)
 */

#include "core/ComponentBase.hpp"
#include "core/Port.hpp"
#include "data/DataModels.hpp"
#include "safety/E2EProtection.hpp"

namespace adas {

class HudSWC : public ComponentBase {
public:
    HudSWC();

    // ─── Ports (Input) ─────────────────────────────────────────────────
    ReceiverPort<EgoState>    port_ego_state;
    ReceiverPort<ObjectList>  port_object_list;

protected:
    bool onInit() override;
    bool onConfigure() override;
    void onStep(float dt) override;
    void onShutdown() override;

private:
    E2EChecker    e2e_checker_;
    VehicleState  current_vehicle_state_;
    AdasStatus    current_status_;
};

} // namespace adas

#endif // ADAS_SWC_HUD_SWC_HPP
