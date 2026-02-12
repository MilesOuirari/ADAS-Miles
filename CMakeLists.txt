cmake_minimum_required(VERSION 3.16)
project(ADAS_Miles
    VERSION     1.0.0
    DESCRIPTION "Level 4 ADAS Visualization — AUTOSAR-inspired SWC Architecture"
    LANGUAGES   CXX
)

# ═══════════════════════════════════════════════════════════════════════════
#  C++ Standard
# ═══════════════════════════════════════════════════════════════════════════
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ═══════════════════════════════════════════════════════════════════════════
#  Compiler Warnings (strict, automotive-grade)
# ═══════════════════════════════════════════════════════════════════════════
add_compile_options(
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter      # Lifecycle hooks often have unused params
)

# ═══════════════════════════════════════════════════════════════════════════
#  Dependencies
# ═══════════════════════════════════════════════════════════════════════════
find_package(OpenGL REQUIRED)
find_package(GLEW   REQUIRED)
find_package(glfw3  REQUIRED)

# GLM (header-only) — try find_package first, fallback to system include
find_package(glm QUIET)
if(NOT glm_FOUND)
    message(STATUS "GLM not found via find_package, assuming system headers")
endif()

# ═══════════════════════════════════════════════════════════════════════════
#  Core Library (header-only infrastructure)
# ═══════════════════════════════════════════════════════════════════════════
add_library(adas_core INTERFACE)
target_include_directories(adas_core INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

# ═══════════════════════════════════════════════════════════════════════════
#  SWC Library (compiled components)
# ═══════════════════════════════════════════════════════════════════════════
add_library(adas_swc STATIC
    src/swc/ScenarioSWC.cpp
    src/swc/EnvironmentModelSWC.cpp
    src/swc/PlannerSWC.cpp
    src/swc/RenderingSWC.cpp
    src/swc/HudSWC.cpp
    src/app/Application.cpp
)
target_link_libraries(adas_swc PUBLIC
    adas_core
    OpenGL::GL
    GLEW::GLEW
    glfw
)
if(glm_FOUND)
    target_link_libraries(adas_swc PUBLIC glm::glm)
endif()

# ═══════════════════════════════════════════════════════════════════════════
#  Main Executable (new SWC architecture)
# ═══════════════════════════════════════════════════════════════════════════
add_executable(adas_miles src/main_swc.cpp)
target_link_libraries(adas_miles PRIVATE adas_swc)

# ═══════════════════════════════════════════════════════════════════════════
#  Legacy Executable (original monolithic main)
# ═══════════════════════════════════════════════════════════════════════════
add_executable(adas_miles_legacy src/main.cpp)
target_include_directories(adas_miles_legacy PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(adas_miles_legacy PRIVATE
    OpenGL::GL
    GLEW::GLEW
    glfw
)
if(glm_FOUND)
    target_link_libraries(adas_miles_legacy PRIVATE glm::glm)
endif()

# ═══════════════════════════════════════════════════════════════════════════
#  Tests
# ═══════════════════════════════════════════════════════════════════════════
enable_testing()

add_executable(test_signal_bus tests/test_signal_bus.cpp)
target_link_libraries(test_signal_bus PRIVATE adas_core)
add_test(NAME SignalBusTests COMMAND test_signal_bus)

add_executable(test_e2e tests/test_e2e.cpp)
target_link_libraries(test_e2e PRIVATE adas_core)
add_test(NAME E2ETests COMMAND test_e2e)

# ═══════════════════════════════════════════════════════════════════════════
#  Print Summary
# ═══════════════════════════════════════════════════════════════════════════
message(STATUS "")
message(STATUS "╔══════════════════════════════════════════════╗")
message(STATUS "║  ADAS-Miles Build Configuration              ║")
message(STATUS "╠══════════════════════════════════════════════╣")
message(STATUS "║  C++ Standard:  ${CMAKE_CXX_STANDARD}                          ║")
message(STATUS "║  Build Type:    ${CMAKE_BUILD_TYPE}                       ║")
message(STATUS "║  Targets:                                    ║")
message(STATUS "║    adas_miles        (SWC architecture)      ║")
message(STATUS "║    adas_miles_legacy (original monolithic)    ║")
message(STATUS "║    test_signal_bus   (port/bus unit tests)    ║")
message(STATUS "║    test_e2e          (E2E protection tests)  ║")
message(STATUS "╚══════════════════════════════════════════════╝")
message(STATUS "")
