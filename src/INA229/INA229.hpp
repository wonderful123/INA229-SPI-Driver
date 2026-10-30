#pragma once

#include "Config.hpp"
#include "ISPI.hpp"
#include "Registers.hpp"
#include "Types.hpp"
#include <optional>

namespace INA229 {

/**
 * @brief High-level driver for the TI INA229 85V, 20-Bit, Ultra-Precise Power
 * Monitor IC.
 *
 * Provides a C++17 interface for configuring, reading measurements, and
 * monitoring device health over SPI.
 *
 * @note This class is non-copyable and non-movable to guarantee unique hardware
 * access.
 */
class INA229_Driver {
public:
  /**
   * @brief Construct a new INA229 Driver instance.
   *
   * @param spi Reference to an abstract SPI bus instance implementing the ISPI
   * interface.
   */
  explicit INA229_Driver(ISPI &spi) noexcept : _spi(spi) {}

  // Non-copyable and non-movable (hardware access must remain unique)
  INA229_Driver(const INA229_Driver &) = delete;
  INA229_Driver &operator=(const INA229_Driver &) = delete;
  INA229_Driver(INA229_Driver &&) = delete;
  INA229_Driver &operator=(INA229_Driver &&) = delete;

  /**
   * @brief Initializes and configures the hardware.
   *
   * Performs a soft reset, verifies manufacturer ID via SPI, writes system
   * configurations, sets shunt calibration and temperature compensation, and
   * verifies readback. Any failure disables measurements until a subsequent
   * successful begin().
   *
   * @param config Driver settings containing shunt resistance, expected
   * current, and ADC configuration.
   * @return Status::OK on success, or an error code describing the failure
   * reason.
   */
  [[nodiscard]] Status begin(const DriverConfig &config);

  // --- Data Acquisition ---

  /**
   * @brief Read current flowing through the shunt resistor.
   *
   * Reads the 20-bit signed CURRENT register (0x07) and converts it to Amperes.
   *
   * @return Current in Amperes (A), or std::nullopt on health check failure.
   */
  [[nodiscard]] std::optional<double> getCurrentAmps() const;

  /**
   * @brief Read raw differential voltage across the shunt resistor inputs (IN+
   * - IN-).
   *
   * Reads the 20-bit signed VSHUNT register (0x04) and converts it to Volts
   * using the configured ADCRANGE scale factor.
   *
   * @return Differential shunt voltage in Volts (V), or std::nullopt on health
   * check failure.
   */
  [[nodiscard]] std::optional<double> getShuntVoltageVolts() const;

  /**
   * @brief Read bus voltage at the VIN+ pin relative to GND.
   *
   * Reads the 20-bit unsigned VBUS register (0x05) and converts it to Volts.
   *
   * @return Bus voltage in Volts (V), or std::nullopt on health check failure.
   */
  [[nodiscard]] std::optional<double> getBusVoltage() const;

  /**
   * @brief Read internal die temperature of the INA229 IC.
   *
   * Reads the 16-bit DIETEMP register (0x06) scaled at 7.8125 m°C/LSB.
   *
   * @return Temperature in degrees Celsius (°C), or std::nullopt on health
   * check failure.
   */
  [[nodiscard]] std::optional<double> getDieTemperature() const;

  /**
   * @brief Read calculated bus power.
   *
   * Reads the 24-bit POWER register (0x08) and scales it using current LSB
   * factor.
   *
   * @return Power in Watts (W), or std::nullopt on health check failure.
   */
  [[nodiscard]] std::optional<double> getPowerWatts() const;

  /**
   * @brief Read accumulated energy measured since last reset.
   *
   * Reads the 40-bit ENERGY register (0x09) scaled by current LSB and time
   * constants.
   *
   * @return Energy in Joules (J), or std::nullopt on health check failure.
   */
  [[nodiscard]] std::optional<double> getEnergyJoules() const;

  /**
   * @brief Read accumulated electrical charge measured since last reset.
   *
   * Reads the 40-bit signed CHARGE register (0x0A) and scales it to Coulombs.
   *
   * @return Charge in Coulombs (C), or std::nullopt on health check failure.
   */
  [[nodiscard]] std::optional<double> getChargeCoulombs() const;

  // --- Diagnostics & Registers ---

  /**
   * @brief Read diagnostic flags and alert status flags (DIAG_ALRT).
   *
   * @return 16-bit raw bitmask of register 0x0B containing alert status and
   * error flags, or std::nullopt on health check failure.
   */
  [[nodiscard]] std::optional<uint16_t> getDiagnosticFlags() const;

  /**
   * @brief Read active temperature compensation coefficient value.
   *
   * @return 14-bit SHUNT_TEMPCO register value (bits 13:0), or std::nullopt on
   * health check failure.
   */
  [[nodiscard]] std::optional<uint16_t> getShuntTempCoRegister() const;

