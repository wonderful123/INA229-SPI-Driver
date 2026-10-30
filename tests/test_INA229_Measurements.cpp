#include "mocks/MockISPI.hpp"
#include <INA229/Config.hpp>
#include <INA229/INA229.hpp>
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>

using namespace INA229;

namespace {

// Test constants
constexpr double DEFAULT_SHUNT_RESISTANCE = 0.002;
constexpr double DEFAULT_MAX_CURRENT = 10.0;
constexpr double TOLERANCE = 1e-9;
constexpr double SCALING_TOLERANCE = 1e-6;
constexpr int32_t FIVE_AMP_COUNTS = 262144;

} // namespace

// Test Fixture

class INA229MeasurementsTest : public ::testing::Test {
protected:
  MockSPI mockSpi;

  void SetUp() override { setupBasicMocks(mockSpi); }

  void setupBasicMocks(MockSPI &spi) { spi.useRegisterModel(); }

  void setRegisterValue(Register reg, int32_t value, MockSPI *spi = nullptr) {
    MockSPI &targetSpi = spi ? *spi : mockSpi;
    const uint32_t raw = (static_cast<uint32_t>(value) & 0xFFFFF) << 4;
    targetSpi.setRegisterValue(reg, raw, 3);
  }

  Status initializeDriver(INA229_Driver &driver,
                          double shunt = DEFAULT_SHUNT_RESISTANCE,
                          double maxCurrent = DEFAULT_MAX_CURRENT) {
    return driver.begin(
        {.shuntResistor = shunt, .maxExpectedCurrent = maxCurrent});
  }

  Status initializeDriver(INA229_Driver &driver, const DriverConfig &config) {
    return driver.begin(config);
  }
};

// Parameterized Tests: Current Accuracy

struct CurrentTestCase {
  int32_t counts;
  std::string name;
};

class INA229CurrentParamTest
    : public INA229MeasurementsTest,
      public ::testing::WithParamInterface<CurrentTestCase> {};

TEST_P(INA229CurrentParamTest, VerifiesCalculatedAmps) {
  const auto counts = GetParam().counts;
  INA229_Driver driver{mockSpi};

  ASSERT_EQ(initializeDriver(driver), Status::OK);

  setRegisterValue(Register::CURRENT, counts);

  // Also verify a zero bus-voltage reading in the zero-current case.
  if (counts == 0) {
    setRegisterValue(Register::VBUS, 0);
  }

  const double expected = static_cast<double>(counts) * driver.getCurrentLsb();
  EXPECT_NEAR(driver.getCurrentAmps().value(), expected, TOLERANCE);

  // Verify sign correctness
  if (counts > 0) {
    EXPECT_GT(driver.getCurrentAmps().value(), 0.0);
  } else if (counts < 0) {
    EXPECT_LT(driver.getCurrentAmps().value(), 0.0);
  } else {
    EXPECT_DOUBLE_EQ(driver.getBusVoltage().value(), 0.0);
  }
}

INSTANTIATE_TEST_SUITE_P(
    CurrentReadings, INA229CurrentParamTest,
    ::testing::Values(CurrentTestCase{1000, "PositiveValue"},
                      CurrentTestCase{-500, "NegativeValue"},
                      CurrentTestCase{0, "ZeroValue"}),
    [](const ::testing::TestParamInfo<CurrentTestCase> &info) {
      return info.param.name;
    });

// Behavioral Tests: Scaling & Units

TEST_F(INA229MeasurementsTest, Current_ScalesLinearlyWithMaxCurrent) {
  INA229_Driver driver1{mockSpi};

  MockSPI mockSpi2;
  setupBasicMocks(mockSpi2);
  INA229_Driver driver2{mockSpi2};

  // Driver2 has 2x max current → 2x LSB → 2x current reading
  ASSERT_EQ(initializeDriver(driver1, DEFAULT_SHUNT_RESISTANCE, 10.0),
            Status::OK);
  ASSERT_EQ(initializeDriver(driver2, DEFAULT_SHUNT_RESISTANCE, 20.0),
            Status::OK);

  constexpr int32_t counts = 10000;
  setRegisterValue(Register::CURRENT, counts, &mockSpi);
  setRegisterValue(Register::CURRENT, counts, &mockSpi2);

  const double amps1 = driver1.getCurrentAmps().value();
  const double amps2 = driver2.getCurrentAmps().value();

  EXPECT_NEAR(amps2, amps1 * 2.0, SCALING_TOLERANCE);
  EXPECT_GT(amps1, 0.0);
}

