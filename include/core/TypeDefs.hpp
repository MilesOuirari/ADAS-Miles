#ifndef ADAS_CORE_TYPEDEFS_HPP
#define ADAS_CORE_TYPEDEFS_HPP

/**
 * @file TypeDefs.hpp
 * @brief AUTOSAR-compliant type aliases for platform independence.
 *
 * Following AUTOSAR Platform Types (SWS_Platform_00001 - SWS_Platform_00013).
 * These ensure consistent type widths across all target platforms.
 */

#include <cstdint>
#include <cstddef>

namespace adas {

// ─── Unsigned Integer Types ────────────────────────────────────────────────
using uint8   = std::uint8_t;    ///< 8-bit unsigned
using uint16  = std::uint16_t;   ///< 16-bit unsigned
using uint32  = std::uint32_t;   ///< 32-bit unsigned
using uint64  = std::uint64_t;   ///< 64-bit unsigned

// ─── Signed Integer Types ──────────────────────────────────────────────────
using sint8   = std::int8_t;     ///< 8-bit signed
using sint16  = std::int16_t;    ///< 16-bit signed
using sint32  = std::int32_t;    ///< 32-bit signed
using sint64  = std::int64_t;    ///< 64-bit signed

// ─── Floating Point Types ──────────────────────────────────────────────────
using float32 = float;           ///< 32-bit IEEE 754 float
using float64 = double;          ///< 64-bit IEEE 754 double

// ─── Boolean ───────────────────────────────────────────────────────────────
using boolean = bool;            ///< Boolean type

// ─── Standard Return Type ──────────────────────────────────────────────────
enum class StdReturnType : uint8 {
    E_OK       = 0x00,   ///< Operation succeeded
    E_NOT_OK   = 0x01,   ///< Operation failed
    E_PENDING  = 0x02    ///< Operation still in progress
};

} // namespace adas

#endif // ADAS_CORE_TYPEDEFS_HPP
