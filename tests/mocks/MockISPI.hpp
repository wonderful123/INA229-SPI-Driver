// test/mocks/MockISPI.hpp
#pragma once

#include <INA229/INA229.hpp> // Required for Register enum and DataUtility
#include <INA229/ISPI.hpp>
#include <INA229/Utility.hpp>
#include <gmock/gmock.h>
#include <vector>

namespace INA229 {

class MockSPI : public ISPI {
public:
  MOCK_METHOD(void, read, (uint8_t command, uint8_t *rx, size_t len),
              (override));
  MOCK_METHOD(void, write, (uint8_t command, const uint8_t *tx, size_t len),
              (override));

  /**
   * @brief Helper to simulate register data in the Mock SPI.
   * This belongs in the mock class so all tests can use it.
   */
  void setRegisterValue(Register reg, uint64_t value, uint8_t width) {
    using ::testing::_;
    using ::testing::Eq;

    uint8_t cmd = DataUtility::makeCommandByte(reg, true);

    // 1. Create the data buffer
    std::vector<uint8_t> buffer(width);
    for (int i = 0; i < width; ++i) {
      buffer[i] = static_cast<uint8_t>((value >> ((width - 1 - i) * 8)) & 0xFF);
    }

    // 2. Use a lambda to perform the copy.
    // Capturing 'buffer' by value [buffer] ensures the data lives
    // inside the Mock's expectation until the test is over.
    EXPECT_CALL(*this, read(Eq(cmd), _, Eq(width)))
        .WillRepeatedly([buffer](uint8_t cmd, uint8_t *rx, size_t len) {
          std::copy(buffer.begin(), buffer.end(), rx);
        });
  }
};

} // namespace INA229