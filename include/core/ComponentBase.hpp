#ifndef ADAS_CORE_COMPONENT_BASE_HPP
#define ADAS_CORE_COMPONENT_BASE_HPP

/**
 * @file ComponentBase.hpp
 * @brief Template Method base class for Software Components.
 *
 * Manages lifecycle state transitions and logging. Concrete SWCs
 * override the onXxx() hooks instead of the raw IComponent methods.
 */

#include "IComponent.hpp"
#include "Logger.hpp"

namespace adas {

/**
 * @brief Base class providing lifecycle management for SWCs.
 *
 * Uses the Template Method pattern:
 *  - init()      → guards state, calls onInit(),      transitions state
 *  - configure() → guards state, calls onConfigure(), transitions state
 *  - step(dt)    → guards state, calls onStep(dt),    transitions state
 *  - shutdown()  → guards state, calls onShutdown(),  transitions state
 *
 * Subclasses implement:
 *  - onInit(), onConfigure(), onStep(dt), onShutdown()
 */
class ComponentBase : public IComponent {
public:
    explicit ComponentBase(const std::string& component_name)
        : log_(component_name)
        , component_name_(component_name)
        , state_(ComponentState::CREATED) {}

    ~ComponentBase() override = default;

    // ─── IComponent Interface ──────────────────────────────────────────
    std::string name() const override { return component_name_; }
    ComponentState state() const override { return state_; }

    bool init() override {
        if (state_ != ComponentState::CREATED) {
            log_.error("init() called in invalid state (expected CREATED)");
            return false;
        }
        log_.info("Initializing...");
        if (!onInit()) {
            log_.error("Initialization failed");
            return false;
        }
        state_ = ComponentState::INITIALIZED;
        log_.info("Initialized successfully");
        return true;
    }

    bool configure() override {
        if (state_ != ComponentState::INITIALIZED) {
            log_.error("configure() called in invalid state (expected INITIALIZED)");
            return false;
        }
        log_.info("Configuring...");
        if (!onConfigure()) {
            log_.error("Configuration failed");
            return false;
        }
        state_ = ComponentState::CONFIGURED;
        log_.info("Configured successfully");
        return true;
    }

    void step(float dt) override {
        if (state_ != ComponentState::CONFIGURED && state_ != ComponentState::RUNNING) {
            log_.warn("step() called in invalid state — skipping");
            return;
        }
        if (state_ == ComponentState::CONFIGURED) {
            state_ = ComponentState::RUNNING;
        }
        onStep(dt);
    }

    void shutdown() override {
        if (state_ == ComponentState::SHUTDOWN) {
            log_.warn("shutdown() called on already-shutdown component");
            return;
        }
        log_.info("Shutting down...");
        onShutdown();
        state_ = ComponentState::SHUTDOWN;
        log_.info("Shutdown complete");
    }

protected:
    // ─── Template Method Hooks — override these in subclasses ──────────
    virtual bool onInit()          = 0;    ///< Allocate resources
    virtual bool onConfigure()     = 0;    ///< Wire ports, finalize setup
    virtual void onStep(float dt)  = 0;    ///< Execute one cycle
    virtual void onShutdown()      = 0;    ///< Release resources

    Logger log_;   ///< Per-component logger instance

private:
    std::string component_name_;
    ComponentState state_;
};

} // namespace adas

#endif // ADAS_CORE_COMPONENT_BASE_HPP
