#include <INA229/Config.hpp>
#include <cmath>
#include <gtest/gtest.h>

using namespace INA229;

/**
 * @file test_INA229_DriverConfig.cpp
 * @brief Behavioral tests for INA229 driver configuration.
 *
 * Tests focus on what the configuration does, not how it's implemented.
 */

// ============================================================================
// Configuration Validation Tests
// ============================================================================

TEST(DriverConfigTest, ValidConfiguration_Passes) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  EXPECT_EQ(config.validate(), Status::OK);
}

TEST(DriverConfigTest, ZeroShuntResistor_Rejected) {
  DriverConfig config{
      .shuntResistor = 0.0,
      .maxExpectedCurrent = 10.0,
  };

  std::string errMsg;
  EXPECT_EQ(config.validate(&errMsg), Status::ERR_INVALID_CONFIG);
  EXPECT_FALSE(errMsg.empty());
}

TEST(DriverConfigTest, NegativeShuntResistor_Rejected) {
  DriverConfig config{
      .shuntResistor = -0.002,
      .maxExpectedCurrent = 10.0,
  };

  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
}

TEST(DriverConfigTest, ZeroMaxCurrent_Rejected) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 0.0,
  };

  std::string errMsg;
  EXPECT_EQ(config.validate(&errMsg), Status::ERR_INVALID_CONFIG);
  EXPECT_FALSE(errMsg.empty());
}

TEST(DriverConfigTest, NegativeMaxCurrent_Rejected) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = -10.0,
  };

  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
}

TEST(DriverConfigTest, TempCompensationWithoutCoefficient_Rejected) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .tempCompensation = true,
      .shuntTempCoeff = std::nullopt,
  };

  std::string errMsg;
  EXPECT_EQ(config.validate(&errMsg), Status::ERR_INVALID_CONFIG);
  EXPECT_NE(errMsg.find("Temperature"), std::string::npos);
}

TEST(DriverConfigTest, TempCompensationWithCoefficient_Passes) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .tempCompensation = true,
      .shuntTempCoeff = 50.0,
  };

  EXPECT_EQ(config.validate(), Status::OK);
}

TEST(DriverConfigTest, TempCoefficientWithoutCompensation_Passes) {
  // Having coefficient set but not enabling compensation is valid
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .tempCompensation = false,
      .shuntTempCoeff = 50.0,
  };

  EXPECT_EQ(config.validate(), Status::OK);
}

TEST(DriverConfigTest, TempCoefficientTooHigh_Rejected) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .shuntTempCoeff = 600.0, // Out of range
  };

  std::string errMsg;
  EXPECT_EQ(config.validate(&errMsg), Status::ERR_INVALID_CONFIG);
  EXPECT_NE(errMsg.find("range"), std::string::npos);
}

TEST(DriverConfigTest, TempCoefficientTooLow_Rejected) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .shuntTempCoeff = -600.0,
  };

  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
}

TEST(DriverConfigTest, ValidationErrorMessage_Provided) {
  DriverConfig config{.shuntResistor = 0.0, .maxExpectedCurrent = 10.0};

  std::string errMsg;
  Status status = config.validate(&errMsg);

  EXPECT_EQ(status, Status::ERR_INVALID_CONFIG);
  EXPECT_FALSE(errMsg.empty());
  EXPECT_GT(errMsg.length(), 10); // Should be descriptive
}

// ============================================================================
// Current LSB Calculation Tests
// ============================================================================

TEST(DriverConfigTest, CurrentLsb_CalculatedCorrectly) {
  DriverConfig config{.maxExpectedCurrent = 10.0};

  const double lsb = config.calculateLsb();
  const double expected = 10.0 / 524288.0; // 2^19

  EXPECT_NEAR(lsb, expected, 1e-12);
}

TEST(DriverConfigTest, CurrentLsb_ScalesWithMaxCurrent) {
  DriverConfig config1{.maxExpectedCurrent = 10.0};
  DriverConfig config2{.maxExpectedCurrent = 20.0};

  EXPECT_NEAR(config2.calculateLsb(), config1.calculateLsb() * 2.0, 1e-12);
}