TEST_F(INA229MeasurementsTest, Current_ReturnsAmperesNotMilliamperes) {
  INA229_Driver driver{mockSpi};
  ASSERT_EQ(initializeDriver(driver), Status::OK);

  setRegisterValue(Register::CURRENT, FIVE_AMP_COUNTS);

  // Verify units are Amperes (not mA)
  EXPECT_NEAR(driver.getCurrentAmps().value(), 5.0, 0.1);
}

// Bus Voltage Measurement Tests

TEST_F(INA229MeasurementsTest, BusVoltage_PositiveValue) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Simulate 12V: 12V / 195.3125µV = ~61440 raw, left-aligned
  uint32_t rawValue = static_cast<uint32_t>(12.0 / 195.3125e-6);
  uint32_t leftAligned = (rawValue & 0xFFFFF) << 4;

  mockSpi.setRegisterValue(Register::VBUS, leftAligned, 3);

  double voltage = driver.getBusVoltage().value();

  EXPECT_NEAR(voltage, 12.0, 0.1);
}

TEST_F(INA229MeasurementsTest, BusVoltage_Zero) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  mockSpi.setRegisterValue(Register::VBUS, 0x000000, 3);

  double voltage = driver.getBusVoltage().value();

  EXPECT_NEAR(voltage, 0.0, 1e-6);
}

TEST_F(INA229MeasurementsTest, BusVoltage_InVolts) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Simulate 5V
  uint32_t rawValue = static_cast<uint32_t>(5.0 / 195.3125e-6);
  uint32_t leftAligned = (rawValue & 0xFFFFF) << 4;

  mockSpi.setRegisterValue(Register::VBUS, leftAligned, 3);

  double voltage = driver.getBusVoltage().value();

  // Should be ~5.0 (Volts), not 5000 (millivolts)
  EXPECT_NEAR(voltage, 5.0, 0.1);
}

TEST_F(INA229MeasurementsTest, BusVoltage_MaximumRange) {
  INA229_Driver driver{mockSpi};
  DriverConfig config{.shuntResistor = 0.002, .maxExpectedCurrent = 10.0};
  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Exercise a large unsigned bus reading, left-aligned for the wire format.
  uint32_t maxPositiveRaw = 0x7FFFF0;

  mockSpi.setRegisterValue(Register::VBUS, maxPositiveRaw, 3);

  double voltage = driver.getBusVoltage().value();

  // 0x7FFFF * 195.3125µV = 102.39V
  // This synthetic value exceeds the 85 V physical input limit.
  EXPECT_GT(voltage, 0.0);
  EXPECT_NEAR(voltage, 102.39, 0.1);
}

// Temperature Measurement Tests

TEST_F(INA229MeasurementsTest, Temperature_RoomTemperature) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Simulate 25°C: 25 / 0.0078125 = 3200
  int16_t rawValue = static_cast<int16_t>(25.0 / 0.0078125);

  mockSpi.setRegisterValue(Register::DIETEMP, static_cast<uint16_t>(rawValue),
                           2);

  double temp = driver.getDieTemperature().value();

  EXPECT_NEAR(temp, 25.0, 0.1);
}

TEST_F(INA229MeasurementsTest, Temperature_NegativeValue) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Simulate -10°C
  int16_t rawValue = static_cast<int16_t>(-10.0 / 0.0078125);

  mockSpi.setRegisterValue(Register::DIETEMP, static_cast<uint16_t>(rawValue),
                           2);

  double temp = driver.getDieTemperature().value();

  EXPECT_NEAR(temp, -10.0, 0.1);
}

TEST_F(INA229MeasurementsTest, Temperature_InCelsius) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Simulate 100°C
  int16_t rawValue = static_cast<int16_t>(100.0 / 0.0078125);

  mockSpi.setRegisterValue(Register::DIETEMP, static_cast<uint16_t>(rawValue),
                           2);

  double temp = driver.getDieTemperature().value();

  // Should be in Celsius
  EXPECT_NEAR(temp, 100.0, 0.1);
  EXPECT_GT(temp, 50.0); // Sanity check
  EXPECT_LT(temp, 150.0);
}

TEST_F(INA229MeasurementsTest, Temperature_Zero) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  mockSpi.setRegisterValue(Register::DIETEMP, 0x0000, 2);

  double temp = driver.getDieTemperature().value();

  EXPECT_NEAR(temp, 0.0, 0.01);
}

// Power Measurement Tests

TEST_F(INA229MeasurementsTest, Power_PositiveValue) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Simulate some power reading
  mockSpi.setRegisterValue(Register::POWER, 0x100000, 3);

  double power = driver.getPowerWatts().value();

  EXPECT_GT(power, 0.0);
}

