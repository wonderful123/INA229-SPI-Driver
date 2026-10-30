#pragma once

#include "Arduino.h"
#include <cstddef>

constexpr uint8_t MSBFIRST = 1;
constexpr uint8_t SPI_MODE1 = 1;

struct SPISettings {
  SPISettings(uint32_t clock, uint8_t order, uint8_t mode)
      : clock(clock), order(order), mode(mode) {}
  uint32_t clock;
  uint8_t order;
  uint8_t mode;
};

class SPIClass {
public:
  SPISettings lastSettings{0, 0, 0};
  std::vector<uint8_t> incoming;
  size_t nextByte = 0;

  void beginTransaction(const SPISettings &settings) {
    lastSettings = settings;
    ArduinoTest::events.push_back("begin");
  }
  uint8_t transfer(uint8_t byte) {
    ArduinoTest::events.push_back("transfer:" + std::to_string(byte));
    return nextByte < incoming.size() ? incoming[nextByte++] : 0;
  }
  void endTransaction() { ArduinoTest::events.push_back("end"); }
};
