# ADAS-Miles

A real-time autonomous driving simulator written in C++17. The project implements a full ADAS stack — traffic simulation, trajectory planning, and OpenGL-based HMI visualization — structured around an AUTOSAR-inspired Software Component (SWC) architecture. The goal was to build something that closely mirrors how production ADAS ECU software is organized, not just a graphics demo.

---

## Architecture

The system is split into five Software Components that communicate exclusively through a typed SignalBus, which acts as a Runtime Environment (RTE) analog. Each SWC inherits from `ComponentBase` and goes through a defined lifecycle (CREATED → INITIALIZED → CONFIGURED → RUNNING → SHUTDOWN). The Application orchestrator steps them in dependency order at ~60 Hz.

```
ScenarioSWC  →  EnvironmentModelSWC  →  PlannerSWC  →  RenderingSWC  →  HudSWC
     |                  |                    |               |
     └──────────────────┴────────────────────┴───────────────┘
                         SignalBus (RTE)
```

Inter-component communication uses typed `SenderPort<T>` / `ReceiverPort<T>` pairs backed by `SignalSlot<T>`, with atomic update flags for last-is-best semantics. Signal identifiers follow the `"ComponentName/SignalName"` convention. Type safety is enforced at runtime via `std::any` + `std::type_index`.

Platform types (`uint8`, `float32`, `E_OK`, etc.) mirror the AUTOSAR SWS_Platform specification to keep the codebase portable and industry-readable.

---

## Traffic Simulation (EnvironmentModelSWC)

The environment model runs a continuous multi-agent traffic simulation on a 4-lane highway (3.5 m per lane):

**Longitudinal control — IDM (Intelligent Driver Model)**

Each traffic agent computes its acceleration using:

```
a = a_max * (1 - (v / v_desired)^4 - (s_star / gap)^2)
```

where `s_star` is the desired following distance as a function of speed and approach rate. This produces smooth, realistic braking and acceleration without scripted behavior.

**Lane changes — MOBIL**

Agents evaluate lane-change decisions using incentive/safety trade-offs: a move is accepted when the acceleration gain exceeds a politeness threshold and the resulting gap to any rear vehicle stays within a safety margin.

**Traffic flow**

Vehicles spawn 200–300 m ahead of the ego and are removed 300 m behind it, creating an infinite road. A traffic light state machine (GREEN 15 s → YELLOW 3 s → RED 12 s) gates traffic at intersections. Eight predefined scenarios (highway cruise, urban congestion, emergency braking, lane change, traffic jam, pedestrian crossing, intersection, highway merge) are cycled automatically or triggered via keyboard.

---

## Planner (PlannerSWC)

The planner generates up to three trajectory candidates — Keep Lane, Lane Change Left, Lane Change Right — over a 4-second horizon (0.1 s timestep) and selects the minimum-cost option.

**Cost function**

| Term | Weight | Description |
|------|--------|-------------|
| Collision | 99999 | Hard rejection of any trajectory with predicted overlap |
| Efficiency | 1.0 | Penalizes delta from speed limit |
| Comfort | 10.0 | Penalizes lateral deviation |
| Lane discipline | 2.0 / 10.0 | Discourages leftmost lanes (keep-right rule) |

**Safety checks**

- Blind-spot TTC: For any rear vehicle within 60 m, relative speed is computed and the candidate is rejected if TTC < 3.5 s.
- Cut-in prediction: Vehicles within 2 m of the target lane boundary are treated as already-present obstacles before they fully merge.
- Curve speed adaptation: Target speed is reduced proportionally to road curvature.

The winning trajectory's target lane and speed are applied to the ego vehicle each frame via a P-controller (lateral) and ACC logic (longitudinal).

---

## Safety Layer (E2E Protection)

Critical signals are wrapped in `SafePacket<T>` carrying a 4-bit sequence counter and a CRC-8-SAE J1850 checksum (polynomial 0x1D, initial 0xFF, final XOR 0xFF). The `E2EChecker` validates each incoming frame and rejects messages with a CRC mismatch or a non-sequential counter.

Unit tests in `tests/test_e2e.cpp` cover: deterministic CRC, data-sensitivity, first-message acceptance, sequential acceptance, CRC rejection, counter-jump rejection, and reset behavior. The port/signal-bus layer has its own test suite in `tests/test_signal_bus.cpp` (6 tests including one-to-many routing and type-mismatch handling).

---

## Visualization (RenderingSWC + HudSWC)

The renderer reads published signals from the bus and drives an OpenGL 3.3 scene:

- Ego-centric chase camera with smooth vertical/longitudinal interpolation and a 5 m look-ahead offset
- Procedural 3D meshes for car, bus, truck, bike, pedestrian, sign posts, road surface, dashed/solid lane markings, path ribbon, and bounding boxes
- Custom GLSL shaders for geometry, glass HUD panels, road surface, traffic signs, and trajectory path
- HUD overlay: current speed, speed limit, autopilot status, object class labels, trajectory ribbon, traffic sign icons
- Texture atlas generated by Python scripts (PIL) for labels, traffic signs, maneuver arrows, and scenario names

---

## Build

**Dependencies:** CMake 3.16+, GCC/Clang with C++17, OpenGL 3.3, GLFW3, GLEW, GLM

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

**Targets**

| Target | Description |
|--------|-------------|
| `adas_miles` | Main binary (SWC architecture) |
| `adas_miles_legacy` | Monolithic version (earlier iteration) |
| `test_signal_bus` | Port and bus unit tests |
| `test_e2e` | E2E protection unit tests |

```bash
./adas_miles          # run the simulator
./test_signal_bus     # run port/bus tests
./test_e2e            # run safety layer tests
```

---

## Controls

| Key | Action |
|-----|--------|
| Space | Toggle auto-scenario cycling |
| 1 – 8 | Jump to specific scenario |
| N / Right | Next scenario |
| P / Left | Previous scenario |
| ESC | Exit |

---

*Selim Ouirari — 2026*
