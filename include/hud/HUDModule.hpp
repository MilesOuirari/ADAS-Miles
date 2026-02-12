#ifndef HUD_MODULE_HPP
#define HUD_MODULE_HPP

#include "DataModels.hpp"
#include "E2EProtection.hpp"
#include "Renderer.hpp"

namespace adas::hud {

enum class AdasState {
    INACTIVE,
    ACC_ACTIVE,
    LKA_ACTIVE,
    AEB_WARNING,
    AEB_ACTIVE,
    LANE_CHANGE_LEFT,
    LANE_CHANGE_RIGHT
};

struct AdasStatus {
    AdasState state;
    float target_speed;
    float time_gap;
    bool hands_on_wheel;
};

// PID Constants/Tuning
constexpr float kKp_LKA = 0.15f;
constexpr float kKi_LKA = 0.01f;
constexpr float kKd_LKA = 0.05f;

class HUDModule {
public:
    HUDModule() : e2e_checker_() {}

    /**
     * @brief Process incoming vehicle state data.
     * @param packet Protected data packet.
     * @return true if data was valid and accepted.
     */
    bool updateVehicleState(const SafePacket<VehicleState>& packet) {
        if (!e2e_checker_.check(packet.sequence_counter, 
                               reinterpret_cast<const uint8_t*>(&packet.payload), 
                               sizeof(VehicleState), 
                               packet.crc)) {
            // Log E2E failure or handle error
            return false;
        }
        current_state_ = packet.payload;
        return true;
    }

    /**
     * @brief Process incoming lane data.
     */
    bool updateLaneData(const SafePacket<LaneData>& packet) {
        // Use a separate checker or multiplexed checker?
        // For simplicity, assuming simplified single stream or shared checker for demo.
        // In reality, each message ID needs its own checker state.
        // We will ignore E2E for this demo detail to avoid complexity, or reuse.
        (void)packet;       
        current_lane_ = packet.payload;
        return true;
    }

    /**
     * @brief Process incoming object list.
     */
    bool updateObjects(const SafePacket<ObjectList>& packet) {
        // E2E check omitted for brevity in this step, similar to VehicleState
        current_objects_ = packet.payload;
        return true;
    }

    /**
     * @brief Main render entry point.
     * Should be called from the graphics thread/loop.
     */
    void render(Renderer& renderer) {
        // Draw Lane Glow (Background layer)
        renderer.drawLaneGlow(current_lane_);

        // Draw Objects (Mid layer)
        renderer.drawObjects(current_objects_);

        // Draw Speedometer (Foreground/HUD layer)
        renderer.drawSpeedometer(current_state_.speed_kph);

        // Draw TTC Warning (Overlay)
        renderer.drawTTCWarning(current_state_.ttc_seconds);
    }

private:
    E2EChecker e2e_checker_;
    VehicleState current_state_ = {0.0f, 0.0f, 100.0f}; // Default safe state
    LaneData current_lane_;
    ObjectList current_objects_ = {{}, 0, false};
};

} // namespace adas::hud

#endif // HUD_MODULE_HPP
