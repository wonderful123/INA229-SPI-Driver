// lib/INA229/src/INA229.cpp
#include <INA229/INA229.hpp>
#include <INA229/Utility.hpp>
#include <array>
#include <optional>

namespace INA229 {

Status INA229_Driver::begin(const DriverConfig &config) {
  if (auto status = config.validate(); status != Status::OK) {
    _lastStatus = status;
    return status;
  }

  if (auto status = resetAndVerify(); status != Status::OK) {
    _lastStatus = status;
    return status;
  }

  write16(Register::ADC_CONFIG, config.getAdcConfigValue());
  write16(Register::CONFIG, config.getConfigValue());
  write16(Register::SHUNT_CAL, config.getShuntCalValue());

  if (config.tempCompensation) {
    write16(Register::SHUNT_TEMPCO, config.getTempCoValue());
  }

  _storedConfig = config;
  _currentLsb = config.calculateLsb();
  _expectedShuntCal = config.getShuntCalValue();
  _isInitialized = true;
  _lastStatus = Status::OK;

  _isHealthyCached = true;
  _healthCheckCounter = 0;

  return Status::OK;
}

bool INA229_Driver::isHealthy() noexcept {
  invalidateHealthCache();
  return ensureHealthy();
}

bool INA229_Driver::ensureHealthy() const noexcept {
  if (_isHealthyCached && (++_healthCheckCounter < HEALTH_CHECK_INTERVAL)) {
    return true;
  }

  _healthCheckCounter = 0;

  if (!_isInitialized) {
    _lastStatus = Status::ERR_DEV_NOT_READY;
    _isHealthyCached = false;
    return false;
  }

  const auto currentCal =
      static_cast<uint16_t>(readRegister(Register::SHUNT_CAL));

  if (currentCal == _expectedShuntCal) {
    _lastStatus = Status::OK;
    _isHealthyCached = true;
    return true;
  }

  _lastStatus = recoverFromReset();
  _isHealthyCached = (_lastStatus == Status::OK);

  return _isHealthyCached;
}

void INA229_Driver::invalidateHealthCache() noexcept {
  _isHealthyCached = false;
  _healthCheckCounter = 0;
}

Status INA229_Driver::recoverFromReset() const noexcept {
  write16(Register::ADC_CONFIG, _storedConfig.getAdcConfigValue());
  write16(Register::CONFIG, _storedConfig.getConfigValue());
  write16(Register::SHUNT_CAL, _storedConfig.getShuntCalValue());

  if (_storedConfig.tempCompensation) {
    write16(Register::SHUNT_TEMPCO, _storedConfig.getTempCoValue());
  }

  const auto verifyRead =
      static_cast<uint16_t>(readRegister(Register::SHUNT_CAL));

  return (verifyRead == _expectedShuntCal) ? Status::OK : Status::ERR_COMM;
}

// --- Data Acquisition ---

std::optional<double> INA229_Driver::getCurrentAmps() const {
  return safeMeasure([this]() {
    const auto raw = static_cast<uint32_t>(readRegister(Register::CURRENT));
    const int32_t signedValue = DataUtility::signExtend20Bit(raw);
    return static_cast<double>(signedValue) * _currentLsb;
  });
}

std::optional<double> INA229_Driver::getShuntVoltageVolts() const {
  return safeMeasure([this]() {
    const auto raw = static_cast<uint32_t>(readRegister(Register::VSHUNT));
    const int32_t signedValue = DataUtility::signExtend20Bit(raw);
    const double lsb = (_storedConfig.range == ShuntRange::RANGE_164mV)
                           ? Mask::VShunt::LSB_R0
                           : Mask::VShunt::LSB_R1;
    return static_cast<double>(signedValue) * lsb;
  });
}

std::optional<double> INA229_Driver::getBusVoltage() const {
  return safeMeasure([this]() {
    const uint32_t raw = static_cast<uint32_t>(readRegister(Register::VBUS)) >>
                         Mask::VBus::SHIFT;
    return static_cast<double>(raw) * Mask::VBus::LSB;
  });
}

std::optional<double> INA229_Driver::getDieTemperature() const {
  return safeMeasure([this]() {
    const auto raw = static_cast<int16_t>(readRegister(Register::DIETEMP));
    return static_cast<double>(raw) * Mask::DieTemp::LSB_CELSIUS;
  });
}

std::optional<double> INA229_Driver::getPowerWatts() const {
  return safeMeasure([this]() {
    const uint32_t raw = static_cast<uint32_t>(readRegister(Register::POWER));
    return static_cast<double>(raw) * Mask::Power::LSB_FACTOR * _currentLsb;
  });
}

std::optional<double> INA229_Driver::getEnergyJoules() const {
  return safeMeasure([this]() {
    const uint64_t raw = readRegister(Register::ENERGY);
    constexpr double ENERGY_SCALING = 16.0;
    return static_cast<double>(raw) * ENERGY_SCALING *
           (Mask::Power::LSB_FACTOR * _currentLsb);
  });
}

std::optional<double> INA229_Driver::getChargeCoulombs() const {
  return safeMeasure([this]() {
    const uint64_t raw = readRegister(Register::CHARGE);
    const int64_t signedValue = DataUtility::signExtend40Bit(raw);
    return static_cast<double>(signedValue) * _currentLsb;
  });
}

// --- Diagnostics & Registers ---

std::optional<uint16_t> INA229_Driver::getDiagnosticFlags() const {
  return safeMeasure([this]() {
    return static_cast<uint16_t>(readRegister(Register::DIAG_ALRT));
  });
}

std::optional<uint16_t> INA229_Driver::getShuntTempCoRegister() const {
  return safeMeasure([this]() {
    const auto raw =
        static_cast<uint16_t>(readRegister(Register::SHUNT_TEMPCO));
    return static_cast<uint16_t>(raw & Mask::ShuntTempco::DATA_MASK);
  });
}

std::optional<uint16_t> INA229_Driver::getShuntCalRegister() const {
  return safeMeasure([this]() {
    const auto raw = static_cast<uint16_t>(readRegister(Register::SHUNT_CAL));
    return static_cast<uint16_t>(raw & Mask::ShuntCal::SHUNT_CAL_MASK);
  });
}

std::optional<uint16_t> INA229_Driver::getManufacturerId() const {
  return safeMeasure([this]() {
    return static_cast<uint16_t>(readRegister(Register::MANUFACTURER_ID));
  });
}

std::optional<uint16_t> INA229_Driver::getDeviceId() const {
  return safeMeasure([this]() {
    return static_cast<uint16_t>(readRegister(Register::DEVICE_ID));
  });
}

// --- Control & Status ---

Status INA229_Driver::resetAccumulators() {
  if (!ensureHealthy()) {
    return _lastStatus;
  }
  const uint16_t currentConfig = _storedConfig.getConfigValue();
  write16(Register::CONFIG, currentConfig | Mask::Config::RSTACC);
  return Status::OK;
}

// --- Hardware Communication ---

Status INA229_Driver::resetAndVerify() {
  write16(Register::CONFIG, Mask::Config::RST);

  bool resetComplete = false;
  for (int i = 0; i < RESET_POLL_ATTEMPTS; ++i) {
    const auto config = static_cast<uint16_t>(readRegister(Register::CONFIG));
    if (!(config & Mask::Config::RST)) {
      resetComplete = true;
      break;
    }
  }

  if (!resetComplete)
    return Status::ERR_COMM;

  const auto id =
      static_cast<uint16_t>(readRegister(Register::MANUFACTURER_ID));
  return (id == Mask::ManufacturerID::EXPECTED) ? Status::OK : Status::ERR_COMM;
}

void INA229_Driver::write16(Register reg, uint16_t value) const {
  const uint8_t cmd = DataUtility::makeCommandByte(reg, false);

  std::array<uint8_t, 2> buffer{static_cast<uint8_t>(value >> 8),
                                static_cast<uint8_t>(value & 0xFF)};

  _spi.write(cmd, buffer.data(), buffer.size());
}

uint64_t INA229_Driver::readRegister(Register reg) const {
  const uint8_t width = getRegisterWidth(reg);
  const uint8_t cmd = DataUtility::makeCommandByte(reg, true);

  std::array<uint8_t, 5> buffer{};
  _spi.read(cmd, buffer.data(), width);

  return DataUtility::packBytes(std::span{buffer}.first(width));
}

} // namespace INA229