TEST_F(INA229MeasurementsTest, Power_Zero) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  mockSpi.setRegisterValue(Register::POWER, 0x000000, 3);

  double power = driver.getPowerWatts().value();

  EXPECT_NEAR(power, 0.0, 1e-9);
}

TEST_F(INA229MeasurementsTest, Power_InWatts) {
  INA229_Driver driver{mockSpi};
  DriverConfig config{.shuntResistor = 0.002, .maxExpectedCurrent = 10.0};
  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  uint32_t rawPower = 100000;
  mockSpi.setRegisterValue(Register::POWER, rawPower, 3);

  // Driver: raw * 3.2 * LSB
  double expectedPower =
      static_cast<double>(rawPower) * 3.2 * driver.getCurrentLsb();
  EXPECT_NEAR(driver.getPowerWatts().value(), expectedPower, 1e-7);
}

TEST_F(INA229MeasurementsTest, Power_ScalesWithCurrentLSB) {
  MockSPI spi1, spi2;
  INA229_Driver driver1{spi1};
  INA229_Driver driver2{spi2};

  DriverConfig config1{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  DriverConfig config2{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 20.0,
  };

  // Setup SPIs
  for (auto *spi : {&spi1, &spi2}) {
    setupBasicMocks(*spi);
  }

  ASSERT_EQ(driver1.begin(config1), Status::OK);
  ASSERT_EQ(driver2.begin(config2), Status::OK);

  // Same raw power value
  uint32_t rawPower = 0x100000;
  spi1.setRegisterValue(Register::POWER, rawPower, 3);
  spi2.setRegisterValue(Register::POWER, rawPower, 3);

  double power1 = driver1.getPowerWatts().value();
  double power2 = driver2.getPowerWatts().value();

  // Power scales with current LSB, so should be approximately doubled
  EXPECT_NEAR(power2, power1 * 2.0, power1 * 0.01);
}

// Energy Accumulation Tests

TEST_F(INA229MeasurementsTest, Energy_PositiveValue) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  mockSpi.setRegisterValue(Register::ENERGY, 0x1000000000ULL, 5);

  double energy = driver.getEnergyJoules().value();

  EXPECT_GT(energy, 0.0);
}

TEST_F(INA229MeasurementsTest, Energy_Zero) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  mockSpi.setRegisterValue(Register::ENERGY, 0x0000000000ULL, 5);

  double energy = driver.getEnergyJoules().value();

  EXPECT_NEAR(energy, 0.0, 1e-12);
}

TEST_F(INA229MeasurementsTest, Energy_InJoules) {
  INA229_Driver driver{mockSpi};
  DriverConfig config{.shuntResistor = 0.002, .maxExpectedCurrent = 10.0};
  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  uint64_t rawEnergy = 0x1234567890ULL;
  mockSpi.setRegisterValue(Register::ENERGY, rawEnergy, 5);

  // Driver: raw * 16.0 * 3.2 * LSB
  double expectedEnergy =
      static_cast<double>(rawEnergy) * 16.0 * 3.2 * driver.getCurrentLsb();
  EXPECT_NEAR(driver.getEnergyJoules().value(), expectedEnergy, 1e-5);
}

TEST_F(INA229MeasurementsTest, Energy_40BitRange) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Maximum 40-bit value (unsigned for energy)
  uint64_t maxValue = 0xFFFFFFFFFFULL;

  mockSpi.setRegisterValue(Register::ENERGY, maxValue, 5);

  double energy = driver.getEnergyJoules().value();

  // Should handle large values without overflow
  EXPECT_GT(energy, 0.0);
  EXPECT_FALSE(std::isinf(energy));
}

// Charge Accumulation Tests

TEST_F(INA229MeasurementsTest, Charge_PositiveValue) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  mockSpi.setRegisterValue(Register::CHARGE, 0x1000000000ULL, 5);

  double charge = driver.getChargeCoulombs().value();

  EXPECT_GT(charge, 0.0);
}

TEST_F(INA229MeasurementsTest, Charge_NegativeValue) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Negative charge (40-bit signed)
  uint64_t negativeValue = 0xFFFFFFFFFFULL; // -1 in 40-bit

  mockSpi.setRegisterValue(Register::CHARGE, negativeValue, 5);

  double charge = driver.getChargeCoulombs().value();

  EXPECT_LT(charge, 0.0);
}

