# Changelog

## 1.0.0 — 2026-10-31

Initial public release of **INA229 SPI Driver**, a portable C++17 driver with
Arduino and PlatformIO ESP32 support.

- Read voltage, signed current, power, energy, signed charge, and die temperature
  in physical units, with explicit status and optional measurement results.
- Configure shunt range, conversion times, averaging, and temperature compensation.
- Verify configuration readback and restore settings after detected mismatches.
- Reject nonfinite inputs and invalid register settings; encode temperature
  coefficients correctly at 1 ppm/°C per register step.
- Provide the `INA229_SPI_Driver.h` public header and retain `INA229.h` and
  `INA229Driver.h` compatibility includes.
- Include five standalone Arduino examples, local PlatformIO environments,
  wiring guidance, an API overview, and an MIT license.
- Pass 117 native checks, including regression coverage, C++17 compatibility,
  SPI-adapter checks, and sanitizer validation.
- Compile every example under Arduino ESP32 core 3.3.12 and PlatformIO's Arduino
  2.0.17 toolchain. Core 3.3.12 fixes the upstream touch-driver deprecation.
- Gate GitHub releases on native and embedded checks; attach installable
  Arduino/PlatformIO archives and SHA-256 checksums.
- Remove redundant datasheet and Windows metadata copies and unused embedded
  scaffolding from the native test runner.

### Scope and limitations

Physical-device timing and measurement accuracy have not yet been verified.
Alert-threshold configuration and a separate single-shot retrigger API are not
included. Classic AVR boards are not supported by their default toolchains.
Configuration recovery cannot restore lost accumulated energy or charge.

GitHub publication does not automatically register the library with Arduino
Library Manager or the PlatformIO Registry.
