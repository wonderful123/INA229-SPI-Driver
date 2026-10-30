# Changelog

## Unreleased

- Renamed the project to INA229 SPI Driver
  (package identifier: INA229_SPI_Driver).
- Added INA229_SPI_Driver.h as the primary header, retaining previous includes.

- Updated the recommended Arduino ESP32 core to 3.3.12, which fixes its internal
  deprecated touch-driver usage upstream without suppressing warnings.
- Added focused examples for all measurements, energy/charge accumulation,
  precision and temperature compensation, and diagnostics/recovery.
- Added wiring and usage guidance and local PlatformIO example environments.
- Extended packaging CI to compile every example on both toolchains.
- Reworked the README with board compatibility, a compiled quick start, API
  reference, example navigation, and validation evidence.
- Corrected the portable usage snippet to include the public header.

## 1.0.0

Initial packaged release for ESP32 Arduino and PlatformIO, with a portable C++17
core for other applications.

- Added the Arduino SPI adapter, BasicReadings example, package metadata, and
  MIT license.
- Added the public `INA229Driver.h` header; `INA229.h` remains a compatibility
  include.
- Corrected temperature-coefficient encoding to 1 ppm/°C per register step.
- Added configuration readback, safe failure state after initialization errors,
  and reset recovery that checks all configuration registers.
- Rejected nonfinite configuration values and invalid register settings.
- Fixed CMake consumer linking and test exit status, restored the existing tests,
  and added regression, C++17 compatibility, and SPI-adapter checks.
- Added native sanitizer tests and Arduino/PlatformIO packaging CI workflows.

Physical-device timing and measurement accuracy have not yet been verified.
Alert configuration and a separate single-shot retrigger API are not included.
