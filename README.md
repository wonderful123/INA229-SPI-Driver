# INA229Driver

**Power monitoring in real units, with clear errors and a testable SPI interface.**

A C++17 driver for the **Texas Instruments INA229**. Read bus voltage,
bidirectional current, power, accumulated energy and charge, and die temperature
without writing register-packing or conversion code yourself.

Use the included Arduino SPI adapter on **ESP32**, install through **PlatformIO
or Arduino ZIP import**, or bring your own SPI implementation for another C++17
application.

[Quick start](#quick-start) · [Installation](#installation) · [Examples](#examples) · [API](#measurement-api) · [Validation](#tested-and-validated)

## Why choose this driver?

- **Useful values immediately.** Measurement methods return volts, amperes,
  watts, joules, coulombs, and degrees Celsius. Current and charge retain their
  sign for bidirectional measurements.
- **Failures stay visible.** Initialization returns a status code, invalid
  configuration can return an explanation, and measurements use `std::optional`
  so an unavailable reading is distinguishable from a real zero.
- **Configuration is verified.** Initialization checks register readback.
  Health checks can restore and verify configuration after a mismatch.
- **Precision settings are explicit.** Select shunt range, conversion times,
  averaging, and shunt temperature compensation in one `DriverConfig`.
- **Easy to start, easy to test.** Five standalone examples cover common tasks.
  The core depends on an `ISPI` interface rather than Arduino, so applications
  can supply a different transport or a test double.

The driver and Arduino adapter are MIT licensed. There are no third-party
runtime library dependencies beyond the platform's C++17 standard library and,
for Arduino, its built-in SPI library.

## Is it right for your project?

| Your setup | Support |
| --- | --- |
| TI INA229 connected over SPI to an ESP32 | Included adapter and examples; builds verified on both toolchains below |
| Arduino IDE / Arduino CLI | ESP32 core **3.3.12** recommended; use `#include <INA229Driver.h>` |
| PlatformIO | `espressif32@6.10.0`, Arduino framework, and C++17 project flags |
| Another C++17 platform | Portable core available; implement `INA229::ISPI` and validate your board |
| Classic Arduino Uno / Mega (AVR) | Unsupported by their default toolchains: this library needs `std::optional` and `std::string_view` |

## Installation

Until registry registration is complete, install from the Git repository or a
ZIP. Package metadata is included for Arduino Library Manager and the PlatformIO
Registry; publication and indexing are separate release steps.

### Arduino IDE

1. Install Espressif's **ESP32 board package 3.3.12** in Boards Manager.
2. Download this repository as a ZIP, then select
   **Sketch → Include Library → Add .ZIP Library**.
3. Select **ESP32 Dev Module** and open
   **File → Examples → INA229Driver → BasicReadings**.
4. Set your actual shunt resistance and maximum expected current, upload, and
   open Serial Monitor at **115200 baud**.

ESP32 core 3.3.12 fixes the deprecated touch-driver usage inside core 3.3.0.
The examples build with warnings enabled; no deprecation suppression is needed.

### PlatformIO

Add the library and C++17 flags to your project's `platformio.ini`:

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

The project flags matter because your sketch includes C++17 public headers.
Copy the quick-start sketch below into `src/main.cpp`, or copy an example's
`.ino` file into `src/`.

Working from a local checkout? The example environments use that checkout
directly, with no push or registry account required:

```sh
pio run -d examples -e BasicReadings
pio run -d examples -e BasicReadings -t upload
pio device monitor -d examples -e BasicReadings
```

## Quick start

### 1. Connect the sensor

These pins match `SPI.begin()` on **ESP32 Dev Module**:

| INA229 | ESP32 |
| --- | --- |
| VS / logic supply | 3.3 V |
| GND | GND, shared with the measured system |
| SCLK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |
| CS | GPIO 5 |

Connect **IN+ and IN− across your shunt**, and **VBUS to the monitored bus**,
following your module's wiring instructions. VBUS is a measurement input, not
the ESP32 supply. See the [wiring and shunt guide](examples/README.md#wire-the-spi-bus)
for details and the [TI datasheet](https://www.ti.com/lit/ds/symlink/ina229.pdf)
for electrical limits.

### 2. Read voltage and current

This sketch assumes a **2 mΩ shunt** and **10 A maximum expected current**.
Replace those values with your actual circuit values, including peak current.

```cpp
#include <INA229Driver.h>

INA229::ArduinoSPI sensorSPI{SPI, 5}; // SPI bus, chip-select pin
INA229::INA229_Driver sensor{sensorSPI};
bool ready = false;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  sensorSPI.begin();

  INA229::DriverConfig config;
  config.shuntResistor = 0.002;     // Ohms: 2 milliohms
  config.maxExpectedCurrent = 10; // Amperes, including peaks

  const auto status = sensor.begin(config);
  ready = status == INA229::Status::OK;
  if (!ready) {
    Serial.print("Initialization failed: ");
    Serial.println(INA229::statusToString(status).data());
  }
}

void loop() {
  delay(1000); // Allow default conversions to complete between readings.
  if (!ready) {
    return; // Correct the problem and reset the board to retry.
  }

  const auto voltage = sensor.getBusVoltage();
  const auto current = sensor.getCurrentAmps();
  if (!voltage || !current) {
    Serial.println("Reading unavailable. Check device power and SPI wiring.");
    return;
  }

  Serial.print("Bus voltage (V): ");
  Serial.println(*voltage, 4);
  Serial.print("Current (A): ");
  Serial.println(*current, 6);
}
```

With a 12 V rail and positive 1 A load, the output should be approximately:

```text
Bus voltage (V): 12.0000
Current (A): 1.000000
```

The actual result depends on the shunt, wiring, calibration, and temperature.
For descriptive configuration errors, use `config.validate(&error)` as shown in
[BasicReadings](examples/BasicReadings/BasicReadings.ino).

## Examples

Each sketch is self-contained and checks errors before using measurements.
The [examples guide](examples/README.md) covers wiring, expected output,
configuration choices, and running each sketch with either build system.

| Example | Start here when you want to… |
| --- | --- |
| [BasicReadings](examples/BasicReadings/BasicReadings.ino) | Get your first voltage and signed-current readings |
| [AllMeasurements](examples/AllMeasurements/AllMeasurements.ino) | Read every measurement and see its units |
| [EnergyAndCharge](examples/EnergyAndCharge/EnergyAndCharge.ino) | Track Wh and signed Ah, inspect overflow flags, and reset totals |
| [PrecisionAndTemperature](examples/PrecisionAndTemperature/PrecisionAndTemperature.ino) | Choose a narrower shunt range, averaging, and temperature compensation |
| [DiagnosticsAndRecovery](examples/DiagnosticsAndRecovery/DiagnosticsAndRecovery.ino) | Inspect device IDs and flags, force a health check, and handle recovery |

For example, run `pio run -d examples -e EnergyAndCharge` to build the energy
example. Send lowercase **`r`** in Serial Monitor to clear its energy and charge
accumulators.

## Measurement API

All measurement getters return `std::optional<double>`. Check the optional before
using `*value`; an empty optional signals a failed health check.

| Method | Result |
| --- | --- |
| `getBusVoltage()` | Bus voltage in V |
| `getShuntVoltageVolts()` | Signed shunt voltage in V |
| `getCurrentAmps()` | Signed current in A |
| `getDieTemperature()` | INA229 die temperature in °C |
| `getPowerWatts()` | Power in W |
| `getEnergyJoules()` | Accumulated energy in J; divide by 3600 for Wh |
| `getChargeCoulombs()` | Signed accumulated charge in C; divide by 3600 for Ah |

`getDiagnosticFlags()`, `getManufacturerId()`, and `getDeviceId()` return
`std::optional<uint16_t>`. `getLastStatus()` and `statusToString()` explain status
codes. `isHealthy()` forces a configuration check; `resetAccumulators()` clears
energy and charge. See the [public interface](src/INA229/INA229.hpp) for the full API.

### Configure for your circuit

`DriverConfig` groups the settings used by `begin()`:

- **Shunt and current:** resistance in ohms and maximum expected current in A.
  Validation checks the resulting shunt voltage and calibration register range.
- **Shunt range:** ±163.84 mV by default, or ±40.96 mV for a smaller input range.
- **Conversion and averaging:** bus, shunt, and temperature conversion times,
  sample count, and initial conversion delay.
- **Temperature compensation:** an optional unsigned coefficient in ppm/°C,
  encoded at 1 ppm/°C per register step. Use your shunt's specified coefficient
  and consider thermal coupling to the INA229 die.

The [precision example](examples/PrecisionAndTemperature/PrecisionAndTemperature.ino)
shows these settings together. Averaging reduces noise; it does not correct an
incorrect shunt value or poor sense wiring.

### Know what recovery guarantees

`begin()` resets the device and verifies CONFIG, ADC_CONFIG, SHUNT_CAL, and
SHUNT_TEMPCO readback. A failed initialization disables measurements until a
subsequent successful `begin()`.

Measurement health is cached for up to seven accesses. `isHealthy()` checks
immediately and can restore mismatched configuration. Recovery does **not** restore
lost energy/charge totals or wait for a fresh conversion. `begin()` and hardware
reset also erase totals. Reading DIAG_ALRT can clear latched flags; interpret a
saved register value when checking several flags.

The SPI interface has no per-transfer error return, so cached health is not a
guarantee that every individual read succeeded. The Arduino adapter uses mode 1,
MSB first, and a default 1 MHz clock. Serialize access to a shared bus and keep
the SPI bus alive longer than the driver and adapter.

## Tested and validated

**117 native checks pass**, including regression coverage, strict C++17
compatibility, and Arduino SPI transaction behavior. The same checks also pass
with undefined-behavior and float-to-integer-overflow sanitizers.

| Validation | Configuration | Result |
| --- | --- | --- |
| All five Arduino examples | Arduino CLI 1.3.1, ESP32 core 3.3.12, ESP32 Dev Module | Compiled with all warnings enabled; no warnings |
| All five PlatformIO examples | PlatformIO 6.1.18, espressif32 6.10.0, Arduino core 2.0.17 | Compiled against the packaged library |
| Arduino library and sketch lint | Strict Library Manager submission checks | No errors or warnings |
| Native tests | GCC 14, CMake, GoogleTest/GoogleMock | 117 checks passed, normally and with sanitizers |

CI checks native tests and compiles every example under both embedded toolchains.
**Physical-device timing and measurement accuracy have not yet been verified.**
Build and test results establish software coverage, not hardware certification.

Alert-threshold configuration and a separate single-shot retrigger API are not
currently exposed. Other boards need their own validation. The `INA229.h` include
remains available for compatibility; new sketches should use `INA229Driver.h`.

## Native development and other platforms

The core has no Arduino dependency. Implement the two methods in
[`INA229::ISPI`](src/INA229/ISPI.hpp), keeping chip select asserted across the
command and payload, and pass your transport to `INA229_Driver`. Follow the TI
SPI timing and byte-order requirements. Transport implementations must not throw
during the driver's `noexcept` health checks.

Build and test with CMake 3.14 or newer. The driver requires C++17; the tests also
exercise the C++20 span overload and require a C++20 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

CMake uses an installed GoogleTest/GoogleMock package or fetches pinned
GoogleTest 1.17.0. For offline testing, set
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest`.
Use `-DBUILD_TESTING=OFF` for a dependency-free library build. CMake consumers can
link the `INA229::Driver` target.

C++ formatting is defined in `.clang-format` (clang-format 18):

```sh
clang-format -i src/*.h src/INA229/*.hpp src/*.cpp tests/*.cpp tests/mocks/*.hpp tests/arduino/*.h examples/*.cpp examples/*/*.ino
```

## Releases and license

See the [changelog](CHANGELOG.md) for changes. Keep the versions in
`library.properties` and `library.json` aligned, run the test and packaging
workflows, and push a matching version tag when ready to release.

For PlatformIO Registry publication, the maintainer runs `pio account login` and
`pio pkg publish`. For Arduino Library Manager, submit the repository through the
[Arduino library registry](https://github.com/arduino/library-registry#adding-a-library-to-library-manager).
After acceptance, new compliant tags are indexed automatically; change the
packaging workflow's `library-manager` mode from `submit` to `update` once listed.
These steps require maintainer action; CI does not publish automatically.

Driver code is available under the [MIT license](LICENSE). The bundled TI
datasheet retains Texas Instruments' copyright and is excluded from the
PlatformIO distribution archive.
