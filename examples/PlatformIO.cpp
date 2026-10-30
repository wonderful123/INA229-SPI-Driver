// PlatformIO preprocesses .ino files only at the source-directory root.
// Include one selected sketch here so each Arduino example stays
// self-contained.
#if defined(INA229_EXAMPLE_BASIC)
#include "BasicReadings/BasicReadings.ino"
#elif defined(INA229_EXAMPLE_ALL)
#include "AllMeasurements/AllMeasurements.ino"
#elif defined(INA229_EXAMPLE_ENERGY)
#include "EnergyAndCharge/EnergyAndCharge.ino"
#elif defined(INA229_EXAMPLE_PRECISION)
#include "PrecisionAndTemperature/PrecisionAndTemperature.ino"
#elif defined(INA229_EXAMPLE_DIAGNOSTICS)
#include "DiagnosticsAndRecovery/DiagnosticsAndRecovery.ino"
#else
#error "Select an example environment from examples/platformio.ini."
#endif
