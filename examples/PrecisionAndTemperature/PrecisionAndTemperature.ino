#include <INA229_SPI_Driver.h>

// ESP32 Dev Module wiring and shunt selection: see examples/README.md.
constexpr uint8_t CHIP_SELECT = 5;
INA229::ArduinoSPI sensorSPI{SPI, CHIP_SELECT};
INA229::INA229_Driver sensor{sensorSPI};
bool sensorReady = false;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  sensorSPI.begin();

  INA229::DriverConfig config;
  config.shuntResistor = 0.002;   // Use the actual shunt resistance in ohms.
  config.maxExpectedCurrent = 10; // Amperes, including expected peaks.
  // 10 A * 0.002 ohm = 20 mV, within the +/-40.96 mV narrow range.
  config.range = INA229::ShuntRange::RANGE_41mV;
  config.avg = INA229::AvgCount::AVG_16;
  config.vBusTime = INA229::ConversionTime::TIME_1052us;
  config.vShuntTime = INA229::ConversionTime::TIME_1052us;
  config.tempTime = INA229::ConversionTime::TIME_1052us;
  // Example only: replace 50 with the coefficient from your shunt datasheet.
  // Compensation uses INA229 die temperature: thermal coupling matters.
  config.tempCompensation = true;
  config.shuntTempCoeff = 50; // ppm/degree C, encoded as register value 50.
  const char *error = nullptr;
  if (config.validate(&error) != INA229::Status::OK) {
    Serial.print("Invalid configuration: ");
    Serial.println(error);
    return;
  }
  const auto status = sensor.begin(config);
  sensorReady = status == INA229::Status::OK;
  if (!sensorReady) {
    Serial.print("Initialization failed: ");
    Serial.println(INA229::statusToString(status).data());
    return;
  }
  Serial.println("INA229 ready. Open Serial Monitor at 115200 baud.");
}

void loop() {
  if (!sensorReady) {
    delay(1000);
    return; // Correct the wiring/configuration and reset the board to retry.
  }
  // Three 1052 us conversions * 16 averages is about 50.5 ms per cycle.
  delay(1000);
  const auto current = sensor.getCurrentAmps();
  const auto shuntVoltage = sensor.getShuntVoltageVolts();
  const auto temperature = sensor.getDieTemperature();
  if (!current || !shuntVoltage || !temperature) {
    Serial.println("Measurement unavailable.");
    return;
  }
  Serial.print("Current (A): ");
  Serial.println(*current, 6);
  Serial.print("Shunt voltage (uV): ");
  Serial.println(*shuntVoltage * 1e6, 3);
  Serial.print("Die temperature (C): ");
  Serial.println(*temperature, 2);
  // Averaging reduces noise; it does not fix shunt tolerance or wiring errors.
}
