#ifndef ADAS_SAFETY_E2E_PROTECTION_HPP
#define ADAS_SAFETY_E2E_PROTECTION_HPP

/**
 * @file E2EProtection.hpp
 * @brief E2E Profile compliant with ASIL-B data integrity requirements.
 *
 * Implements CRC-8-SAE J1850 and 4-bit Sequence Counter check.
 * Used by safety-relevant SWCs (e.g., HudSWC) to validate incoming data.
 */

#include "core/TypeDefs.hpp"

namespace adas {

class E2EChecker {
public:
    E2EChecker() : expected_counter_(0), first_message_(true) {}

    /**
     * @brief Computes CRC-8 (SAE J1850).
     * Polynomial: 0x1D, Initial: 0xFF, Final XOR: 0xFF
     */
    static uint8 compute_crc8(const uint8* data, size_t length) {
        uint8 crc = 0xFF;
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
     * @brief Validates the incoming message with CRC + sequence counter.
     * @param received_counter 4-bit sequence counter from sender.
     * @param data Pointer to the payload data.
     * @param len Length of the payload data.
     * @param received_crc CRC sent by the sender.
     * @return true if E2E check passes.
     */
    bool check(uint8 received_counter, const uint8* data, size_t len, uint8 received_crc) {
        // 1. CRC check
        if (compute_crc8(data, len) != received_crc) {
            return false;
        }

        // 2. Sequence counter check (4-bit wrap)
        received_counter &= 0x0F;

        if (first_message_) {
            expected_counter_ = (received_counter + 1) & 0x0F;
            first_message_ = false;
            return true;
        }

        if (received_counter != expected_counter_) {
            // Counter jump — resync but flag this packet as invalid
            expected_counter_ = (received_counter + 1) & 0x0F;
            return false;
        }

        expected_counter_ = (expected_counter_ + 1) & 0x0F;
        return true;
    }

    void reset() {
        first_message_ = true;
        expected_counter_ = 0;
    }

private:
    uint8   expected_counter_;
    boolean first_message_;
};

} // namespace adas

#endif // ADAS_SAFETY_E2E_PROTECTION_HPP