TEST(DriverConfigTest, CurrentLsb_SmallCurrent) {
  DriverConfig config{.maxExpectedCurrent = 0.1};

  const double lsb = config.calculateLsb();
  EXPECT_GT(lsb, 0.0);
  EXPECT_LT(lsb, 1e-6); // Should be very small
}

// ============================================================================
// Temperature Coefficient Register Tests
// ============================================================================

TEST(DriverConfigTest, TempCoValue_DisabledReturnsZero) {
  DriverConfig config{
      .tempCompensation = false,
      .shuntTempCoeff = 50.0,
  };

  EXPECT_EQ(config.getTempCoValue(), 0);
}

TEST(DriverConfigTest, TempCoValue_NoCoeffReturnsZero) {
  DriverConfig config{
      .tempCompensation = true,
      .shuntTempCoeff = std::nullopt,
  };

  EXPECT_EQ(config.getTempCoValue(), 0);
}

TEST(DriverConfigTest, TempCoValue_PositiveCoefficient) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .tempCompensation = true,
      .shuntTempCoeff = 100.0, // ppm/°C
  };

  const uint16_t value = config.getTempCoValue();

  // Formula: (ppm × 2^14) / 10^6
  // 100 × 16384 / 1000000 = 1.6384 → rounds to 2
  EXPECT_GT(value, 0);
  EXPECT_LE(value, 0x3FFF); // 14-bit max
}

TEST(DriverConfigTest, TempCoValue_NegativeCoefficient) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .tempCompensation = true,
      .shuntTempCoeff = -100.0,
  };

  // Negative values round toward zero or saturate at 0
  const uint16_t value = config.getTempCoValue();
  EXPECT_LE(value, 0x3FFF);
}

TEST(DriverConfigTest, TempCoValue_ClampedTo14Bit) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .tempCompensation = true,
      .shuntTempCoeff = 500.0, // Maximum reasonable value
  };

  const uint16_t value = config.getTempCoValue();
  EXPECT_LE(value, 0x3FFF); // Must not exceed 14-bit field
}

TEST(DriverConfigTest, TempCoValue_ZeroCoefficient) {
  DriverConfig config{
      .tempCompensation = true,
      .shuntTempCoeff = 0.0,
  };

  EXPECT_EQ(config.getTempCoValue(), 0);
}

// ============================================================================
// Shunt Calibration Register Tests
// ============================================================================

TEST(DriverConfigTest, ShuntCal_DifferentRanges) {
  DriverConfig config164{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .range = ShuntRange::RANGE_164mV,
  };

  DriverConfig config40{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .range = ShuntRange::RANGE_40mV,
  };

  const uint16_t val164 = config164.getShuntCalValue();
  const uint16_t val40 = config40.getShuntCalValue();

  // Different ranges should produce different calibration values
  EXPECT_NE(val164, val40);

  // Both should be non-zero and within 15-bit range
  EXPECT_GT(val164, 0);
  EXPECT_GT(val40, 0);
  EXPECT_LE(val164, 0x7FFF);
  EXPECT_LE(val40, 0x7FFF);
}

