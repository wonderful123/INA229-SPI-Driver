# Examples guide

These sketches target **ESP32 Dev Module** and run independently. Start with
BasicReadings, then open the sketch for the feature you want to use. Set the
Serial Monitor to **115200 baud**. A sensor and shunt are required for readings;
a successful compile alone does not test the hardware.

## Wire the SPI bus

| INA229 connection | ESP32 Dev Module connection |
| --- | --- |
| VS / logic supply | 3.3 V |
| GND | GND (common with the measured system) |
| SCLK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |
| CS | GPIO 5 |

These are the defaults used by `SPI.begin()` on this board. Adapt the pins for
other boards. `ArduinoSPI` handles chip select and uses mode 1, MSB first, at
1 MHz. Start the bus and call `sensorSPI.begin()` before `sensor.begin(config)`.

Connect IN+ and IN- across the shunt, preferably with Kelvin sense wiring, and
connect VBUS to the bus voltage being measured as shown in the TI datasheet.
VBUS is a measurement input, not the MCU's 3.3 V power connection. Follow the
ratings and connection instructions for your particular module.

Every sketch assumes a **0.002 ohm shunt** and **10 A maximum expected current**.
Change both settings to match your circuit, including peak current. At 10 A this
shunt drops 20 mV and dissipates 0.2 W; choose a resistor and wiring with suitable
headroom. `config.validate(&error)` explains invalid settings before any device
reset. A failed initialization leaves the sketch idle; correct the problem and
reset the board to retry.

## Pick a sketch

| Sketch | What it demonstrates | Output / interaction |
| --- | --- | --- |
| [BasicReadings](BasicReadings/BasicReadings.ino) | Initialization, optional values, voltage and current | `Bus voltage (V)` and signed `Current (A)` every second |
| [AllMeasurements](AllMeasurements/AllMeasurements.ino) | All seven measurement getters and their units | V, mV, A, die °C, W, J, and signed coulombs |
| [EnergyAndCharge](EnergyAndCharge/EnergyAndCharge.ino) | Accumulators, unit conversion, overflow checks | Wh and signed Ah; send lowercase `r` to clear both totals |
| [PrecisionAndTemperature](PrecisionAndTemperature/PrecisionAndTemperature.ino) | Narrow shunt range, averaging, conversion times, temperature compensation | Current, shunt µV, and die °C |
| [DiagnosticsAndRecovery](DiagnosticsAndRecovery/DiagnosticsAndRecovery.ino) | Forced health checks, IDs, diagnostic flags, automatic configuration restoration | Manufacturer `0x5449`, device `0x229x`, and flags in hex |

For example, with a 12 V rail and positive 1 A load, BasicReadings should show
approximately `Bus voltage (V): 12.0000` and `Current (A): 1.000000`. Actual accuracy
depends on calibration, shunt tolerance, temperature, and wiring. Reversing shunt
polarity changes the sign of current and charge.

## Run with Arduino

Install Espressif's **ESP32 core 3.3.12**. This release includes the upstream fix
for the deprecated touch driver used internally by 3.3.0. No warning suppression
or changes to the installed core are necessary. The library itself uses SPI,
not the touch peripheral.

Select **ESP32 Dev Module**, then choose a sketch from
**File > Examples > INA229Driver** and upload it. With Arduino CLI, for example:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all --library . examples/AllMeasurements
arduino-cli upload --fqbn esp32:esp32:esp32 --port YOUR_PORT examples/AllMeasurements
```

Use your actual board port in place of `YOUR_PORT`. Installation of the library
and board core is covered in the main README.

## Run with PlatformIO

From the repository root, select one of the five environments in
[platformio.ini](platformio.ini). They use the local checkout, so changes do not
need to be pushed first:

```sh
pio run -d examples -e AllMeasurements
pio run -d examples -e AllMeasurements -t upload
pio device monitor -d examples -e AllMeasurements
```

Replace `AllMeasurements` with another sketch name from the table. The pinned
`espressif32@6.10.0` platform uses the older Arduino 2.0.17 toolchain; the project explicitly enables
C++17 for both the sketch and driver. The packaging workflow separately compiles
every sketch against the exported release archive.

## Understand the results

- An empty `std::optional` means a read failed; do not display it as zero. Error
  paths check the result before dereferencing it.
- AllMeasurements readings come from sequential SPI transfers, not a synchronized
  snapshot. Use appropriate conversion/averaging settings for your sampling rate.
- PrecisionAndTemperature assumes a **50 ppm/°C** shunt coefficient for illustration.
  Replace it with the value from your shunt datasheet. Compensation uses INA229
  **die temperature**; poor thermal coupling to the shunt limits its usefulness.
- Accumulated energy is unsigned and charge is signed. Dividing J or C by 3600
  gives Wh or Ah. These values are not a battery state-of-charge estimate.
- `begin()` resets the device, including accumulators. An unexpected hardware
  reset also loses totals; automatic configuration recovery cannot restore them.
  The recovery example avoids repeatedly calling `begin()` in its loop.
- A healthy check can restore configuration but does not wait for new conversions.
  The recovery example waits 10 ms afterward for its default ~3.2 ms cycle.
- Reading DIAG_ALRT can clear latched flags. Interpret the saved value rather
  than repeatedly reading the register to test individual flags. MEMSTAT reports
  trim-memory checksum health; it is not a reliable reset-history flag.
- The examples do not set alert thresholds or retrigger single-shot conversions;
  these operations are not exposed by the current public API.
