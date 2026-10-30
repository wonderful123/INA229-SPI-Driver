#pragma once

#include "ISPI.hpp"
#include <Arduino.h>
#include <SPI.h>

namespace INA229 {

// Call SPIClass::begin() with the board's pins before using this adapter.
// The bus must outlive the adapter; serialize access when sharing the bus.
class ArduinoSPI final : public ISPI {
public:
  explicit ArduinoSPI(SPIClass &bus, uint8_t chipSelect,
                      uint32_t clockHz = 1000000)
      : bus(bus), chipSelect(chipSelect),
        settings(clockHz, MSBFIRST, SPI_MODE1) {}

  void begin() {
    pinMode(chipSelect, OUTPUT);
    digitalWrite(chipSelect, HIGH);
  }

  void read(uint8_t command, uint8_t *destination, size_t length) override {
    beginTransfer(command);
    for (size_t index = 0; index < length; ++index) {
      destination[index] = bus.transfer(0);
    }
    endTransfer();
  }

  void write(uint8_t command, const uint8_t *source, size_t length) override {
    beginTransfer(command);
    for (size_t index = 0; index < length; ++index) {
      bus.transfer(source[index]);
    }
    endTransfer();
  }

private:
  SPIClass &bus;
  uint8_t chipSelect;
  SPISettings settings;

  void beginTransfer(uint8_t command) {
    bus.beginTransaction(settings);
    digitalWrite(chipSelect, LOW);
    delayMicroseconds(1);
    bus.transfer(command);
  }

  void endTransfer() {
    digitalWrite(chipSelect, HIGH);
    delayMicroseconds(1);
    bus.endTransaction();
  }
};

} // namespace INA229
