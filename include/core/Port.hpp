#ifndef ADAS_CORE_PORT_HPP
#define ADAS_CORE_PORT_HPP

/**
 * @file Port.hpp
 * @brief Typed Sender-Receiver port system inspired by AUTOSAR Rte_Read/Rte_Write.
 *
 * SenderPort<T>   — writes data to the signal bus.
 * ReceiverPort<T> — reads data from the signal bus (last-is-best semantics).
 *
 * Ports are connected via the SignalBus; components never reference each other directly.
 */

#include <memory>
#include <atomic>

namespace adas {

/**
 * @brief Shared signal storage between a sender and its receivers.
 *
 * Contains the latest value written by the sender. Receivers observe
 * the same underlying storage via shared_ptr.
 */
template <typename T>
class SignalSlot {
public:
    SignalSlot() : updated_(false) {}

    void write(const T& value) {
        data_ = value;
        updated_.store(true, std::memory_order_release);
    }

    const T& read() const { return data_; }

    bool isUpdated() const { return updated_.load(std::memory_order_acquire); }

    void clearUpdated() { updated_.store(false, std::memory_order_release); }

private:
    T data_{};
    std::atomic<bool> updated_;
};

// ─── Sender Port ───────────────────────────────────────────────────────────

/**
 * @brief Output port — writes a typed signal to a shared slot.
 *
 * A component's runnable entity uses this to publish data.
 * Equivalent to AUTOSAR Rte_Write_<port>_<element>().
 *
 * @tparam T The data type transported by this port.
 */
template <typename T>
class SenderPort {
public:
    SenderPort() : slot_(std::make_shared<SignalSlot<T>>()) {}

    /// Write a new value to this port's signal slot
    void write(const T& value) { slot_->write(value); }

    /// @return The underlying shared slot (used by SignalBus to connect receivers)
    std::shared_ptr<SignalSlot<T>> slot() const { return slot_; }

private:
    std::shared_ptr<SignalSlot<T>> slot_;
};

// ─── Receiver Port ─────────────────────────────────────────────────────────

/**
 * @brief Input port — reads a typed signal from a shared slot.
 *
 * A component's runnable entity uses this to consume data.
 * Equivalent to AUTOSAR Rte_Read_<port>_<element>().
 *
 * @tparam T The data type transported by this port.
 */
template <typename T>
class ReceiverPort {
public:
    ReceiverPort() = default;

    /// Connect this receiver to a sender's slot
    void connect(std::shared_ptr<SignalSlot<T>> slot) { slot_ = std::move(slot); }

    /// @return true if a slot is connected
    bool isConnected() const { return slot_ != nullptr; }

    /// @return The latest value from the connected sender
    const T& read() const {
        static const T empty{};
        return slot_ ? slot_->read() : empty;
    }

    /// @return true if the sender has written since last clearUpdated()
    bool isUpdated() const { return slot_ && slot_->isUpdated(); }

    /// Reset the updated flag (consume the notification)
    void clearUpdated() { if (slot_) slot_->clearUpdated(); }

private:
    std::shared_ptr<SignalSlot<T>> slot_;
};

} // namespace adas

#endif // ADAS_CORE_PORT_HPP
