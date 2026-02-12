#ifndef ADAS_CORE_SIGNAL_BUS_HPP
#define ADAS_CORE_SIGNAL_BUS_HPP

/**
 * @file SignalBus.hpp
 * @brief Central signal routing — connects SenderPorts to ReceiverPorts.
 *
 * Acts as the AUTOSAR RTE (Runtime Environment) communication layer.
 * Components register their ports with string-based signal IDs;
 * the bus wires senders to receivers automatically.
 *
 * Signal IDs follow the convention: "<component>/<signal>"
 *   e.g., "EnvironmentModel/ObjectList", "Planner/BestTrajectory"
 */

#include "Port.hpp"
#include "Logger.hpp"
#include <unordered_map>
#include <any>
#include <string>
#include <typeindex>
#include <stdexcept>

namespace adas {

/**
 * @brief Central signal bus for inter-component communication (Mediator pattern).
 *
 * Type-safe at runtime via std::any + std::type_index verification.
 * A sender registers a signal slot under a string ID; receivers connect by ID.
 */
class SignalBus {
public:
    SignalBus() : log_("SignalBus") {}

    /**
     * @brief Register a sender port's slot under a named signal.
     *
     * @tparam T        Data type of the signal.
     * @param signal_id Unique string identifier for this signal.
     * @param sender    The SenderPort whose slot will be published.
     */
    template <typename T>
    void registerSender(const std::string& signal_id, SenderPort<T>& sender) {
        if (slots_.count(signal_id)) {
            log_.warn("Signal '" + signal_id + "' already registered — overwriting");
        }
        slots_[signal_id] = sender.slot();
        types_.insert_or_assign(signal_id, std::type_index(typeid(T)));
        log_.debug("Registered sender: " + signal_id);
    }

    /**
     * @brief Connect a receiver port to a named signal.
     *
     * @tparam T        Data type of the signal (must match sender's type).
     * @param signal_id The signal ID to connect to.
     * @param receiver  The ReceiverPort to wire up.
     * @return true if connection succeeded.
     */
    template <typename T>
    bool connectReceiver(const std::string& signal_id, ReceiverPort<T>& receiver) {
        auto it = slots_.find(signal_id);
        if (it == slots_.end()) {
            log_.error("Signal '" + signal_id + "' not found — receiver not connected");
            return false;
        }

        // Type safety check
        auto type_it = types_.find(signal_id);
        if (type_it->second != std::type_index(typeid(T))) {
            log_.error("Type mismatch for signal '" + signal_id + "'");
            return false;
        }

        auto slot = std::any_cast<std::shared_ptr<SignalSlot<T>>>(it->second);
        receiver.connect(slot);
        log_.debug("Connected receiver to: " + signal_id);
        return true;
    }

    /// @return true if a signal with the given ID exists
    bool hasSignal(const std::string& signal_id) const {
        return slots_.count(signal_id) > 0;
    }

    /// @return Number of registered signals
    size_t signalCount() const { return slots_.size(); }

private:
    std::unordered_map<std::string, std::any> slots_;           ///< signal_id → shared_ptr<SignalSlot<T>>
    std::unordered_map<std::string, std::type_index> types_;    ///< signal_id → type_index for safety
    Logger log_;
};

} // namespace adas

#endif // ADAS_CORE_SIGNAL_BUS_HPP
