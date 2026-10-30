# INA229 SPI driver

C++17 driver for the TI INA229 power, energy, and charge monitor. It supports
continuous or single-shot measurement configuration, signed current and charge,
shunt temperature compensation, diagnostic reads, and configuration recovery.

## Arduino IDE / Arduino CLI

The library uses Arduino's standard `src/` layout and `library.properties`.
For local installation, download a ZIP of this repository and select
**Sketch > Include Library > Add .ZIP Library** in the Arduino IDE.
Choose **ESP32 Dev Module** with Espressif's Arduino ESP32 core **3.x or newer**,
then open **File > Examples > INA229Driver > BasicReadings**.
Use `#include <INA229Driver.h>` to include the driver and Arduino SPI adapter.

Arduino CLI can compile a local checkout after the board core is installed:

```sh
arduino-cli core install esp32:esp32@3.3.0 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli compile --fqbn esp32:esp32:esp32 --library . examples/BasicReadings
```

ESP32 is the advertised Arduino architecture. Classic AVR boards such as the
Uno/Mega are not supported: their default toolchains do not provide the C++17
standard library required for `std::optional` and `std::string_view`.
Other modern platforms can use the portable core with their own `ISPI` adapter,
but are not yet advertised as validated Arduino targets.

## PlatformIO

Install from the repository until a release is published to the PlatformIO
Registry. An ESP32 project can use:

```ini
[env:esp32dev]
platform = espressif32@6.10.0
board = esp32dev
framework = arduino
monitor_speed = 115200
build_unflags = -std=gnu++11
build_flags = -std=gnu++17
lib_deps = https://github.com/wonderful123/INA229-SPI-Library.git
```

Set the C++17 flags on the project as well as the library because sketch files
include the public headers. Copy `examples/BasicReadings/BasicReadings.ino` into
your project's `src/`, or run `pio run -d examples/BasicReadings` after these changes
have been pushed to the repository. That example configuration downloads the
Git dependency; the packaging CI instead builds against an archive of the current
checkout so it checks unreleased changes too.

## Build and test

The driver requires C++17; the test suite requires C++20. CMake 3.14 or newer
is required for native builds. Tests use GoogleTest and GoogleMock. CMake uses an installed GoogleTest package or fetches the pinned
GoogleTest 1.17.0 commit from GitHub.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

For offline testing, provide an existing GoogleTest source checkout with
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest`. For a dependency-free
library build, use `-DBUILD_TESTING=OFF`. A consumer can use:

```cmake
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/INA229-SPI-Library ina229)
target_link_libraries(my_application PRIVATE INA229::Driver)
```

## Validated builds

The BasicReadings example has been compiled for ESP32 Dev Module with:

| Build system | Versions | Result |
| --- | --- | --- |
| Arduino CLI | CLI 1.3.1, ESP32 core 3.3.0 | Passed |
| PlatformIO | Core 6.1.18, espressif32 6.10.0, Arduino core 2.0.17 | Passed |
| Native CMake | GCC 14, C++17 core and C++20 tests | 117 checks passed |
| Native sanitizers | Undefined behavior and float-to-integer overflow checks | 117 checks passed |

The Arduino core emits a deprecation warning for its own legacy touch driver
when compiled with all warnings enabled. No physical-device validation is claimed.

## Use

For Arduino, use `INA229::ArduinoSPI` as shown in BasicReadings. It uses SPI
mode 1, MSB first, and a default 1 MHz clock (INA229 supports up to 10 MHz).
Call `SPI.begin()` and the adapter's `begin()` before initializing the driver.
For other environments, implement `INA229::ISPI` using your board's peripheral.
Each read/write must keep chip select asserted for the command byte followed by exactly `len` payload
bytes. Transfer bytes most significant first. Follow the bundled TI datasheet's
SPI mode and timing requirements. The driver handles command construction and
payload byte ordering; the adapter handles chip select and the SPI transaction.
The adapter must outlive the driver. Serialize access to the driver and bus.

```cpp
#include <INA229/INA229Driver.hpp>