TEST_F(INA229MeasurementsTest, Charge_Zero) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  mockSpi.setRegisterValue(Register::CHARGE, 0x0000000000ULL, 5);

  double charge = driver.getChargeCoulombs().value();

  EXPECT_NEAR(charge, 0.0, 1e-12);
}

TEST_F(INA229MeasurementsTest, Charge_InCoulombs) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Charge LSB = Current LSB
  double currentLsb = 10.0 / 524288.0;

  // Simulate 100C
  int64_t rawValue = static_cast<int64_t>(100.0 / currentLsb);

  mockSpi.setRegisterValue(
      Register::CHARGE, static_cast<uint64_t>(rawValue) & 0xFFFFFFFFFFULL, 5);

  double charge = driver.getChargeCoulombs().value();

  EXPECT_NEAR(charge, 100.0, 1.0);
}

// Current LSB Getter Test

TEST_F(INA229MeasurementsTest, CurrentLSB_ReturnsConfiguredValue) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  double lsb = driver.getCurrentLsb();
  double expectedLsb = 10.0 / 524288.0;

  EXPECT_NEAR(lsb, expectedLsb, 1e-12);
}

// Integration / Real-World Scenario Tests

TEST_F(INA229MeasurementsTest, Scenario_12VRail_5A) {
  INA229_Driver driver{mockSpi};
  DriverConfig config{.shuntResistor = 0.002, .maxExpectedCurrent = 10.0};
  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  double currentLsb = driver.getCurrentLsb();
  double busVoltageLsb = 195.3125e-6;
  double powerLsb = 3.2 * currentLsb;

  // 12V Setup
  uint32_t vbusRaw = static_cast<uint32_t>(12.0 / busVoltageLsb);
  mockSpi.setRegisterValue(Register::VBUS, (vbusRaw & 0xFFFFF) << 4, 3);

  // 5A Setup
  int32_t currentRaw = static_cast<int32_t>(5.0 / currentLsb);
  mockSpi.setRegisterValue(
      Register::CURRENT, (static_cast<uint32_t>(currentRaw) & 0xFFFFF) << 4, 3);

  // Calculated Power (P = V * I)
  uint32_t powerRaw = static_cast<uint32_t>(60.0 / powerLsb);
  mockSpi.setRegisterValue(Register::POWER, powerRaw & 0xFFFFFF, 3);

  EXPECT_NEAR(driver.getBusVoltage().value(), 12.0, 0.01);
  EXPECT_NEAR(driver.getCurrentAmps().value(), 5.0, 0.01);
  EXPECT_NEAR(driver.getPowerWatts().value(), 60.0, 0.1);
}

TEST_F(INA229MeasurementsTest, Scenario_LowCurrent_100mA) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.1, // 100mΩ for better resolution at low currents
      .maxExpectedCurrent = 0.4,
      .range = ShuntRange::RANGE_41mV,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  double currentLsb = config.calculateLsb();

  // Setup 0.1A current
  int32_t currentRaw = static_cast<int32_t>(0.1 / currentLsb);
  mockSpi.setRegisterValue(
      Register::CURRENT, (static_cast<uint32_t>(currentRaw) & 0xFFFFF) << 4, 3);

  double current = driver.getCurrentAmps().value();

  EXPECT_NEAR(current, 0.1, 0.01);
}

TEST_F(INA229MeasurementsTest, Scenario_BidirectionalCurrent) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  double currentLsb = config.calculateLsb();

  // Test positive current
  int32_t positiveRaw = static_cast<int32_t>(3.0 / currentLsb);
  mockSpi.setRegisterValue(Register::CURRENT,
                           (static_cast<uint32_t>(positiveRaw) & 0xFFFFF) << 4,
                           3);

  double positiveCurrent = driver.getCurrentAmps().value();
  EXPECT_GT(positiveCurrent, 0.0);

  // Test negative current (regenerative braking scenario)
  int32_t negativeRaw = static_cast<int32_t>(-3.0 / currentLsb);
  mockSpi.setRegisterValue(Register::CURRENT,
                           (static_cast<uint32_t>(negativeRaw) & 0xFFFFF) << 4,
                           3);

  double negativeCurrent = driver.getCurrentAmps().value();
  EXPECT_LT(negativeCurrent, 0.0);
}

// Data Validity Tests