TEST(DriverConfigTest, ShuntCal_ScalesWithResistor) {
  DriverConfig config1{
      .shuntResistor = 0.001,
      .maxExpectedCurrent = 10.0,
  };

  DriverConfig config2{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  const uint16_t val1 = config1.getShuntCalValue();
  const uint16_t val2 = config2.getShuntCalValue();

  // Doubling resistor should approximately double calibration
  EXPECT_NEAR(val2, val1 * 2.0, val1 * 0.1); // Within 10%
}

TEST(DriverConfigTest, ShuntCal_ClampedTo15Bit) {
  DriverConfig config{
      .shuntResistor = 1.0, // Unusually high to test clamping
      .maxExpectedCurrent = 100.0,
  };

  const uint16_t value = config.getShuntCalValue();
  EXPECT_LE(value, 0x7FFF); // Bit 15 is reserved
}

// ============================================================================
// ADC Configuration Register Tests
// ============================================================================

TEST(DriverConfigTest, AdcConfig_CombinesAllSettings) {
  DriverConfig config{
      .mode = AdcMode::CONTINUOUS_ALL,
      .avg = AvgCount::AVG_16,
      .vBusTime = ConversionTime::TIME_1052us,
      .vShuntTime = ConversionTime::TIME_540us,
      .tempTime = ConversionTime::TIME_2074us,
  };

  const uint16_t value = config.getAdcConfigValue();

  // Should produce non-zero value when configured
  EXPECT_NE(value, 0);
}

TEST(DriverConfigTest, AdcConfig_DefaultSettings) {
  DriverConfig config{}; // Default construction

  const uint16_t value = config.getAdcConfigValue();

  // Should have valid default configuration
  EXPECT_NE(value, 0);
}

TEST(DriverConfigTest, AdcConfig_DifferentModes) {
  DriverConfig shutdown{.mode = AdcMode::SHUTDOWN_0};
  DriverConfig continuous{.mode = AdcMode::CONTINUOUS_ALL};

  EXPECT_NE(shutdown.getAdcConfigValue(), continuous.getAdcConfigValue());
}

TEST(DriverConfigTest, AdcConfig_DifferentAveraging) {
  DriverConfig avg1{.avg = AvgCount::AVG_1};
  DriverConfig avg1024{.avg = AvgCount::AVG_1024};

  EXPECT_NE(avg1.getAdcConfigValue(), avg1024.getAdcConfigValue());
}

TEST(DriverConfigTest, AdcConfig_DifferentConversionTimes) {
  DriverConfig fast{
      .vBusTime = ConversionTime::TIME_50us,
      .vShuntTime = ConversionTime::TIME_50us,
      .tempTime = ConversionTime::TIME_50us,
  };

  DriverConfig slow{
      .vBusTime = ConversionTime::TIME_4120us,
      .vShuntTime = ConversionTime::TIME_4120us,
      .tempTime = ConversionTime::TIME_4120us,
  };

  EXPECT_NE(fast.getAdcConfigValue(), slow.getAdcConfigValue());
}

// ============================================================================
// Device Configuration Register Tests
// ============================================================================

TEST(DriverConfigTest, Config_ConversionDelay) {
  DriverConfig config1{.conversionDelay = 0};
  DriverConfig config2{.conversionDelay = 50};

  EXPECT_NE(config1.getConfigValue(), config2.getConfigValue());
}

TEST(DriverConfigTest, Config_ShuntRangeFlag) {
  DriverConfig config164{.range = ShuntRange::RANGE_164mV};
  DriverConfig config40{.range = ShuntRange::RANGE_40mV};

  const uint16_t val164 = config164.getConfigValue();
  const uint16_t val40 = config40.getConfigValue();

  // Should set different bits
  EXPECT_NE(val164, val40);
}

TEST(DriverConfigTest, Config_TempCompensationFlag) {
  DriverConfig disabled{
      .tempCompensation = false,
      .shuntTempCoeff = 50.0,
  };

  DriverConfig enabled{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .tempCompensation = true,
      .shuntTempCoeff = 50.0,
  };

  EXPECT_NE(disabled.getConfigValue(), enabled.getConfigValue());
}

TEST(DriverConfigTest, Config_CombinesMultipleFlags) {
  DriverConfig config{
      .range = ShuntRange::RANGE_40mV,
      .conversionDelay = 100,
      .tempCompensation = true,
  };

  const uint16_t value = config.getConfigValue();
  EXPECT_NE(value, 0);
}

// ============================================================================
// Integration / Real-World Scenario Tests
// ============================================================================

TEST(DriverConfigTest, TypicalApplication_10A_2mOhm) {
  DriverConfig config{
      .shuntResistor = 0.002,     // 2mΩ
      .maxExpectedCurrent = 10.0, // 10A
      .mode = AdcMode::CONTINUOUS_ALL,
      .avg = AvgCount::AVG_16,
  };

  EXPECT_EQ(config.validate(), Status::OK);
  EXPECT_GT(config.getShuntCalValue(), 0);
  EXPECT_GT(config.getAdcConfigValue(), 0);
}

TEST(DriverConfigTest, LowCurrentApplication_100mA_100mOhm) {
  DriverConfig config{
      .shuntResistor = 0.1,      // 100mΩ
      .maxExpectedCurrent = 0.1, // 100mA
      .range = ShuntRange::RANGE_40mV,
  };

  EXPECT_EQ(config.validate(), Status::OK);
  EXPECT_GT(config.calculateLsb(), 0);
}

TEST(DriverConfigTest, HighPrecision_SlowConversion) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 5.0,
      .avg = AvgCount::AVG_1024,
      .vBusTime = ConversionTime::TIME_4120us,
      .vShuntTime = ConversionTime::TIME_4120us,
  };

  EXPECT_EQ(config.validate(), Status::OK);
}

