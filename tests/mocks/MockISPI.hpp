#pragma once

#include "RegisterSPI.hpp"
#include <gmock/gmock.h>

namespace INA229 {

class MockSPI : public ISPI {
public:
  MOCK_METHOD(void, read, (uint8_t command, uint8_t *rx, size_t len),
              (override));
  MOCK_METHOD(void, write, (uint8_t command, const uint8_t *tx, size_t len),
              (override));

  void useRegisterModel() {
    registerModel.reset();
    EXPECT_CALL(*this, read(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(
            ::testing::Invoke(&registerModel, &TestSupport::RegisterSPI::read));
    EXPECT_CALL(*this, write(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Invoke(&registerModel,
                                          &TestSupport::RegisterSPI::write));
  }

  // Retain a width-specific expectation to check measurement transaction sizes.
  void setRegisterValue(Register reg, uint64_t value, uint8_t width) {
    registerModel[reg] = value;
    const uint8_t command = DataUtility::makeCommandByte(reg, true);
    EXPECT_CALL(*this, read(command, ::testing::_, width))
        .WillRepeatedly(
            ::testing::Invoke(&registerModel, &TestSupport::RegisterSPI::read));
  }

private:
  TestSupport::RegisterSPI registerModel;
};

} // namespace INA229
