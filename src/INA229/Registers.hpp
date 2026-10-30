#pragma once

#include <cstdint>

namespace INA229 {

/**
 * @brief Register Addresses per Datasheet Table 7-3.
 */
enum class Register : uint8_t {
  CONFIG = 0x00,          // Configuration (16-bit)
  ADC_CONFIG = 0x01,      // ADC Configuration (16-bit)
  SHUNT_CAL = 0x02,       // Shunt Calibration (16-bit)
  SHUNT_TEMPCO = 0x03,    // Shunt Temperature Coefficient (16-bit)
  VSHUNT = 0x04,          // Shunt Voltage Measurement (24-bit)
  VBUS = 0x05,            // Bus Voltage Measurement (24-bit)
  DIETEMP = 0x06,         // Temperature Measurement (16-bit)
  CURRENT = 0x07,         // Current Measurement (24-bit)
  POWER = 0x08,           // Power Measurement (24-bit)
  ENERGY = 0x09,          // Energy Measurement (40-bit)
  CHARGE = 0x0A,          // Charge Measurement (40-bit)
  DIAG_ALRT = 0x0B,       // Diagnostic Flags and Alert (16-bit)
  SOVL = 0x0C,            // Shunt Overvoltage Threshold (16-bit)
  SUVL = 0x0D,            // Shunt Undervoltage Threshold (16-bit)
  BOVL = 0x0E,            // Bus Overvoltage Threshold (16-bit)
  BUVL = 0x0F,            // Bus Undervoltage Threshold (16-bit)
  TEMP_LIMIT = 0x10,      // Temperature Over-Limit Threshold (16-bit)
  PWR_LIMIT = 0x11,       // Power Over-Limit Threshold (16-bit)
  MANUFACTURER_ID = 0x3E, // Manufacturer ID (16-bit)
  DEVICE_ID = 0x3F        // Device ID (16-bit)
};

namespace Mask {

// --- CONFIG (0x00) 16-bit ---
namespace Config {
inline constexpr uint16_t RST = 0x8000u;          // Bit 15: System Reset
inline constexpr uint16_t RSTACC = 0x4000u;       // Bit 14: Accumulator Reset
inline constexpr uint16_t CONVDLY_MASK = 0x3FC0u; // Bits 13-6: Conversion Delay
inline constexpr uint8_t CONVDLY_SHIFT = 6;
inline constexpr uint16_t TEMPCOMP = 0x0020u; // Bit 5: Shunt Temp Compensation
inline constexpr uint16_t ADCRANGE = 0x0010u; // Bit 4: ADC Range Select
inline constexpr uint16_t RESERVED = 0x000Fu; // Bits 3-0
inline constexpr uint16_t CONFIG_VALID_MASK =
    RST | RSTACC | CONVDLY_MASK | TEMPCOMP | ADCRANGE;
inline constexpr uint16_t RESET_VALUE = 0x0000u;
} // namespace Config

// --- ADC_CONFIG (0x01) 16-bit ---
namespace AdcConfig {
inline constexpr uint16_t MODE_MASK = 0xF000u;   // Bits 15-12
inline constexpr uint16_t VBUSCT_MASK = 0x0E00u; // Bits 11-9
inline constexpr uint16_t VSHCT_MASK = 0x01C0u;  // Bits 8-6
inline constexpr uint16_t VTCT_MASK = 0x0038u;   // Bits 5-3
inline constexpr uint16_t AVG_MASK = 0x0007u;    // Bits 2-0

inline constexpr uint8_t MODE_SHIFT = 12;
inline constexpr uint8_t VBUSCT_SHIFT = 9;
inline constexpr uint8_t VSHCT_SHIFT = 6;
inline constexpr uint8_t VTCT_SHIFT = 3;
inline constexpr uint8_t AVG_SHIFT = 0;

inline constexpr uint16_t RESET_VALUE =
    0xFB68u; // Factory default: Continuous All, 1052us, 1x AVG
} // namespace AdcConfig

// --- Shunt Temperature Coefficient (SHUNT_TEMPCO) (0x03) ---
namespace ShuntTempco {
inline constexpr uint16_t DATA_MASK = 0x3FFFu; // Bits 13-0
inline constexpr uint16_t RESERVED = 0xC000u;  // Bits 15-14
} // namespace ShuntTempco

// --- Shunt Voltage Measurement (VSHUNT) (0x04) [24-bit read] ---
namespace VShunt {
inline constexpr uint32_t RAW_MASK = 0x00FF'FFF0u; // Bits 23-4 in 24-bit word
inline constexpr uint32_t DATA_MASK =
    0x000F'FFFFu; // 20-bit raw value after shift
inline constexpr uint8_t SHIFT = 4;
inline constexpr double LSB_R0 = 312.5e-9;  // ADCRANGE = 0 (312.5 nV/LSB)
inline constexpr double LSB_R1 = 78.125e-9; // ADCRANGE = 1 (78.125 nV/LSB)
} // namespace VShunt

// --- Bus Voltage Measurement (VBUS) (0x05) [24-bit read] ---
namespace VBus {
inline constexpr uint32_t RAW_MASK = 0x00FF'FFF0u; // Bits 23-4 in 24-bit word
inline constexpr uint32_t DATA_MASK =
    0x000F'FFFFu; // 20-bit raw value after shift
inline constexpr uint8_t SHIFT = 4;
inline constexpr double LSB = 195.3125e-6; // 195.3125 µV/LSB
} // namespace VBus

// --- Temperature Measurement (DIETEMP) (0x06) ---
namespace DieTemp {
inline constexpr uint16_t DATA_MASK = 0xFFFFu;
inline constexpr double LSB_CELSIUS = 0.0078125; // 7.8125 m°C/LSB
} // namespace DieTemp

// --- Current Result (CURRENT) (0x07) [24-bit read] ---
namespace Current {
inline constexpr uint32_t RAW_MASK = 0x00FF'FFF0u; // Bits 23-4 in 24-bit word
inline constexpr uint32_t DATA_MASK =
    0x000F'FFFFu; // 20-bit raw value after shift
inline constexpr uint8_t SHIFT = 4;
} // namespace Current

// --- Power Result (POWER) (0x08) [24-bit read] ---
namespace Power {
inline constexpr uint32_t DATA_MASK = 0x00FF'FFFFu; // 24-bit unsigned
inline constexpr double LSB_FACTOR = 3.2;           // 3.2 * Current_LSB
} // namespace Power

// --- Energy Result (ENERGY) (0x09) [40-bit read] ---
namespace Energy {
inline constexpr uint64_t DATA_MASK =
    0x0000'00FF'FFFF'FFFFull;             // 40-bit unsigned
inline constexpr uint8_t LSB_FACTOR = 16; // 16 * Power_LSB
} // namespace Energy

// --- Charge Result (CHARGE) (0x0A) [40-bit read] ---
namespace Charge {
inline constexpr uint64_t DATA_MASK =
    0x0000'00FF'FFFF'FFFFull; // 40-bit two's complement
} // namespace Charge

// --- Diagnostic Flags and Alert (DIAG_ALRT) (0x0B) ---
namespace DiagAlert {
inline constexpr uint16_t ALATCH = 0x8000u; // Bit 15: Alert Latch Enable
inline constexpr uint16_t CNVR =
    0x4000u; // Bit 14: Conversion Ready Flag on ALRT Pin
inline constexpr uint16_t SLOWALERT =
    0x2000u; // Bit 13: Alert Comparison on Averaged Output
inline constexpr uint16_t APOL =
    0x1000u; // Bit 12: Alert Polarity (1 = Active High)
inline constexpr uint16_t ENERGYOF = 0x0800u; // Bit 11: Energy Overflow
inline constexpr uint16_t CHARGEOF = 0x0400u; // Bit 10: Charge Overflow
inline constexpr uint16_t MATHOF = 0x0200u;   // Bit 9: Math Overflow
inline constexpr uint16_t TMPOL = 0x0080u;    // Bit 7: Temperature Over-Limit
inline constexpr uint16_t SHNTOL = 0x0040u;   // Bit 6: Shunt Over-Voltage Limit
inline constexpr uint16_t SHNTUL = 0x0020u; // Bit 5: Shunt Under-Voltage Limit
inline constexpr uint16_t BUSOL = 0x0010u;  // Bit 4: Bus Over-Voltage Limit
inline constexpr uint16_t BUSUL = 0x0008u;  // Bit 3: Bus Under-Voltage Limit
inline constexpr uint16_t POL = 0x0004u;    // Bit 2: Power Over-Limit
inline constexpr uint16_t CNVRF = 0x0002u;  // Bit 1: Conversion Ready Flag
inline constexpr uint16_t MEMSTAT =
    0x0001u; // Bit 0: Memory Status (1 = Good, 0 = Checksum Error)
inline constexpr uint16_t RESET_VALUE = 0x0001u;
} // namespace DiagAlert

/** @brief Shunt Voltage Limits (SOVL, SUVL) - 16-bit Two's Complement */
namespace ShuntLimit {
inline constexpr uint16_t DATA_MASK = 0xFFFFu;
inline constexpr double LSB_R0 = 5.0e-6;  // ADCRANGE = 0 (5 µV/LSB)
inline constexpr double LSB_R1 = 1.25e-6; // ADCRANGE = 1 (1.25 µV/LSB)
} // namespace ShuntLimit

/** @brief Bus Voltage Limits (BOVL, BUVL) - 15-bit Unsigned */
namespace BusLimit {
inline constexpr uint16_t DATA_MASK = 0x7FFFu; // Bits 14-0
inline constexpr uint16_t RESERVED = 0x8000u;  // Bit 15
inline constexpr double LSB = 3.125e-3;        // 3.125 mV/LSB
} // namespace BusLimit

/** @brief Temperature Limit (TEMP_LIMIT) - 16-bit Two's Complement */
namespace TempLimit {
inline constexpr uint16_t DATA_MASK = 0xFFFFu;
inline constexpr double LSB_CELSIUS = 0.0078125; // 7.8125 m°C/LSB
} // namespace TempLimit

/** @brief Power Limit (PWR_LIMIT) - 16-bit Unsigned */
namespace PowerLimit {
inline constexpr uint16_t DATA_MASK = 0xFFFFu;
inline constexpr uint16_t LSB_FACTOR = 256; // 256 * Power_LSB
} // namespace PowerLimit

// --- Manufacturer ID (0x3E) ---
namespace ManufacturerID {
inline constexpr uint16_t DATA_MASK = 0xFFFFu;
inline constexpr uint16_t EXPECTED = 0x5449u; // 'TI' in ASCII
} // namespace ManufacturerID

// --- Device ID (0x3F) ---
namespace DeviceID {
inline constexpr uint16_t DIE_ID_MASK = 0xFFF0u; // Bits 15-4 (Reads 0x229)
inline constexpr uint16_t REV_ID_MASK = 0x000Fu; // Bits 3-0 (Silicon Revision)
inline constexpr uint8_t DIE_ID_SHIFT = 4;
inline constexpr uint16_t EXPECTED_DIE = 0x2290u;
} // namespace DeviceID

namespace ShuntCal {
inline constexpr uint16_t SHUNT_CAL_MASK = 0x7FFFu; // Bits 14-0

namespace Calculation {
inline constexpr double MULTIPLIER_RANGE0 =
    13107.2e6; // ADCRANGE = 0 (163.84 mV)
inline constexpr double MULTIPLIER_RANGE1 =
    52428.8e6; // ADCRANGE = 1 (40.96 mV)
} // namespace Calculation
} // namespace ShuntCal

} // namespace Mask

/**
 * @brief Trait helper returning register byte widths for SPI transactions.
 */
[[nodiscard]] constexpr uint8_t getRegisterWidth(Register reg) noexcept {
  switch (reg) {
  case Register::VSHUNT:
  case Register::VBUS:
  case Register::CURRENT:
  case Register::POWER:
    return 3; // 24-bit
  case Register::ENERGY:
  case Register::CHARGE:
    return 5; // 40-bit
  default:
    return 2; // 16-bit
  }
}

} // namespace INA229
