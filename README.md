# ADAS-Miles Level 4 Autonomous Driving Simulator

**ADAS-Miles** is a high-fidelity traffic simulation and autonomous driving platform built from scratch in C++ using an AUTOSAR-inspired Software Component (SWC) architecture. It simulates complex highway and urban scenarios with realistic vehicle dynamics, sensor fusion, and advanced decision-making algorithms.

## 🚀 Key Features

### 1. Realistic Traffic Simulation
- **Physics-Based Models**: Uses the **IDM (Intelligent Driver Model)** for car-following behavior (acceleration/braking) and **MOBIL (Minimizing Overall Braking Induced by Lane changes)** for lane-change decisions.
- **Continuous Traffic Flow**: Implements a persistent traffic environment where vehicles spawn on the horizon (200-300m ahead) and are cleaned up behind the ego vehicle, creating an infinite driving experience.
- **Scenario Management**: Supports dynamic scenarios including:
  - Highway Cruising (4-lane, high speed)
  - Urban Congestion (Traffic jams)
  - Emergency Braking (Cut-ins)
  - Intersection Handling (Traffic lights with Green/Yellow/Red cycles)

### 2. Autonomous Stack (Level 4)
- **Perception Layer**: Simulates object detection (LiDAR/Radar/Camera fusion) via `ObjectList` and lane detection via `LaneNetwork`.
- **Planning & Decision Making**:
  - **Trajectory Generation**: Generates 4-second lookahead trajectories (polynomials) for multiple candidates (Keep Lane, Lane Change Left/Right).
  - **Cost-Based Selection**: Evaluates trajectories based on Safety (Collision), Efficiency (Speed), Comfort (Jerk), and Lane Discipline.
  - **Predictive Safety**:
    - **TTC (Time-To-Collision)**: Monitors blind spots and prevents lane changes if a rear vehicle is approaching fast (`TTC < 3.5s`).
    - **Cut-in Prediction**: Proactively detects adjacent vehicles merging into the ego lane and treats them as obstacles before they fully arrive.
  - **Speed Adaptation**: Automatically adjusts target speed based on road curvature (e.g., slowing down for sharp turns).
- **Control System**:
  - **Lateral Control**: P-Controller with high gain for decisive lane centering and lane changes.
  - **Longitudinal Control**: Adaptive Cruise Control (ACC) logic with smooth acceleration profiles.

### 3. Visualization & HMI
- **3D Rendering**: OpenGL-based rendering engine with custom shaders for lighting, shadows, and materials.
- **Ego-Centric View**: The camera is locked to the ego vehicle's lateral position, providing a stable "driver's eye" or "chase cam" perspective where the world moves around the car.
- **Head-Up Display (HUD)**: Real-time visualization of:
  - Ego Speed (km/h) & Gear
  - Object Bounding Boxes & Class Labels (Car, Truck, Pedestrian)
  - Trajectory Path Ribbon (Yellow curve showing planned path)
  - Lane Markings (curved based on road geometry)
  - Traffic Signs & Traffic Lights

## 🏗️ Architecture

The system follows a modular **Software Component (SWC)** design pattern, simulating an automotive ECU environment:

```
[ Application Orchestrator ]
       |
       v
[ Signal Bus (RTE) ]
       |
  +----+-----+----------------+-----------------+
  |          |                |                 |
[Scenario] [EnvModel]     [Planner]        [Rendering]
(Config)   (Physics)      (Logic)          (Visuals)
```

- **ScenarioSWC**: Manages high-level state (e.g., "Switch to Traffic Jam").
- **EnvironmentModelSWC**: The "World Sim". Handles all traffic agents, physics updates (IDM/MOBIL), and road geometry. Outputs `ObjectList` and `LaneNetwork`.
- **PlannerSWC**: The "Brain". Reads sensors, computes trajectories, selects the best path, and outputs a `Trajectory`.
- **Application**: The "ECU". Orchestrates the loop, steps components, and applies the planner's output to the ego vehicle dynamics.

## 🎮 Controls

| Key | Action |
|-----|--------|
| **Space** | Toggle **Auto-Scenario Cycling** (Traffic Jam -> Highway -> City...) |
| **1-8**   | Manually select specific scenarios |
| **N** / **Right** | Next Scenario |
| **P** / **Left** | Previous Scenario |
| **ESC**   | Exit Simulation |

## 🛠️ Build & Run

### Prerequisites
- Linux (Ubuntu 20.04/22.04 recommended)
- CMake 3.10+
- GCC/G++
- OpenGL / GLFW / GLEW
- GLM (Mathematics library)

### Compilation
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Running
```bash
./adas_miles
```

## 🚗 Physics & Logic Details

**Coordinate System**:
- All internal calculations use **SI Units (meters, m/s)**.
- **World Coordinates**: 
  - X: Lateral position (0 = road center). Lane centers at ±1.75m, ±5.25m.
  - Y: Longitudinal position (0 = start). Positive is forward.
  - Z: Up (0 = ground).
- **Planner Logic**:
  - **Keep-Right Rule**: Penalizes driving in left-most lanes (Lane 1 cost += 2.0, Land 0 cost += 10.0) to encourage proper highway discipline.
  - **Lane Change Incentive**: `W_LANE_CHANGE` cost (2.0) balances stability vs. efficiency (overtaking).

---
*Developed by Selim Ouirari for ADAS-Miles Project 2026.*