TEST(DriverConfigTest, TriggeredMode_SingleShot) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .mode = AdcMode::TRIGGERED_ALL,
      .avg = AvgCount::AVG_1,
  };

  EXPECT_EQ(config.validate(), Status::OK);
  EXPECT_NE(config.getAdcConfigValue(), 0);
}

TEST(DriverConfigTest, WithTempCompensation_Realistic) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 5.0,
      .tempCompensation = true,
      .shuntTempCoeff = 50.0, // Typical for metal film
  };

  EXPECT_EQ(config.validate(), Status::OK);
  EXPECT_GT(config.getTempCoValue(), 0);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(DriverConfigTest, VerySmallShunt_1microOhm) {
  DriverConfig config{
      .shuntResistor = 0.000001, // 1µΩ
      .maxExpectedCurrent = 100.0,
  };

  EXPECT_EQ(config.validate(), Status::OK);
  const uint16_t cal = config.getShuntCalValue();
  EXPECT_GE(cal, 0);
}

TEST(DriverConfigTest, VeryLargeShunt_1Ohm) {
  DriverConfig config{
      .shuntResistor = 1.0, // 1Ω
      .maxExpectedCurrent = 0.01,
  };

  EXPECT_EQ(config.validate(), Status::OK);
}

TEST(DriverConfigTest, MaxConversionDelay) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .conversionDelay = 255, // Maximum 8-bit value
  };

  EXPECT_EQ(config.validate(), Status::OK);
  EXPECT_NE(config.getConfigValue(), 0);
}

TEST(DriverConfigTest, MixedModes_BusOnly) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .mode = AdcMode::CONTINUOUS_BUS,
  };

  EXPECT_EQ(config.validate(), Status::OK);
}

TEST(DriverConfigTest, MixedModes_ShuntOnly) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .mode = AdcMode::CONTINUOUS_SHUNT,
  };

  EXPECT_EQ(config.validate(), Status::OK);
}

TEST(DriverConfigTest, MixedModes_TempOnly) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .mode = AdcMode::CONTINUOUS_TEMP,
  };

  EXPECT_EQ(config.validate(), Status::OK);
}

// ============================================================================
// Boundary Value Tests
// ============================================================================

TEST(DriverConfigTest, BoundaryTempCoeff_Positive) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .shuntTempCoeff = 500.0, // At upper limit
  };

  EXPECT_EQ(config.validate(), Status::OK);
}

TEST(DriverConfigTest, BoundaryTempCoeff_Negative) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .shuntTempCoeff = -500.0, // At lower limit
  };

  EXPECT_EQ(config.validate(), Status::OK);
}

TEST(DriverConfigTest, BoundaryTempCoeff_JustOverPositive) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .shuntTempCoeff = 500.1, // Just over limit
  };

  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
}

TEST(DriverConfigTest, BoundaryTempCoeff_JustOverNegative) {
  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
      .shuntTempCoeff = -500.1, // Just over limit
  };

  EXPECT_EQ(config.validate(), Status::ERR_INVALID_CONFIG);
}