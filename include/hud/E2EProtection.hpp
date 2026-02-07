#ifndef HUD_E2E_PROTECTION_HPP
#define HUD_E2E_PROTECTION_HPP

#include <cstdint>
#include <cstddef>

namespace adas::hud {

/**
 * @brief E2E Profile compliant with simple ASIL-B data integrity requirements.
 * Implements CRC-8-SAE J1850 and 4-bit Sequence Counter check.
 */
class E2EChecker {
public:
    E2EChecker() : expected_counter_(0), first_message_(true) {}

    /**
     * @brief Computes CRC-8 (SAE J1850)
     * Polynomial: 0x1D, Initial: 0xFF, Final XOR: 0xFF
     */
    static uint8_t compute_crc8(const uint8_t* data, size_t length) {
        uint8_t crc = 0xFF;
        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x80) {
                    crc = (crc << 1) ^ 0x1D;
                } else {
                    crc <<= 1;
                }
            }
        }
        return crc ^ 0xFF;
    }

    /**
     * @brief Validates the incoming message.
     * @param received_counter 4-bit sequence counter from sender.
     * @param data Pointer to the payload data.
     * @param len Length of the payload data.
     * @param received_crc CRC sent by the sender.
     * @return true if E2E check passes, false otherwise.
     */
    bool check(uint8_t received_counter, const uint8_t* data, size_t len, uint8_t received_crc) {
        // 1. Check CRC
        if (compute_crc8(data, len) != received_crc) {
            return false;
        }

        // 2. Check Sequence Counter (simple increment)
        // Mask to 4 bits (0-15) just in case, though usually handled by type
        received_counter &= 0x0F;

        if (first_message_) {
            expected_counter_ = (received_counter + 1) & 0x0F;
            first_message_ = false;
            return true;
        }

        if (received_counter != expected_counter_) {
            // Counter jump detected - could be packet loss or ordering issue.
            // For this simplified module, we resync to the new counter + 1
            // but return false for THIS packet to indicate 'invalid sequence'.
            // In strict ASIL systems, we might have a tolerance window (Project specific).
            expected_counter_ = (received_counter + 1) & 0x0F;
            return false;
        }

        // Prepare for next
        expected_counter_ = (expected_counter_ + 1) & 0x0F;
        return true;
    }

    void reset() {
        first_message_ = true;
        expected_counter_ = 0;
    }

private:
    uint8_t expected_counter_;
    bool first_message_;
};

} // namespace adas::hud

#endif // HUD_E2E_PROTECTION_HPP
