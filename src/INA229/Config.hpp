#pragma once

#include "Registers.hpp"
#include "Types.hpp"
#include <optional>

namespace INA229 {

namespace Detail {

/**
 * @brief Helper to construct a valid ADC_CONFIG register value.
 */
[[nodiscard]] constexpr uint16_t
makeAdcConfig(AdcMode mode, ConversionTime busTime, ConversionTime shuntTime,
              ConversionTime tempTime, AvgCount avg) noexcept {
  using namespace Mask::AdcConfig;
  return (static_cast<uint16_t>(mode) << MODE_SHIFT) |
         (static_cast<uint16_t>(busTime) << VBUSCT_SHIFT) |
         (static_cast<uint16_t>(shuntTime) << VSHCT_SHIFT) |
         (static_cast<uint16_t>(tempTime) << VTCT_SHIFT) |
         (static_cast<uint16_t>(avg) << AVG_SHIFT);
}

} // namespace Detail

/**
 * @brief Configuration parameters for the INA229 driver.
 */
struct DriverConfig {
  double shuntResistor = 0.0;      ///< Shunt resistor value in Ohms
  double maxExpectedCurrent = 0.0; ///< Maximum expected current in Amperes

  AdcMode mode = AdcMode::CONTINUOUS_ALL;
  AvgCount avg = AvgCount::AVG_1;
  ConversionTime vBusTime = ConversionTime::TIME_1052us;
  ConversionTime vShuntTime = ConversionTime::TIME_1052us;
  ConversionTime tempTime = ConversionTime::TIME_1052us;

  ShuntRange range = ShuntRange::RANGE_164mV;
  uint8_t conversionDelay = 0; ///< Delay in 2ms steps (0-255)
  bool tempCompensation = false;
  std::optional<double> shuntTempCoeff = std::nullopt; ///< ppm/°C (0 to 16383)

  static constexpr double CURRENT_LSB_DIVISOR = 524288.0; // 2^19

  /**
   * @brief Calculate Current LSB in Amperes per bit.
   */
  [[nodiscard]] constexpr double calculateLsb() const noexcept {
    return maxExpectedCurrent / CURRENT_LSB_DIVISOR;
  }

  /**
   * @brief Generate ADC_CONFIG register value.
   */
  [[nodiscard]] constexpr uint16_t getAdcConfigValue() const noexcept {
    return Detail::makeAdcConfig(mode, vBusTime, vShuntTime, tempTime, avg);
  }

  /**
   * @brief Generate CONFIG register value.
   */
  [[nodiscard]] constexpr uint16_t getConfigValue() const noexcept {
    using namespace Mask::Config;

    uint16_t val = (static_cast<uint16_t>(conversionDelay) << CONVDLY_SHIFT) &
                   CONVDLY_MASK;

    if (range == ShuntRange::RANGE_41mV) {
      val |= ADCRANGE;
    }
    if (tempCompensation) {
      val |= TEMPCOMP;
    }

    return val;
  }

  /**
   * @brief Generate SHUNT_CAL register value safely clamped to 15 bits.
   */
  [[nodiscard]] uint16_t getShuntCalValue() const noexcept;

  /**
   * @brief Generate SHUNT_TEMPCO register value safely clamped to 14 bits.
   */
  [[nodiscard]] uint16_t getTempCoValue() const noexcept;

  /**
   * @brief Validate configuration parameters without dynamic memory allocation.
   * @param errorMsg Optional pointer to static error string.
   * @return Status::OK if valid, or Status::ERR_INVALID_CONFIG.
   */
  [[nodiscard]] Status validate(const char **errorMsg = nullptr) const noexcept;
};

} // namespace INA229
