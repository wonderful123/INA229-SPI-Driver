#include "mocks/RegisterSPI.hpp"
#include <INA229/INA229.hpp>
#include <algorithm>
#include <gtest/gtest.h>
#include <limits>

namespace {

using namespace INA229;
using TestSupport::RegisterSPI;

DriverConfig validConfig() {
  return {.shuntResistor = 0.002, .maxExpectedCurrent = 10};
}

// A read-only identity failure following reset after a prior successful begin.
struct FailingIdentitySpi : RegisterSPI {
  bool failIdentity = false;
  void read(uint8_t command, uint8_t *destination, size_t length) override {
    RegisterSPI::read(command, destination, length);
    if (failIdentity &&
        static_cast<Register>(command >> 2) == Register::MANUFACTURER_ID)
      std::fill(destination, destination + length, 0);
  }
};

// Drop writes to one register while still allowing the initialization reset.
struct DroppedRegisterSpi : RegisterSPI {
  Register droppedRegister = Register::CONFIG;

  void write(uint8_t command, const uint8_t *source, size_t length) override {
    const auto reg = static_cast<Register>(command >> 2);
    const auto value = DataUtility::packBytes({source, length});
    const bool isReset = reg == Register::CONFIG && (value & Mask::Config::RST);
    if (reg != droppedRegister || isReset) {
      RegisterSPI::write(command, source, length);
    }
  }
};

TEST(Baseline, NotInitialized) {
  RegisterSPI spi;
  INA229_Driver driver{spi};
  EXPECT_FALSE(driver.getCurrentAmps());
  EXPECT_EQ(driver.getLastStatus(), Status::ERR_DEV_NOT_READY);
}

TEST(Baseline, Measurements) {
  RegisterSPI spi;
  INA229_Driver driver{spi};
  ASSERT_EQ(driver.begin(validConfig()), Status::OK);
  spi[Register::CURRENT] = 0x400000;
  EXPECT_DOUBLE_EQ(driver.getCurrentAmps().value(), 5);
  spi[Register::VSHUNT] = 0xfffff0;
  EXPECT_DOUBLE_EQ(driver.getShuntVoltageVolts().value(), -312.5e-9);
  spi[Register::VBUS] = 245760ull << 4;
  EXPECT_DOUBLE_EQ(driver.getBusVoltage().value(), 48);
  spi[Register::DIETEMP] = static_cast<uint16_t>(-128);
  EXPECT_DOUBLE_EQ(driver.getDieTemperature().value(), -1);
  spi[Register::POWER] = 1000;
  EXPECT_DOUBLE_EQ(driver.getPowerWatts().value(), 1000 * 3.2 * 10 / 524288);
  spi[Register::ENERGY] = 1000;
  EXPECT_DOUBLE_EQ(driver.getEnergyJoules().value(),
                   1000 * 16 * 3.2 * 10 / 524288);
  spi[Register::CHARGE] = 0xffffffffff;
  EXPECT_DOUBLE_EQ(driver.getChargeCoulombs().value(), -10.0 / 524288);
}

TEST(Baseline, NarrowRange) {
  RegisterSPI spi;
  INA229_Driver driver{spi};
  auto config = validConfig();
  config.range = ShuntRange::RANGE_41mV;
  ASSERT_EQ(driver.begin(config), Status::OK);
  EXPECT_EQ(spi[Register::SHUNT_CAL], 2000u);
  spi[Register::VSHUNT] = 16;
  EXPECT_DOUBLE_EQ(driver.getShuntVoltageVolts().value(), 78.125e-9);
}

TEST(Baseline, UtilityBoundaryValues) {
  EXPECT_EQ(DataUtility::signExtend20Bit(0x800000), -524288);
  EXPECT_EQ(DataUtility::signExtend20Bit(0x7ffff0), 524287);
  EXPECT_EQ(DataUtility::signExtend40Bit(0x8000000000), -549755813888ll);
  EXPECT_EQ(DataUtility::signExtend40Bit(0x7fffffffff), 549755813887ll);
}

TEST(Regression, TemperatureCoefficientUsesOnePpmPerLsb) {
  auto config = validConfig();
  config.tempCompensation = true;
  config.shuntTempCoeff = 100;
  ASSERT_EQ(config.validate(), Status::OK);
  EXPECT_EQ(config.getTempCoValue(), 100);
}

TEST(Regression, RejectNanResistor) {
  auto config = validConfig();
  config.shuntResistor = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
}

TEST(Regression, RejectNanCurrent) {
  auto config = validConfig();
  config.maxExpectedCurrent = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
}

TEST(Regression, RejectNanTempco) {
  auto config = validConfig();
  config.tempCompensation = true;
  config.shuntTempCoeff = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
}

TEST(Regression, DroppedWritesDoNotSucceed) {
  RegisterSPI spi;
  spi.dropConfigWrites = true;
  INA229_Driver driver{spi};
  EXPECT_NE(driver.begin(validConfig()), Status::OK);
  EXPECT_NE(spi[Register::SHUNT_CAL], validConfig().getShuntCalValue());
}

TEST(Regression, MeasurementsRejectedAfterFailedBegin) {
  FailingIdentitySpi spi;
  INA229_Driver driver{spi};
  ASSERT_EQ(driver.begin(validConfig()), Status::OK);
  spi.failIdentity = true;
  ASSERT_NE(driver.begin(validConfig()), Status::OK);
  EXPECT_FALSE(driver.getCurrentAmps());
}

TEST(Regression, DetectResetWhenCalibrationEqualsDefault) {
  RegisterSPI spi;
  INA229_Driver driver{spi};
  auto config = validConfig();
  config.shuntResistor = 0.016384;
  config.range = ShuntRange::RANGE_41mV;
  config.maxExpectedCurrent = 2.5;
  config.mode = AdcMode::SHUTDOWN_0;
  ASSERT_EQ(config.getShuntCalValue(), 4096);
  ASSERT_EQ(driver.begin(config), Status::OK);
  spi.reset();
  ASSERT_TRUE(driver.isHealthy());
  EXPECT_EQ(spi[Register::CONFIG], config.getConfigValue());
  EXPECT_EQ(spi[Register::ADC_CONFIG], config.getAdcConfigValue());
}

TEST(Regression, VerifiesEveryConfigurationWrite) {
  for (Register reg : {Register::CONFIG, Register::ADC_CONFIG,
                       Register::SHUNT_CAL, Register::SHUNT_TEMPCO}) {
    DroppedRegisterSpi spi;
    spi.droppedRegister = reg;
    auto config = validConfig();
    config.range = ShuntRange::RANGE_41mV;
    config.mode = AdcMode::SHUTDOWN_0;
    config.tempCompensation = true;
    config.shuntTempCoeff = 100;
    INA229_Driver driver{spi};
    EXPECT_EQ(driver.begin(config), Status::ERR_COMM)
        << "register " << static_cast<unsigned>(reg);
    EXPECT_FALSE(driver.getCurrentAmps());
  }
}

TEST(Regression, RecoveryVerifiesAllRegisters) {
  RegisterSPI spi;
  INA229_Driver driver{spi};
  auto config = validConfig();
  config.tempCompensation = true;
  config.shuntTempCoeff = 100;
  ASSERT_EQ(driver.begin(config), Status::OK);
  for (Register reg : {Register::CONFIG, Register::ADC_CONFIG,
                       Register::SHUNT_CAL, Register::SHUNT_TEMPCO}) {
    const uint16_t corruptionMask =
        reg == Register::CONFIG ? Mask::Config::ADCRANGE : 1u;
    spi[reg] ^= corruptionMask;
    ASSERT_TRUE(driver.isHealthy())
        << "register " << static_cast<unsigned>(reg);
    EXPECT_EQ(spi[Register::CONFIG], config.getConfigValue());
    EXPECT_EQ(spi[Register::ADC_CONFIG], config.getAdcConfigValue());
    EXPECT_EQ(spi[Register::SHUNT_CAL], config.getShuntCalValue());
    EXPECT_EQ(spi[Register::SHUNT_TEMPCO], config.getTempCoValue());
  }
  spi.reset();
  spi.dropConfigWrites = true;
  EXPECT_FALSE(driver.isHealthy());
  EXPECT_EQ(driver.getLastStatus(), Status::ERR_COMM);
  EXPECT_FALSE(driver.getCurrentAmps());
}

TEST(Regression, InvalidReinitializationClearsState) {
  RegisterSPI spi;
  INA229_Driver driver{spi};
  ASSERT_EQ(driver.begin(validConfig()), Status::OK);
  EXPECT_EQ(driver.begin({}), Status::ERR_INVALID_CONFIG);
  EXPECT_DOUBLE_EQ(driver.getCurrentLsb(), 0);
  EXPECT_FALSE(driver.getCurrentAmps());
  ASSERT_EQ(driver.begin(validConfig()), Status::OK);
  EXPECT_TRUE(driver.getCurrentAmps());
}

TEST(Regression, NonfiniteValuesRejectedAndSafeToEncode) {
  for (double value : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()}) {
    auto config = validConfig();
    config.shuntResistor = value;
    EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
    EXPECT_EQ(config.getShuntCalValue(), 0);
    config = validConfig();
    config.maxExpectedCurrent = value;
    EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
    EXPECT_EQ(config.getShuntCalValue(), 0);
    config = validConfig();
    config.tempCompensation = true;
    config.shuntTempCoeff = value;
    EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
    EXPECT_EQ(config.getTempCoValue(), 0);
  }
}

TEST(Regression, TemperatureCoefficientBoundariesAndRounding) {
  auto config = validConfig();
  config.tempCompensation = true;
  config.shuntTempCoeff = 16383;
  EXPECT_EQ(config.validate(), Status::OK);
  EXPECT_EQ(config.getTempCoValue(), 16383);
  config.shuntTempCoeff = 99.6;
  EXPECT_EQ(config.getTempCoValue(), 100);
  config.shuntTempCoeff = 1e6;
  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
  EXPECT_EQ(config.getTempCoValue(), 16383);
}

TEST(Regression, InvalidEnumsRejected) {
  auto config = validConfig();
  config.range = static_cast<ShuntRange>(2);
  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
  config = validConfig();
  config.mode = static_cast<AdcMode>(16);
  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
  config = validConfig();
  config.avg = static_cast<AvgCount>(8);
  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
  config = validConfig();
  config.vBusTime = static_cast<ConversionTime>(8);
  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
  config = validConfig();
  config.vShuntTime = static_cast<ConversionTime>(8);
  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
  config = validConfig();
  config.tempTime = static_cast<ConversionTime>(8);
  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
}

} // namespace
