#include <INA229Driver.h>

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
  delay(1000);
  // Values are read sequentially, rather than as an atomic measurement
  // snapshot.
  const auto voltage = sensor.getBusVoltage();
  const auto shuntVoltage = sensor.getShuntVoltageVolts();
  const auto current = sensor.getCurrentAmps();
  const auto temperature = sensor.getDieTemperature();
  const auto power = sensor.getPowerWatts();
  const auto energy = sensor.getEnergyJoules();
  const auto charge = sensor.getChargeCoulombs();
  if (!voltage || !shuntVoltage || !current || !temperature || !power ||
      !energy || !charge) {
    Serial.println("Measurement unavailable; check wiring and device power.");
    return;
  }
  Serial.print("Bus (V): ");
  Serial.println(*voltage, 4);
  Serial.print("Shunt (mV): ");
  Serial.println(*shuntVoltage * 1000.0, 6);
  Serial.print("Current (A, signed): ");
  Serial.println(*current, 6);
  Serial.print("Die temperature (C): ");
  Serial.println(*temperature, 2);
  Serial.print("Power (W): ");
  Serial.println(*power, 4);
  Serial.print("Energy since reset (J): ");
  Serial.println(*energy, 4);
  Serial.print("Charge since reset (C, signed): ");
  Serial.println(*charge, 4);
}
