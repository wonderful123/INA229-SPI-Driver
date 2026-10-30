#include "INA229/Config.hpp"
#include <algorithm>
#include <cmath>

namespace INA229 {

uint16_t DriverConfig::getTempCoValue() const noexcept {
  if (!tempCompensation || !shuntTempCoeff.has_value() ||
      shuntTempCoeff.value() < 0.0) {
    return 0;
  }

  // Formula: SHUNT_TEMPCO = (ppm/°C × 2^14) / 10^6
  static constexpr double SCALE = 16'384.0; // 2^14
  static constexpr double PPM_DIVISOR = 1'000'000.0;

  const double scaled = (shuntTempCoeff.value() * SCALE) / PPM_DIVISOR;
  const double clamped =
      std::clamp(scaled, 0.0, static_cast<double>(Mask::ShuntTempco::DATA_MASK));

  return static_cast<uint16_t>(round(clamped));
}

uint16_t DriverConfig::getShuntCalValue() const noexcept {
  const double multiplier =
      (range == ShuntRange::RANGE_164mV)
          ? Mask::ShuntCal::Calculation::MULTIPLIER_RANGE0
          : Mask::ShuntCal::Calculation::MULTIPLIER_RANGE1;

  const double cal = multiplier * shuntResistor * calculateLsb();
  const double clamped =
      std::clamp(cal, 0.0, static_cast<double>(Mask::ShuntCal::SHUNT_CAL_MASK));

  return static_cast<uint16_t>(round(clamped));
}

Status DriverConfig::validate(const char **errorMsg) const noexcept {
  auto setError = [errorMsg](const char *msg) {
    if (errorMsg) {
      *errorMsg = msg;
    }
  };

  if (shuntResistor <= 0.0) {
    setError("Shunt resistor must be > 0");
    return Status::ERR_INVALID_CONFIG;
  }

  if (maxExpectedCurrent <= 0.0) {
    setError("Maximum expected current must be > 0");
    return Status::ERR_INVALID_CONFIG;
  }

  // Validate maximum differential shunt voltage against chosen ShuntRange limit
  const double maxShuntVoltage = maxExpectedCurrent * shuntResistor;
  const double maxAllowedVoltage =
      (range == ShuntRange::RANGE_164mV) ? 0.16384 : 0.04096;

  if (maxShuntVoltage > maxAllowedVoltage) {
    setError("Max expected voltage (I_max * R_shunt) exceeds selected ShuntRange limit");
    return Status::ERR_INVALID_CONFIG;
  }

  // Validate SHUNT_CAL register capacity (15-bit field: max 32767)
  const double multiplier =
      (range == ShuntRange::RANGE_164mV)
          ? Mask::ShuntCal::Calculation::MULTIPLIER_RANGE0
          : Mask::ShuntCal::Calculation::MULTIPLIER_RANGE1;

  const double rawCal = multiplier * shuntResistor * calculateLsb();
  if (rawCal > static_cast<double>(Mask::ShuntCal::SHUNT_CAL_MASK)) {
    setError("SHUNT_CAL overflows 15-bit register (32767). Increase maxExpectedCurrent");
    return Status::ERR_INVALID_CONFIG;
  }
  if (rawCal < 1.0) {
    setError("SHUNT_CAL underflows (< 1). Decrease maxExpectedCurrent or increase R_shunt");
    return Status::ERR_INVALID_CONFIG;
  }

  // Validate temperature compensation settings
  if (tempCompensation) {
    if (!shuntTempCoeff.has_value()) {
      setError("Temperature compensation enabled but shuntTempCoeff not set");
      return Status::ERR_INVALID_CONFIG;
    }

    const double coeff = shuntTempCoeff.value();
    if (coeff < 0.0 || coeff > 16383.0) {
      setError("Shunt temperature coefficient out of valid range [0, 16383] ppm/°C");
      return Status::ERR_INVALID_CONFIG;
    }
  }

  return Status::OK;
}

} // namespace INA229