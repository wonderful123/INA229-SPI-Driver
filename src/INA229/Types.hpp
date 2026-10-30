#pragma once

#include <cstdint>
#include <string_view>

namespace INA229 {

/**
 * @brief Driver status codes
 */
enum class Status : uint8_t {
  OK,
  ERR_COMM,
  ERR_ID_MISMATCH,
  ERR_DEV_NOT_READY,
  ERR_DEV_RESET,
  ERR_INVALID_CONFIG
};

/**
 * @brief MODE (Bits 15-12 in ADC_CONFIG)
 */
enum class AdcMode : uint16_t {
  SHUTDOWN_0 = 0x0,
  TRIGGERED_BUS = 0x1,
  TRIGGERED_SHUNT = 0x2,
  TRIGGERED_BUS_SHUNT = 0x3,
  TRIGGERED_TEMP = 0x4,
  TRIGGERED_TEMP_BUS = 0x5,
  TRIGGERED_TEMP_SHUNT = 0x6,
  TRIGGERED_ALL = 0x7,
  SHUTDOWN_8 = 0x8,
  CONTINUOUS_BUS = 0x9,
  CONTINUOUS_SHUNT = 0xA,
  CONTINUOUS_BUS_SHUNT = 0xB,
  CONTINUOUS_TEMP = 0xC,
  CONTINUOUS_BUS_TEMP = 0xD,
  CONTINUOUS_TEMP_SHUNT = 0xE,
  CONTINUOUS_ALL = 0xF // Default
};

/**
 * @brief Conversion Time settings (VBUSCT, VSHCT, VTCT in ADC_CONFIG)
 */
enum class ConversionTime : uint16_t {
  TIME_50us = 0x0,
  TIME_84us = 0x1,
  TIME_150us = 0x2,
  TIME_280us = 0x3,
  TIME_540us = 0x4,
  TIME_1052us = 0x5, // Default
  TIME_2074us = 0x6,
  TIME_4120us = 0x7
};

/**
 * @brief AVG (Bits 2-0 in ADC_CONFIG): ADC Sample Averaging Count
 */
enum class AvgCount : uint16_t {
  AVG_1 = 0x0, // Default
  AVG_4 = 0x1,
  AVG_16 = 0x2,
  AVG_64 = 0x3,
  AVG_128 = 0x4,
  AVG_256 = 0x5,
  AVG_512 = 0x6,
  AVG_1024 = 0x7
};

/**
 * @brief Selectable Shunt Full Scale Range (ADCRANGE in CONFIG register bit 4)
 */
enum class ShuntRange : uint16_t {
  RANGE_164mV = 0, // ±163.84 mV (ADCRANGE = 0, Default)
  RANGE_41mV = 1   // ±40.96 mV  (ADCRANGE = 1)
};

/**
 * @brief Bitmask flags for the DIAG_ALRT register (0x0B).
 */
enum class AlertStatus : uint16_t {
  NONE = 0,
  ALATCH = 1U << 15,     // Latch alert status until read
  CNVR = 1U << 14,       // Alert pin asserts on conversion ready
  SLOWALERT = 1U << 13,  // Alert comparison delay until averaging complete
  APOL = 1U << 12,       // Alert polarity (1 = Active high)
  ENERGY_OF = 1U << 11,  // Energy accumulator overflow
  CHARGE_OF = 1U << 10,  // Charge accumulator overflow
  MATH_OF = 1U << 9,     // Internal math overflow
  TMP_OVER = 1U << 7,    // Die temperature over-limit
  SHUNT_OVER = 1U << 6,  // Shunt voltage over-limit
  SHUNT_UNDER = 1U << 5, // Shunt voltage under-limit
  BUS_OVER = 1U << 4,    // Bus voltage over-limit
  BUS_UNDER = 1U << 3,   // Bus voltage under-limit
  POWER_OVER = 1U << 2,  // Power over-limit
  CONV_READY = 1U << 1,  // Conversion ready flag
  MEMSTAT = 1U << 0      // Trim memory status (0 = Checksum error)
};

// --- AlertStatus Bitwise Operators ---

[[nodiscard]] constexpr AlertStatus operator|(AlertStatus lhs,
                                              AlertStatus rhs) noexcept {
  return static_cast<AlertStatus>(static_cast<uint16_t>(lhs) |
                                  static_cast<uint16_t>(rhs));
}

[[nodiscard]] constexpr AlertStatus operator&(AlertStatus lhs,
                                              AlertStatus rhs) noexcept {
  return static_cast<AlertStatus>(static_cast<uint16_t>(lhs) &
                                  static_cast<uint16_t>(rhs));
}

[[nodiscard]] constexpr AlertStatus operator~(AlertStatus flag) noexcept {
  return static_cast<AlertStatus>(~static_cast<uint16_t>(flag));
}

constexpr AlertStatus &operator|=(AlertStatus &lhs, AlertStatus rhs) noexcept {
  lhs = lhs | rhs;
  return lhs;
}

constexpr AlertStatus &operator&=(AlertStatus &lhs, AlertStatus rhs) noexcept {
  lhs = lhs & rhs;
  return lhs;
}

[[nodiscard]] constexpr bool hasFlag(AlertStatus value,
                                     AlertStatus flag) noexcept {
  return (static_cast<uint16_t>(value) & static_cast<uint16_t>(flag)) != 0;
}

/**
 * @brief Convert a Status code to a human-readable string.
 */
[[nodiscard]] constexpr std::string_view
statusToString(Status status) noexcept {
  switch (status) {
  case Status::OK:
    return "OK";
  case Status::ERR_COMM:
    return "Communication Error";
  case Status::ERR_ID_MISMATCH:
    return "Device ID Mismatch";
  case Status::ERR_DEV_NOT_READY:
    return "Device Not Ready";
  case Status::ERR_INVALID_CONFIG:
    return "Invalid Configuration";
  case Status::ERR_DEV_RESET:
    return "Device Reset Detected";
  }
  return "Unknown Status";
}

} // namespace INA229
