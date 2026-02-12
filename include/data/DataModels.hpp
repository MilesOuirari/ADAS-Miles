#ifndef ADAS_DATA_MODELS_HPP
#define ADAS_DATA_MODELS_HPP

/**
 * @file DataModels.hpp
 * @brief Shared data types and signal definitions for the ADAS system.
 *
 * All inter-component signals are defined here with their bus IDs.
 * Types use AUTOSAR-compliant aliases from TypeDefs.hpp.
 */

#include "core/TypeDefs.hpp"
#include <array>
#include <string>

namespace adas {

// ═══════════════════════════════════════════════════════════════════════════
//  Signal Bus IDs — used by SenderPort / ReceiverPort connections
// ═══════════════════════════════════════════════════════════════════════════

namespace signal {
    constexpr const char* EGO_STATE         = "EgoModel/EgoState";
    constexpr const char* OBJECT_LIST       = "EnvironmentModel/ObjectList";
    constexpr const char* LANE_NETWORK      = "EnvironmentModel/LaneNetwork";
    constexpr const char* TRAFFIC_SIGNS     = "EnvironmentModel/TrafficSigns";
    constexpr const char* BEST_TRAJECTORY   = "Planner/BestTrajectory";
    constexpr const char* SCENARIO_CONFIG   = "Scenario/Config";
    constexpr const char* PLANNER_DECISION  = "Planner/DecisionText";
} // namespace signal

// ═══════════════════════════════════════════════════════════════════════════
//  Geometry Primitives
// ═══════════════════════════════════════════════════════════════════════════

struct Point2D {
    float32 x = 0.0f;
    float32 y = 0.0f;
};

// ═══════════════════════════════════════════════════════════════════════════
//  Object Detection
// ═══════════════════════════════════════════════════════════════════════════

/// Object classification: matches model mesh indices
enum class ObjectClass : uint8 {
    CAR        = 0,
    TRUCK      = 1,
    PEDESTRIAN = 2,
    BIKE       = 3,
    BUS        = 4
};

struct ObjectData {
    float32  x         = 0.0f;   ///< Lateral position (meters)
    float32  y         = 0.0f;   ///< Longitudinal position (meters)
    float32  width     = 0.0f;
    float32  height    = 0.0f;
    uint32   class_id  = 0;      ///< @see ObjectClass
    float32  yaw       = 0.0f;   ///< Orientation (radians)
};

constexpr size_t kMaxObjects = 32;

struct ObjectList {
    std::array<ObjectData, kMaxObjects> objects{};
    size_t count    = 0;
    boolean is_valid = false;
};

// ═══════════════════════════════════════════════════════════════════════════
//  Lane Network
// ═══════════════════════════════════════════════════════════════════════════

constexpr size_t kMaxLanePoints = 100;

/// Lane marking types
enum class LaneType : uint8 {
    DASHED       = 0,
    SOLID        = 1,
    DOUBLE_SOLID = 2,
    EDGE         = 3
};

struct LaneInfo {
    std::array<Point2D, kMaxLanePoints> points{};
    size_t point_count = 0;
    int    type        = 0;
    float32 color[3]   = {1.0f, 1.0f, 1.0f};
};

struct LaneNetwork {
    std::array<LaneInfo, 4> lanes{};   ///< 0=EgoLeft, 1=EgoRight, 2=FarLeft, ...
    size_t count = 0;
};

// ═══════════════════════════════════════════════════════════════════════════
//  Traffic Signs
// ═══════════════════════════════════════════════════════════════════════════

enum class TrafficSignType : uint8 {
    SPEED_LIMIT = 0,
    STOP        = 1,
    YIELD       = 2,
    NO_ENTRY    = 3,
    TRAFFIC_LIGHT_RED    = 5,
    TRAFFIC_LIGHT_GREEN  = 6,
    TRAFFIC_LIGHT_YELLOW = 7
};

struct TrafficSign {
    float32 x = 0.0f, y = 0.0f, z = 0.0f;
    int type   = 0;     ///< @see TrafficSignType
    int value  = 0;     ///< e.g., speed limit value (50, 80, 120)
};

struct TrafficSignList {
    std::array<TrafficSign, 8> signs{};
    size_t count = 0;
};

// ═══════════════════════════════════════════════════════════════════════════
//  Ego Vehicle State
// ═══════════════════════════════════════════════════════════════════════════

struct EgoState {
    float32 speed_kph        = 0.0f;
    float32 speed_limit      = 0.0f;
    boolean autopilot_engaged = false;
    int     scenario_index   = 0;       ///< Current scenario (0–7)
    float32 world_y          = 0.0f;    ///< Forward position in world (meters)
    float32 lane_x           = 0.0f;    ///< Lateral position (meters)
};

struct VehicleState {
    float32 speed_kph   = 0.0f;
    float32 yaw_rate    = 0.0f;
    float32 ttc_seconds = 100.0f;   ///< Time To Collision
};

// ═══════════════════════════════════════════════════════════════════════════
//  Trajectory (Planner Output)
// ═══════════════════════════════════════════════════════════════════════════

constexpr size_t kMaxTrajectoryPoints = 50;

struct TrajectoryPoint {
    float32 x = 0.0f;    ///< Lateral
    float32 y = 0.0f;    ///< Longitudinal (relative to ego)
};

struct Trajectory {
    std::array<TrajectoryPoint, kMaxTrajectoryPoints> points{};
    size_t  point_count   = 0;
    float32 cost          = 0.0f;
    int     target_lane   = 0;
    float32 target_speed  = 0.0f;
    boolean valid         = false;
    std::string description;
};

// ═══════════════════════════════════════════════════════════════════════════
//  Scenario Configuration
// ═══════════════════════════════════════════════════════════════════════════

enum class ScenarioType : uint8 {
    HIGHWAY_CRUISE,
    CITY_DRIVING,
    EMERGENCY_BRAKE,
    LANE_CHANGE,
    TRAFFIC_JAM,
    PEDESTRIAN_CROSSING,
    INTERSECTION,
    HIGHWAY_MERGE,
    COUNT
};

struct ScenarioConfig {
    std::string name;
    ScenarioType type        = ScenarioType::HIGHWAY_CRUISE;
    float32 initial_speed    = 60.0f;
    float32 speed_limit      = 100.0f;
    int     traffic_density  = 5;       ///< 0–10
    boolean has_intersection = false;
    float32 curve_intensity  = 0.0f;    ///< Road curvature magnitude
    std::string description;
};

// ═══════════════════════════════════════════════════════════════════════════
//  E2E Protected Packet Wrapper
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Wrapper for data protected by E2E mechanism.
 * T must be trivially copyable (POD).
 */
template <typename T>
struct SafePacket {
    uint8 sequence_counter = 0;   ///< 4-bit counter
    T     payload{};
    uint8 crc = 0;                ///< CRC-8 over the payload
};

// ═══════════════════════════════════════════════════════════════════════════
//  ADAS Functional States
// ═══════════════════════════════════════════════════════════════════════════

enum class AdasState : uint8 {
    INACTIVE,
    ACC_ACTIVE,
    LKA_ACTIVE,
    AEB_WARNING,
    AEB_ACTIVE,
    LANE_CHANGE_LEFT,
    LANE_CHANGE_RIGHT
};

struct AdasStatus {
    AdasState state        = AdasState::INACTIVE;
    float32   target_speed = 0.0f;
    float32   time_gap     = 0.0f;
    boolean   hands_on_wheel = true;
};

// ═══════════════════════════════════════════════════════════════════════════
//  Traffic Light State
// ═══════════════════════════════════════════════════════════════════════════

enum class TrafficLightState : uint8 { GREEN, YELLOW, RED };

// ═══════════════════════════════════════════════════════════════════════════
//  Lane Data (legacy alias for HUD Renderer)
// ═══════════════════════════════════════════════════════════════════════════

struct LaneData {
    std::array<Point2D, kMaxLanePoints> left_points{};
    std::array<Point2D, kMaxLanePoints> right_points{};
    size_t point_count = 0;
    float32 confidence = 1.0f;
};

} // namespace adas

#endif // ADAS_DATA_MODELS_HPP
