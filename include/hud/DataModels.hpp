#ifndef HUD_DATA_MODELS_HPP
#define HUD_DATA_MODELS_HPP

#include <cstdint>
#include <array>

namespace adas::hud {

/// Max points for a lane polyline to avoid dynamic allocation
constexpr size_t kMaxLanePoints = 100;

struct Point2D {
    float x;
    float y;
};

struct ObjectData {
    float x;           // lateral
    float y;           // longitudinal
    float width;
    float height;
    uint32_t class_id; // 0=Car, 1=Truck, 2=Pedestrian, 3=Bike
    float yaw;         // orientation
};

constexpr size_t kMaxObjects = 32; // Increased for traffic
struct ObjectList {
    std::array<ObjectData, kMaxObjects> objects;
    size_t count;
    bool is_valid;
};

// Lane Types: 0=Dashed, 1=Solid, 2=DoubleSolid, 3=Edge
struct LaneInfo {
    std::array<Point2D, kMaxLanePoints> points;
    size_t point_count;
    int type; 
    float color[3]; // r,g,b
};

struct LaneNetwork {
    // 0 = Ego Left, 1 = Ego Right, 2 = Far Left...
    std::array<LaneInfo, 4> lanes; 
    size_t count;
};

struct TrafficSign {
    float x, y, z;
    int type; // 0=SpeedLimit, 1=Stop, 2=Yield
    int value; // e.g. 50, 80
};

struct TrafficSignList {
    std::array<TrafficSign, 8> signs;
    size_t count;
};

struct EgoState {
    float speed_kph;
    float speed_limit;
    bool autopilot_engaged;
    int scenario_index = 0; // Current scenario (0-7)
    float world_y = 0.0f;   // Forward position in world
    float lane_x = 0.0f;    // Lateral position
};

struct VehicleState {
    float speed_kph;
    float yaw_rate;
    float ttc_seconds; // Time To Collision
};

/**
 * @brief Wrapper for data protected by E2E mechanism.
 * T must be trivially copyable (POD).
 */
template <typename T>
struct SafePacket {
    uint8_t sequence_counter; // 4-bit counter
    T payload;
    uint8_t crc;              // CRC-8 over the payload
};

} // namespace adas::hud

#endif // HUD_DATA_MODELS_HPP