void measure(INA229::ISPI &spi) {
    INA229::INA229_Driver driver{spi};
    INA229::DriverConfig config;
    config.shuntResistor = 0.002;     // ohms
    config.maxExpectedCurrent = 10; // amperes
    config.tempCompensation = true;
    config.shuntTempCoeff = 100;    // ppm/degree C; register value is also 100
    if (driver.begin(config) != INA229::Status::OK) {
        return;
    }
    // Wait for the configured conversion/averaging period before using results.
    // Poll getDiagnosticFlags() and inspect AlertStatus::CONV_READY if needed.
    if (auto amps = driver.getCurrentAmps()) {
        // Use *amps; a missing value means the health check failed.
    }
}
```

`begin()` resets the device, validates finite inputs and register ranges, and
verifies configuration readback. Any failed `begin()` leaves measurement access
disabled; call `begin()` successfully before trying measurements again.
Temperature coefficients are unsigned, 0–16383 ppm/degree C, and are rounded to
1 ppm/degree C register steps. A coefficient is only used and validated when
temperature compensation is enabled. Register encoders return zero for
nonfinite input instead of performing an invalid integer conversion.

Health is cached for up to seven measurement accesses. `isHealthy()` forces a
configuration check immediately. Configuration mismatch triggers restoration and
verification of CONFIG, ADC_CONFIG, SHUNT_CAL, and SHUNT_TEMPCO. Recovery does not
restore lost accumulated energy/charge or wait for fresh conversions. After a
reset, allow the configured conversion period before consuming results.
Diagnostic reads can clear latched alert flags as specified in the datasheet.

The current SPI interface has no per-transaction error return. Configuration
readback detects mismatches, but the driver cannot report every individual
transport failure, nor does a cached health check guarantee a fresh sample.
Adapters must not throw during health checks, which are `noexcept`.

## Scope

An Arduino SPI adapter is included. There are currently no APIs for programming
alert thresholds/polarity/latching. Single-shot mode starts on `begin()`; there is no
separate retrigger API. These are extension points, not features provided by this
version. No platform-specific Arduino or PlatformIO toolchain is required for the
native tests. Native tests simulate SPI; physical device timing and accuracy still
require hardware verification. See `docs/INA229 Datasheet.pdf` for register details.

## Formatting

C++ files follow the checked-in `.clang-format` configuration (clang-format 18):

```sh
clang-format -i src/*.h src/INA229/*.hpp src/*.cpp tests/*.cpp tests/mocks/*.hpp tests/arduino/*.h examples/BasicReadings/*.ino
```

## Releasing to the library registries

Metadata alone does not publish the library. Before publishing:

1. Run native tests and the packaging workflow (strict Arduino Lint, Arduino
   ESP32 compilation, PlatformIO packing, and compilation against that archive).
2. Keep the versions in `library.properties` and `library.json` in sync, commit
   the changes, and push a matching version tag, for example `v1.0.0` for the
   first release. Use a new version if 1.0.0 is already registered.
3. For PlatformIO, sign in using `pio account login`, then run `pio pkg publish`
   from the library root. Publishing requires the owner's registry credentials.
4. For Arduino Library Manager, submit the repository URL through the
   [Arduino library registry](https://github.com/arduino/library-registry#adding-a-library-to-library-manager).
   After acceptance, compliant new version tags are indexed automatically.
   Change the packaging workflow's `library-manager` mode from `submit` to
   `update` once the library is listed.

The library name must be available in each registry. The Git dependency and ZIP
installation methods work before registry registration; registry search/install
requires publication and indexing. This repository does not contain registry
credentials or automatically publish on push.

## License

Driver code is released under the [MIT license](LICENSE). The bundled TI
datasheet retains Texas Instruments' copyright and is excluded from the
PlatformIO distribution archive.