TEST_F(INA229MeasurementsTest, AllMeasurements_NoNaN) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Set all registers to valid values
  mockSpi.setRegisterValue(Register::CURRENT, 0x100000, 3);
  mockSpi.setRegisterValue(Register::VBUS, 0x100000, 3);
  mockSpi.setRegisterValue(Register::DIETEMP, 0x1000, 2);
  mockSpi.setRegisterValue(Register::POWER, 0x100000, 3);
  mockSpi.setRegisterValue(Register::ENERGY, 0x1000000000ULL, 5);
  mockSpi.setRegisterValue(Register::CHARGE, 0x1000000000ULL, 5);

  EXPECT_FALSE(std::isnan(driver.getCurrentAmps().value()));
  EXPECT_FALSE(std::isnan(driver.getBusVoltage().value()));
  EXPECT_FALSE(std::isnan(driver.getDieTemperature().value()));
  EXPECT_FALSE(std::isnan(driver.getPowerWatts().value()));
  EXPECT_FALSE(std::isnan(driver.getEnergyJoules().value()));
  EXPECT_FALSE(std::isnan(driver.getChargeCoulombs().value()));
}

TEST_F(INA229MeasurementsTest, AllMeasurements_NoInfinity) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Set all registers to maximum valid values
  mockSpi.setRegisterValue(Register::CURRENT, 0xFFFFF0, 3);
  mockSpi.setRegisterValue(Register::VBUS, 0xFFFFF0, 3);
  mockSpi.setRegisterValue(Register::DIETEMP, 0xFFFF, 2);
  mockSpi.setRegisterValue(Register::POWER, 0xFFFFFF, 3);
  mockSpi.setRegisterValue(Register::ENERGY, 0xFFFFFFFFFFULL, 5);
  mockSpi.setRegisterValue(Register::CHARGE, 0x7FFFFFFFFFULL, 5);

  EXPECT_FALSE(std::isinf(driver.getCurrentAmps().value()));
  EXPECT_FALSE(std::isinf(driver.getBusVoltage().value()));
  EXPECT_FALSE(std::isinf(driver.getDieTemperature().value()));
  EXPECT_FALSE(std::isinf(driver.getPowerWatts().value()));
  EXPECT_FALSE(std::isinf(driver.getEnergyJoules().value()));
  EXPECT_FALSE(std::isinf(driver.getChargeCoulombs().value()));
}

// Consistency Tests

TEST_F(INA229MeasurementsTest, RepeatedReads_Consistent) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  mockSpi.setRegisterValue(Register::CURRENT, 0x100000, 3);

  double current1 = driver.getCurrentAmps().value();
  double current2 = driver.getCurrentAmps().value();
  double current3 = driver.getCurrentAmps().value();

  // Multiple reads should give identical results
  EXPECT_EQ(current1, current2);
  EXPECT_EQ(current2, current3);
}

// Boundary Tests

TEST_F(INA229MeasurementsTest, Current_MaximumPositive) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Maximum positive 20-bit value
  mockSpi.setRegisterValue(Register::CURRENT, 0x7FFFF0, 3);

  double current = driver.getCurrentAmps().value();

  EXPECT_GT(current, 0.0);
  EXPECT_FALSE(std::isinf(current));
}

TEST_F(INA229MeasurementsTest, Current_MaximumNegative) {
  INA229_Driver driver{mockSpi};
  DriverConfig config{.shuntResistor = 0.002, .maxExpectedCurrent = 10.0};
  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // Minimum negative 20-bit value: 0x800000 (left-aligned)
  // Driver logic: (0x800000 << 8) >> 12 = -524288
  mockSpi.setRegisterValue(Register::CURRENT, 0x800000, 3);

  double expectedCurrent = -524288.0 * driver.getCurrentLsb();
  EXPECT_NEAR(driver.getCurrentAmps().value(), expectedCurrent, 1e-7);
}

TEST_F(INA229MeasurementsTest, Temperature_ExtremeCold) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // -40°C
  int16_t rawValue = static_cast<int16_t>(-40.0 / 0.0078125);
  mockSpi.setRegisterValue(Register::DIETEMP, static_cast<uint16_t>(rawValue),
                           2);

  double temp = driver.getDieTemperature().value();

  EXPECT_NEAR(temp, -40.0, 0.5);
}

TEST_F(INA229MeasurementsTest, Temperature_ExtremeHot) {
  INA229_Driver driver{mockSpi};

  DriverConfig config{
      .shuntResistor = 0.002,
      .maxExpectedCurrent = 10.0,
  };

  ASSERT_EQ(initializeDriver(driver, config), Status::OK);

  // 125°C
  int16_t rawValue = static_cast<int16_t>(125.0 / 0.0078125);
  mockSpi.setRegisterValue(Register::DIETEMP, static_cast<uint16_t>(rawValue),
                           2);

  double temp = driver.getDieTemperature().value();

  EXPECT_NEAR(temp, 125.0, 0.5);
}
