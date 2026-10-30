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
  delay(2000);
  // Force a hardware check instead of relying on cached measurement health.
  // A configuration mismatch triggers automatic restoration and verification.
  if (!sensor.isHealthy()) {
    Serial.print("Device unhealthy: ");
    Serial.println(INA229::statusToString(sensor.getLastStatus()).data());
    return; // Retry next loop without repeatedly resetting the whole device.
  }
  delay(10); // Recovery may have restarted conversions (default cycle ~3.2 ms).
  const auto rawFlags = sensor.getDiagnosticFlags();
  const auto manufacturer = sensor.getManufacturerId();
  const auto device = sensor.getDeviceId();
  if (!rawFlags || !manufacturer || !device) {
    Serial.println("Diagnostic read failed.");
    return;
  }
  Serial.print("Manufacturer ID (expected 5449): 0x");
  Serial.println(*manufacturer, HEX);
  Serial.print("Device ID (die 229, revision in low nibble): 0x");
  Serial.println(*device, HEX);
  Serial.print("Diagnostic flags: 0x");
  Serial.println(*rawFlags, HEX);
  // Interpret this saved value: a second register read may clear latched flags.
  const auto flags = static_cast<INA229::AlertStatus>(*rawFlags);
  if (!INA229::hasFlag(flags, INA229::AlertStatus::MEMSTAT)) {
    Serial.println("Device trim-memory checksum error.");
  }
  if (INA229::hasFlag(flags, INA229::AlertStatus::MATH_OF)) {
    Serial.println("Math overflow: review the shunt range/calibration.");
  }
  if (INA229::hasFlag(flags, INA229::AlertStatus::CONV_READY)) {
    Serial.println("Conversion-ready flag was set.");
  }
  // Healthy does not report whether a reset occurred. Hardware reset loses
  // energy/charge history; restoring configuration cannot recover those totals.
}
