#include <INA229Driver.h>

// ESP32 Dev Module: default SPI pins are SCK=18, MISO=19, MOSI=23.
// Connect INA229 CS to GPIO 5; use compatible supply and logic voltages.
constexpr uint8_t CHIP_SELECT = 5;
INA229::ArduinoSPI sensorSPI{SPI, CHIP_SELECT};
INA229::INA229_Driver sensor{sensorSPI};
bool sensorReady = false;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  sensorSPI.begin();

  INA229::DriverConfig config;
  config.shuntResistor = 0.002;   // Replace with the actual shunt resistance.
  config.maxExpectedCurrent = 10; // Amperes; must fit the selected shunt range.
  const auto status = sensor.begin(config);
  sensorReady = status == INA229::Status::OK;
  if (!sensorReady) {
    Serial.println("INA229 initialization failed. Check SPI wiring and power.");
  }
}

void loop() {
  if (sensorReady) {
    // Permit default conversions and averaging to complete before reading.
    delay(1000);
    const auto busVoltage = sensor.getBusVoltage();
    const auto current = sensor.getCurrentAmps();
    if (busVoltage && current) {
      Serial.print("Bus voltage (V): ");
      Serial.println(*busVoltage, 4);
      Serial.print("Current (A): ");
      Serial.println(*current, 6);
    } else {
      Serial.println("INA229 health check failed.");
    }
  } else {
    delay(1000);
  }
}