  /**
   * @brief Read raw hardware SHUNT_CAL register directly from device over SPI.
   *
   * @return 15-bit calibration register value (bits 14:0), or std::nullopt on
   * health check failure.
   */
  [[nodiscard]] std::optional<uint16_t> getShuntCalRegister() const;

  /**
   * @brief Read manufacturer identification register (MANUFACTURER_ID).
   *
   * @return 16-bit ID value (expected 0x5449), or std::nullopt on health check
   * failure.
   */
  [[nodiscard]] std::optional<uint16_t> getManufacturerId() const;

  /**
   * @brief Read device hardware ID and silicon revision register (DEVICE_ID).
   *
   * @return 16-bit word containing DIEID (bits 15:4) and REV_ID (bits 3:0), or
   * std::nullopt on health check failure.
   */
  [[nodiscard]] std::optional<uint16_t> getDeviceId() const;

  // --- Control & Status ---

  /**
   * @brief Clear energy and charge accumulation registers (ENERGY and CHARGE).
   *
   * Asserts the RSTACC bit in the CONFIG register to reset internal
   * accumulators.
   *
   * @return Status::OK on successful register update, or error status code on
   * failure.
   */
  Status resetAccumulators();

  /**
   * @brief Get calculated current LSB used for measurement scaling.
   *
   * @return Current LSB in Amperes per bit.
   */
  [[nodiscard]] double getCurrentLsb() const noexcept { return _currentLsb; }

  /**
   * @brief Get status code of the last attempted SPI operation or health check.
   *
   * @return Last recorded Status enum value.
   */
  [[nodiscard]] Status getLastStatus() const noexcept { return _lastStatus; }

  /**
   * @brief Force an immediate hardware health check and attempt recovery if
   * needed.
   *
   * Invalidates cached health state and triggers verification of key registers
   * via SPI.
   *
   * @return true if hardware is healthy and communicating, false otherwise.
   */
  [[nodiscard]] bool isHealthy() noexcept;

private:
  ISPI &_spi;
  DriverConfig _storedConfig{};
  double _currentLsb = 0.0;
  bool _isInitialized = false;
  mutable Status _lastStatus = Status::ERR_DEV_NOT_READY;

  // Health check caching parameters
  mutable bool _isHealthyCached = false;
  mutable uint8_t _healthCheckCounter = 0;
  static constexpr uint8_t HEALTH_CHECK_INTERVAL = 8;
  static constexpr int RESET_POLL_ATTEMPTS = 32;

  /**
   * @brief Internal check to confirm device readiness and configuration
   * validity.
   *
   * Evaluates cached health status or checks register configuration over SPI.
   * If hardware power-on reset occurs, triggers recovery.
   *
   * @return true if hardware is functional, false otherwise.
   */
  [[nodiscard]] bool ensureHealthy() const noexcept;

  /**
   * @brief Restores configuration settings to hardware following an unexpected
   * device reset.
   *
   * Writes back and verifies all stored configuration registers.
   * Recovery does not wait for a fresh conversion or restore lost accumulators.
   *
   * @return Status::OK if recovery succeeded and was verified over SPI.
   */
  [[nodiscard]] Status recoverFromReset() const noexcept;

  /**
   * @brief Flushes the internal health status cache, forcing the next call to
   * re-verify over SPI.
   */
  void invalidateHealthCache() noexcept;

  void applyConfiguration(const DriverConfig &config) const;
  [[nodiscard]] bool configurationMatches(const DriverConfig &config) const;

  /**
   * @brief Issues soft reset to chip and polls CONFIG until bit clears and
   * manufacturer ID matches.
   *
   * @return Status::OK if reset completes and ID is valid.
   */
  [[nodiscard]] Status resetAndVerify();

  /**
   * @brief Write a 16-bit word to a specified device register.
   *
   * @param reg Target register enum address.
   * @param value 16-bit register payload.
   */
  void write16(Register reg, uint16_t value) const;

  /**
   * @brief Read register payload up to 40 bits and pack into a 64-bit integer.
   *
   * @param reg Target register enum address.
   * @return Packed unsigned 64-bit raw register value.
   */
  [[nodiscard]] uint64_t readRegister(Register reg) const;

  /**
   * @brief Templated safety wrapper for data acquisition and register reads.
   *
   * Evaluates `ensureHealthy()` prior to executing reading closure `readFn`.
   *
   * @tparam ReadFn Callable reading logic type.
   * @param readFn Lambda/function executing register read and conversion.
   * @return Result of `readFn()` wrapped in std::optional, or std::nullopt if
   * unhealthy.
   */
  template <typename ReadFn>
  [[nodiscard]] auto safeMeasure(ReadFn &&readFn) const {
    using ReturnType = decltype(readFn());
    return ensureHealthy() ? std::optional<ReturnType>{readFn()} : std::nullopt;
  }
};

} // namespace INA229
