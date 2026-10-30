#pragma once

#include <cstdint>
#include <string>
#include <vector>

constexpr uint8_t OUTPUT = 1;
constexpr uint8_t LOW = 0;
constexpr uint8_t HIGH = 1;

namespace ArduinoTest {
inline std::vector<std::string> events;
}

inline void pinMode(uint8_t pin, uint8_t mode) {
  ArduinoTest::events.push_back("pin:" + std::to_string(pin) + ":" +
                                std::to_string(mode));
}
inline void digitalWrite(uint8_t pin, uint8_t level) {
  ArduinoTest::events.push_back("cs:" + std::to_string(pin) + ":" +
                                std::to_string(level));
}
inline void delayMicroseconds(unsigned int delay) {
  ArduinoTest::events.push_back("delay:" + std::to_string(delay));
}
