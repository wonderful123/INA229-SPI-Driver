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
  // Type r in Serial Monitor to start a new energy/charge measurement window.
  if (Serial.available() && Serial.read() == 'r') {
    const auto status = sensor.resetAccumulators();
    if (status != INA229::Status::OK) {
      Serial.println("Accumulator reset failed.");
      return;
    }
    Serial.println("Energy and charge cleared.");
    delay(10); // Allow fresh default conversions after reset/recovery.
  }
  // Read diagnostic flags once; reading this register can clear latched flags.
  const auto rawFlags = sensor.getDiagnosticFlags();
  if (!rawFlags) {
    Serial.println("Cannot read diagnostics.");
    return;
  }
  const auto flags = static_cast<INA229::AlertStatus>(*rawFlags);
  if (INA229::hasFlag(flags, INA229::AlertStatus::ENERGY_OF) ||
      INA229::hasFlag(flags, INA229::AlertStatus::CHARGE_OF) ||
      INA229::hasFlag(flags, INA229::AlertStatus::MATH_OF)) {
    Serial.println(
        "Overflow detected: totals may be invalid; type r to clear.");
    return;
  }
  const auto energy = sensor.getEnergyJoules();
  const auto charge = sensor.getChargeCoulombs();
  if (!energy || !charge) {
    Serial.println("Totals unavailable.");
    return;
  }
  Serial.print("Energy (Wh): ");
  Serial.println(*energy / 3600.0, 6);
  Serial.print("Net charge (Ah, signed): ");
  Serial.println(*charge / 3600.0, 6);
  // Energy is unsigned; charge is signed. These totals are not a battery SOC.
  // Power-on reset or begin() erases accumulators; recovery cannot restore
  // them.
}
