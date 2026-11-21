# INA229 SPI Driver

Read voltage and signed current from a **TI INA229** over SPI. Track signed
amp-hours for battery monitoring, or read watts, watt-hours, and die temperature
for DC power logging. The driver verifies configuration on startup and can
restore it after a detected mismatch.

[Arduino IDE](#arduino-ide) · [PlatformIO](#platformio) ·
[Wiring and first reading](#get-your-first-reading) ·
[Portable C++](#portable-c)

Includes an ESP32 Arduino adapter and a portable C++17 core. The examples build
on both [ESP32 toolchains](#compatibility-and-validation). Physical-device timing
and measurement accuracy remain unverified. The INA228 uses I²C and requires a
different driver.

## Install

### Arduino IDE

1. In **Boards Manager**, install **esp32 by Espressif Systems**. Use core
   **3.3.12** to match the tested build.
2. In **Library Manager**, search for **INA229_SPI_Driver** by **Josh Jenkins**
   and install it.
3. Select **ESP32 Dev Module** and open **File → Examples →
   INA229_SPI_Driver → BasicReadings**.

For ZIP installation, download the Arduino library ZIP from
[GitHub Releases](https://github.com/wonderful123/INA229-SPI-Driver/releases)
and select **Sketch → Include Library → Add .ZIP Library**.

### PlatformIO

Add this to your project's `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32@6.10.0
board = esp32dev
framework = arduino
monitor_speed = 115200
build_unflags = -std=gnu++11
build_flags = -std=gnu++17
lib_deps = wonderful123/INA229_SPI_Driver@1.0.0
```

The sketch also needs C++17 because it includes the driver's public headers.
Copy [BasicReadings.ino](examples/BasicReadings/BasicReadings.ino) into
`src/main.cpp` to try it. For Git-based installation, use
`https://github.com/wonderful123/INA229-SPI-Driver.git#v1.0.0` as the dependency.

## Get your first reading

### Wire the sensor

| INA229 module | ESP32 Dev Module |
| --- | --- |
| VS / module supply | 3.3 V, if supported by the module |
| GND | Common ground with the ESP32 and monitored circuit |
| SCLK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |
| CS | GPIO 5 |

This pinout is for **ESP32 Dev Module**. For other boards, configure the SPI
pins and chip select to match your wiring. Connect **IN+ and IN− across the
shunt**, preferably with Kelvin sense connections, and **VBUS to the bus
voltage being measured**, following the module's wiring instructions and
ratings. VBUS is the voltage sense input. VS powers the INA229.
See the [TI datasheet](https://www.ti.com/lit/ds/symlink/ina229.pdf).

### Configure and run BasicReadings

1. Set `config.shuntResistor` in ohms. The sketch uses **0.002 Ω (2 mΩ)**.
   A shunt marked **100 A / 50 mV** is **0.0005 Ω (0.5 mΩ)**.
2. Set `config.maxExpectedCurrent` in amps. The sketch uses **10 A**. This
   value sets the current scale. Allow for expected peaks and choose a shunt
   range that can accommodate the resulting voltage drop.
3. Upload the sketch and open the serial monitor at **115200 baud**.

Illustrative output for a 12 V bus and a 1 A load flowing from IN+ toward IN−:

```text
INA229 ready. Open Serial Monitor at 115200 baud.
Bus voltage (V): 12.0000
Current (A): 1.000000
```

Actual readings depend on the circuit and shunt. Reversing the current reverses its sign.

An **invalid configuration** message points to the shunt, current scale, or
ADC settings. For an **initialization failure**, check sensor power, ground,
CS, and SPI wiring. If voltage looks right but current has the wrong scale
or sign, check the shunt resistance and IN+/IN− orientation. The
[examples guide](examples/README.md) includes wiring and build commands.

## Examples

| Sketch | Use it to |
| --- | --- |
| [BasicReadings](examples/BasicReadings/BasicReadings.ino) | Get bus voltage and signed current |
| [AllMeasurements](examples/AllMeasurements/AllMeasurements.ino) | Read all seven measurements |
| [EnergyAndCharge](examples/EnergyAndCharge/EnergyAndCharge.ino) | Display Wh and signed Ah, inspect overflow flags, and request an accumulator reset with `r` |
| [PrecisionAndTemperature](examples/PrecisionAndTemperature/PrecisionAndTemperature.ino) | Set shunt range, conversion times, averaging, and shunt temperature compensation |
| [DiagnosticsAndRecovery](examples/DiagnosticsAndRecovery/DiagnosticsAndRecovery.ino) | Read IDs and flags and force a configuration check |

## Measurement API

Include `<INA229_SPI_Driver.h>`. The older `INA229.h` and `INA229Driver.h`
headers are available for existing sketches.

| Getter | Value returned when available |
| --- | --- |
| `getBusVoltage()` | Bus voltage in V |
| `getShuntVoltageVolts()` | Signed differential shunt voltage in V |
| `getCurrentAmps()` | Signed current in A |
| `getDieTemperature()` | INA229 die temperature in °C |
| `getPowerWatts()` | Unsigned power in W |
| `getEnergyJoules()` | Unsigned accumulated energy in J. Divide by 3600 for Wh. |
| `getChargeCoulombs()` | Signed net accumulated charge in C. Divide by 3600 for Ah. |

Every measurement getter returns `std::optional<double>`. An empty value
means the driver did not pass its health check. A populated value means the
health check allowed the read, sometimes using cached state. The SPI
interface has no per-transfer error result. Getters read individual
registers and can return data from different conversion cycles.

### Configuration and recovery

- `DriverConfig::validate(&error)` checks the settings and can return an
  explanation. The shunt range defaults to **±163.84 mV**. Select **±40.96 mV**
  for a smaller differential input range. If using temperature compensation,
  enter the shunt's coefficient in ppm/°C. Compensation uses the INA229's die
  temperature, so thermal coupling matters.
- `begin()` resets the chip, checks the TI manufacturer ID, and verifies
  readback of CONFIG, ADC_CONFIG, SHUNT_CAL, and SHUNT_TEMPCO. It does not
  check conversion readiness. Use `getDeviceId()` to inspect the
  INA229 die ID and revision separately.
- The driver can reuse a successful health check for up to seven subsequent
  accesses. `isHealthy()` forces a configuration check and can reapply
  settings after a mismatch. After initialization or possible recovery, allow
  a full conversion cycle for your settings before relying on fresh data.
  The default continuous configuration takes about **3.2 ms** per cycle.
- Read diagnostic flags before reading the accumulators when checking for
  overflow. Energy and charge can roll over, and reading either accumulator
  clears its overflow flag. Reading DIAG_ALRT can also clear latched flags.
  A device reset, including the reset in a successful `begin()`, clears
  accumulated totals. Recovery restores settings only.
  `resetAccumulators()` sends the clear command without verifying the result.
- The public API exposes diagnostic flags but provides no alert-threshold
  configuration or separate single-shot retrigger method.

See the [public header](src/INA229/INA229.hpp) for diagnostic and status methods.

## Portable C++

The core uses the C++17 standard library. Implement the two methods in
[`INA229::ISPI`](src/INA229/ISPI.hpp) and pass the adapter to
`INA229::INA229_Driver`. Keep chip select asserted across the command and
payload, use SPI mode 1 and MSB first, and follow the datasheet timing.
The adapter must outlive the driver and must not throw. Serialize access to a
shared bus. The included Arduino adapter defaults to 1 MHz.

CMake projects can add this repository as a subdirectory and link
`INA229::Driver`. Use `-DBUILD_TESTING=OFF` for a driver-only build.
Native tests require a C++20 compiler and use GoogleTest/GoogleMock.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Compatibility and validation

| Target | Build coverage |
| --- | --- |
| Arduino IDE or CLI | ESP32 Dev Module with Espressif Arduino core 3.3.12 |
| PlatformIO | `esp32dev` with `espressif32@6.10.0` (Arduino core 2.0.17) and explicit C++17 flags |
| Other C++17 targets | Bring your own [`ISPI` adapter](src/INA229/ISPI.hpp) and validate on your board |
| Classic Arduino Uno and Mega (AVR) | Unsupported with their default toolchains |

The [v1.0.0 changelog](CHANGELOG.md) records 117 passing native checks,
including C++17 compatibility and Arduino SPI transaction tests, plus
sanitizer runs. CI runs the [native tests](.github/workflows/tests.yml) and
[builds the ESP32 examples](.github/workflows/packaging.yml) on both embedded
toolchains.

[MIT License](LICENSE) · [Changelog](CHANGELOG.md) ·
[Release workflow](.github/workflows/release.yml)
