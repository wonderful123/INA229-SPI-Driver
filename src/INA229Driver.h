#pragma once

#if __cplusplus < 201703L
#error                                                                         \
    "INA229Driver requires C++17 or newer. Enable -std=gnu++17 in your project."
#endif

#include "INA229/INA229.hpp"

#if defined(ARDUINO)
#include "INA229/ArduinoSPI.hpp"
#endif
