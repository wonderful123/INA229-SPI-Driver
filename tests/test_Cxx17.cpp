#include <INA229.h> // Verify the compatibility include remains usable.
#include <INA229/Utility.hpp>
#include <INA229Driver.h>
#include <array>

// This target is compiled as strict C++17 independently of the GoogleTest
// suite.
int main() {
  INA229::DriverConfig config;
  config.shuntResistor = 0.002;
  config.maxExpectedCurrent = 10;
  if (config.validate() != INA229::Status::OK ||
      config.getShuntCalValue() != 500) {
    return 1;
  }
  const uint8_t raw[]{0x12, 0x34};
  const std::array<uint8_t, 2> bytes{0x12, 0x34};
  return INA229::DataUtility::packBytes(raw, 2) == 0x1234 &&
                 INA229::DataUtility::packBytes(raw) == 0x1234 &&
                 INA229::DataUtility::packBytes(bytes) == 0x1234
             ? 0
             : 1;
}
