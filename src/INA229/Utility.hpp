#pragma once

#include "Registers.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

#if __cplusplus >= 202002L && __has_include(<span>)
#include <span>
#endif

namespace INA229 {

namespace DataUtility {

/**
 * @brief Constructs the 8-bit SPI command byte.
 *
 * Format: [7:2] Register Address (6 bits), [1] Reserved (0), [0] R/W bit
 * (Read=1, Write=0)
 *
 * @param reg Target register.
 * @param isRead true for Read operation, false for Write operation.
 * @return 8-bit formatted SPI command byte.
 */
[[nodiscard]] constexpr uint8_t makeCommandByte(Register reg,
                                                bool isRead) noexcept {
  return ((static_cast<uint8_t>(reg) & 0x3FU) << 2) | (isRead ? 0x01U : 0x00U);
}

/**
 * @brief Packs raw big-endian SPI bytes into a 64-bit unsigned integer.
 *
 * @param bytes Raw SPI payload, excluding the command byte.
 * @param length Number of bytes in the payload (at most eight).
 * @return Packed 64-bit register value.
 */
[[nodiscard]] constexpr uint64_t packBytes(const uint8_t *bytes,
                                           size_t length) noexcept {
  uint64_t result = 0;
  for (size_t index = 0; index < length; ++index) {
    result = (result << 8) | bytes[index];
  }
  return result;
}

template <size_t Size>
[[nodiscard]] constexpr uint64_t
packBytes(const uint8_t (&bytes)[Size]) noexcept {
  return packBytes(bytes, Size);
}

template <size_t Size>
[[nodiscard]] constexpr uint64_t
packBytes(const std::array<uint8_t, Size> &bytes) noexcept {
  return packBytes(bytes.data(), bytes.size());
}

#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L
// Preserve the span overload for callers using C++20 or newer.
[[nodiscard]] constexpr uint64_t
packBytes(std::span<const uint8_t> bytes) noexcept {
  return packBytes(bytes.data(), bytes.size());
}
#endif

/**
 * @brief Converts a left-aligned 20-bit two's complement value (from a 24-bit
 * register read) to a signed 32-bit integer.
 *
 * Used for VSHUNT (0x04) and CURRENT (0x07) registers.
 *
 * @param raw24 Raw 24-bit value read over SPI (bits [23:4] hold 20-bit value,
 * bits [3:0] are 0).
 * @return Sign-extended 20-bit value in range [-524288, 524287].
 */
[[nodiscard]] constexpr int32_t signExtend20Bit(uint32_t raw24) noexcept {
  const uint32_t clean24 = raw24 & 0x00FF'FFFFU;
  return static_cast<int32_t>(clean24 << 8) >> 12;
}

/**
 * @brief Converts a 40-bit two's complement value to a signed 64-bit integer.
 *
 * Used for the CHARGE (0x0A) register.
 *
 * @param raw40 Raw 40-bit packed integer (upper 24 bits ignored).
 * @return Sign-extended 40-bit value in range [-549755813888, 549755813887].
 */
[[nodiscard]] constexpr int64_t signExtend40Bit(uint64_t raw40) noexcept {
  const uint64_t clean40 = raw40 & 0x0000'00FF'FFFF'FFFFULL;
  return static_cast<int64_t>(clean40 << 24) >> 24;
}

/**
 * @brief Converts a raw 16-bit value to a signed 16-bit integer.
 *
 * Used for the DIETEMP (0x06) register.
 *
 * @param raw16 Raw 16-bit register word.
 * @return Signed 16-bit integer.
 */
[[nodiscard]] constexpr int16_t toInt16(uint16_t raw16) noexcept {
  return static_cast<int16_t>(raw16);
}

} // namespace DataUtility

} // namespace INA229
