#include "INA229/Config.hpp"
#include <algorithm>
#include <cmath>

namespace INA229 {
namespace {

double calculateRawShuntCalibration(const DriverConfig &config) noexcept {
  const double multiplier =
      (config.range == ShuntRange::RANGE_164mV)
          ? Mask::ShuntCal::Calculation::MULTIPLIER_RANGE0
          : Mask::ShuntCal::Calculation::MULTIPLIER_RANGE1;

  return multiplier * config.shuntResistor * config.calculateLsb();
}

} // namespace

uint16_t DriverConfig::getTempCoValue() const noexcept {
  if (!tempCompensation || !shuntTempCoeff.has_value() ||
      shuntTempCoeff.value() < 0.0) {
    return 0;
  }

  // Datasheet section 7.6.1.4: one register LSB is 1 ppm/°C.
  if (!std::isfinite(*shuntTempCoeff)) {
    return 0;
  }
  const double clamped = std::clamp(
      *shuntTempCoeff, 0.0, static_cast<double>(Mask::ShuntTempco::DATA_MASK));

  return static_cast<uint16_t>(std::round(clamped));
}

uint16_t DriverConfig::getShuntCalValue() const noexcept {
  const double cal = calculateRawShuntCalibration(*this);
  if (!std::isfinite(cal)) {
    return 0;
  }
  const double clamped =
      std::clamp(cal, 0.0, static_cast<double>(Mask::ShuntCal::SHUNT_CAL_MASK));

  return static_cast<uint16_t>(std::round(clamped));
}

Status DriverConfig::validate(const char **errorMsg) const noexcept {
  auto setError = [errorMsg](const char *msg) {
    if (errorMsg) {
      *errorMsg = msg;
    }
  };

  if (errorMsg) {
    *errorMsg = nullptr;
  }

  if (!std::isfinite(shuntResistor) || shuntResistor <= 0.0) {
    setError("Shunt resistor must be > 0");
    return Status::ERR_INVALID_CONFIG;
  }

  if (!std::isfinite(maxExpectedCurrent) || maxExpectedCurrent <= 0.0) {
    setError("Maximum expected current must be > 0");
    return Status::ERR_INVALID_CONFIG;
  }

  if ((range != ShuntRange::RANGE_164mV && range != ShuntRange::RANGE_41mV) ||
      static_cast<uint16_t>(mode) > 15 || static_cast<uint16_t>(avg) > 7 ||
      static_cast<uint16_t>(vBusTime) > 7 ||
      static_cast<uint16_t>(vShuntTime) > 7 ||
      static_cast<uint16_t>(tempTime) > 7) {
    setError("Invalid ADC configuration enum value");
    return Status::ERR_INVALID_CONFIG;
  }

  // Validate maximum differential shunt voltage against chosen ShuntRange limit
  const double maxShuntVoltage = maxExpectedCurrent * shuntResistor;
  const double maxAllowedVoltage =
      (range == ShuntRange::RANGE_164mV) ? 0.16384 : 0.04096;

  if (maxShuntVoltage > maxAllowedVoltage) {
    setError("Max expected voltage (I_max * R_shunt) exceeds selected "
             "ShuntRange limit");
    return Status::ERR_INVALID_CONFIG;
  }

  // Validate SHUNT_CAL register capacity (15-bit field: max 32767)
  const double rawCal = calculateRawShuntCalibration(*this);
  if (!std::isfinite(rawCal) ||
      rawCal > static_cast<double>(Mask::ShuntCal::SHUNT_CAL_MASK)) {
    setError("SHUNT_CAL overflows 15-bit register (32767). Decrease "
             "maxExpectedCurrent");
    return Status::ERR_INVALID_CONFIG;
  }
  if (rawCal < 1.0) {
    setError("SHUNT_CAL underflows (< 1). Increase maxExpectedCurrent or "
             "increase R_shunt");
    return Status::ERR_INVALID_CONFIG;
  }

  // Validate temperature compensation settings
  if (tempCompensation) {
    if (!shuntTempCoeff.has_value()) {
      setError("Temperature compensation enabled but shuntTempCoeff not set");
      return Status::ERR_INVALID_CONFIG;
    }

    const double coeff = shuntTempCoeff.value();
    if (!std::isfinite(coeff) || coeff < 0.0 || coeff > 16383.0) {
      setError(
          "Shunt temperature coefficient out of valid range [0, 16383] ppm/°C");
      return Status::ERR_INVALID_CONFIG;
    }
  }

  return Status::OK;
}

} // namespace INA229
