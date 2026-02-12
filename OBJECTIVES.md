# ADAS HMI Professional Upgrade Objectives

> **Goal**: Create a Tesla-competitive ADAS visualization system.

---

## Phase 1: High-Fidelity 3D Models (P0)

| Status | Item | Description |
|--------|------|-------------|
| [ ] | **OBJ Loader** | Integrate `tinyobjloader` for .obj file support |
| [ ] | **Vehicle Models** | Import/Create low-poly Car, Truck, Bus, Bike models |
| [ ] | **PBR Lighting** | Enhanced shader with metallic/roughness |

---

## Phase 2: Autopilot Status UI (P0)

| Status | Item | Description |
|--------|------|-------------|
| [ ] | **Status Bar** | Central "Autopilot Engaged / Manual" indicator |
| [ ] | **ACC Target** | Highlight lead vehicle with icon/link |
| [ ] | **Hands-on Warning** | Steering wheel icon with alert state |
| [ ] | **AEB Flash** | Red screen border on emergency braking |

---

## Phase 3: Object Detection Visualization (P1)

| Status | Item | Description |
|--------|------|-------------|
| [ ] | **Bounding Boxes** | Wireframe boxes around detected objects |
| [ ] | **Object Labels** | Floating text ("Car", "Truck", "Person") |
| [ ] | **Confidence Viz** | Color-coded detection confidence |

---

## Phase 4: Lane Visualization (P1)

| Status | Item | Description |
|--------|------|-------------|
| [ ] | **Lane Shading** | Semi-transparent filled lane polygons |
| [ ] | **Ego Lane Highlight** | Distinct color for current lane |
| [ ] | **Lane Confidence** | Visual solid/dashed based on confidence |

---

## Phase 5: Navigation UI (P2)

| Status | Item | Description |
|--------|------|-------------|
| [ ] | **Turn Arrows** | Directional arrows on planned path |
| [ ] | **Maneuver Text** | "Lane Change Left in 200m" overlay |
| [ ] | **Animated Path** | Pulsing highlight effect |

---

## Phase 6: Visual Polish (P2)

| Status | Item | Description |
|--------|------|-------------|
| [ ] | **Smooth Transitions** | Camera easing, object fade-in/out |
| [ ] | **Day/Night Mode** | Time-of-day lighting switch |
| [ ] | **UI Hover Effects** | Interactive element feedback |

---

## Phase 7: Sensor Visualization (P3)

| Status | Item | Description |
|--------|------|-------------|
| [ ] | **360° Mini-Map** | Top-down radar sweep widget |
| [ ] | **Ultrasonic Zones** | Parking sensor visualization |

---

## Tech Stack

- **3D Rendering**: OpenGL 3.3+ Core Profile
- **Model Loading**: tinyobjloader (header-only)
- **Textures**: stb_image (header-only)
- **Math**: GLM
- **UI**: Custom GLSL shaders (Glassmorphism)

---

## Success Criteria

- [ ] Visually indistinguishable from Tesla visualization quality
- [ ] Smooth 60 FPS performance
- [ ] Clear, intuitive status indicators
- [ ] Professional-grade object detection display
