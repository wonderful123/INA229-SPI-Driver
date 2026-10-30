#pragma once

#include <INA229/ISPI.hpp>
#include <INA229/Registers.hpp>
#include <INA229/Utility.hpp>
#include <array>

namespace INA229::TestSupport {

// Minimal register model shared by measurement mocks and fault-injection tests.
// Conversion timing and accumulator behavior require separate hardware tests.
class RegisterSPI : public ISPI {
public:
  RegisterSPI() { reset(); }

  void reset() {
    registers.fill(0);
    (*this)[Register::ADC_CONFIG] = Mask::AdcConfig::RESET_VALUE;
    (*this)[Register::SHUNT_CAL] = 0x1000; // Datasheet power-on calibration.
    (*this)[Register::MANUFACTURER_ID] = Mask::ManufacturerID::EXPECTED;
    (*this)[Register::DEVICE_ID] = Mask::DeviceID::EXPECTED_DIE | 1;
    (*this)[Register::DIAG_ALRT] = Mask::DiagAlert::RESET_VALUE;
  }

  uint64_t &operator[](Register reg) {
    return registers[static_cast<size_t>(reg)];
  }

  void read(uint8_t command, uint8_t *destination, size_t length) override {
    const auto reg = static_cast<Register>(command >> 2);
    const uint64_t value = (*this)[reg];
    for (size_t index = 0; index < length; ++index) {
      destination[index] =
          static_cast<uint8_t>(value >> (8 * (length - index - 1)));
    }
  }

  void write(uint8_t command, const uint8_t *source, size_t length) override {
    const auto reg = static_cast<Register>(command >> 2);
    const uint64_t value = DataUtility::packBytes({source, length});
    // Fault injection drops configuration writes but permits a device reset.
    if (reg == Register::CONFIG && (value & Mask::Config::RST)) {
      reset();
    } else if (!dropConfigWrites) {
      (*this)[reg] = value;
    }
  }

  bool dropConfigWrites = false;

private:
  std::array<uint64_t, 64> registers{};
};

} // namespace INA229::TestSupport
