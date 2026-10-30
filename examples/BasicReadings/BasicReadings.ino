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
  delay(1000); // Default conversions finish well before the next read.
  const auto voltage = sensor.getBusVoltage();
  const auto current = sensor.getCurrentAmps();
  if (!voltage || !current) {
    Serial.print("Read failed: ");
    Serial.println(INA229::statusToString(sensor.getLastStatus()).data());
    return; // Never turn a failed read into a displayed zero.
  }
  Serial.print("Bus voltage (V): ");
  Serial.println(*voltage, 4);
  Serial.print("Current (A): ");
  Serial.println(*current, 6);
}
