#ifndef ADAS_CORE_ICOMPONENT_HPP
#define ADAS_CORE_ICOMPONENT_HPP

/**
 * @file IComponent.hpp
 * @brief Abstract interface for all Software Components (SWCs).
 *
 * Defines the lifecycle contract inspired by AUTOSAR BSW Module lifecycle:
 *   CREATED → INITIALIZED → CONFIGURED → RUNNING → SHUTDOWN
 *
 * Each SWC must implement this interface. The Application orchestrator
 * calls lifecycle methods in the correct order.
 */

#include <string>

namespace adas {

/**
 * @brief Lifecycle states for a Software Component.
 *
 * State machine:
 *   CREATED → init() → INITIALIZED → configure() → CONFIGURED
 *   CONFIGURED → step() → RUNNING (stays RUNNING across step calls)
 *   RUNNING → shutdown() → SHUTDOWN
 */
enum class ComponentState {
    CREATED,       ///< Component constructed but not initialized
    INITIALIZED,   ///< Resources allocated, connections not yet wired
    CONFIGURED,    ///< Ports connected, ready to execute
    RUNNING,       ///< Actively executing step()
    SHUTDOWN       ///< Cleaned up, no longer usable
};

/**
 * @brief Interface for all ADAS Software Components.
 *
 * Inspired by AUTOSAR SWC (Software Component) lifecycle model.
 * Subclass ComponentBase for default lifecycle management.
 */
class IComponent {
public:
    virtual ~IComponent() = default;

    /// @return Human-readable component name (e.g., "PlannerSWC")
    virtual std::string name() const = 0;

    /**
     * @brief Initialize the component — allocate resources, create internal state.
     * @return true on success, false if initialization failed.
     *
     * Called once after construction. Must transition state to INITIALIZED.
     */
    virtual bool init() = 0;

    /**
     * @brief Configure the component — connect ports, finalize setup.
     * @return true on success, false if configuration failed.
     *
     * Called after all components are initialized. Must transition to CONFIGURED.
     */
    virtual bool configure() = 0;

    /**
     * @brief Execute one cycle of the component's runnable entity.
     * @param dt Delta time in seconds since last step.
     *
     * Called every frame/cycle. Component transitions to RUNNING on first call.
     */
    virtual void step(float dt) = 0;

    /**
     * @brief Shutdown the component — release resources, save state.
     *
     * Called once during application teardown. Must transition to SHUTDOWN.
     */
    virtual void shutdown() = 0;

    /// @return Current lifecycle state.
    virtual ComponentState state() const = 0;
};

} // namespace adas

#endif // ADAS_CORE_ICOMPONENT_HPP
