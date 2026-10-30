#pragma once
#include <cstddef>
#include <cstdint>

namespace INA229 {

class ISPI {
public:
  virtual ~ISPI() = default;

  /**
   * @brief Reads data from the device.
   * @param command The register address + R/W bit.
   * @param rx Pointer to the destination buffer.
   * @param len Number of bytes to read.
   */
  virtual void read(uint8_t command, uint8_t *rx, size_t len) = 0;

  /**
   * @brief Writes data to the device.
   * @param command The register address + R/W bit.
   * @param tx Pointer to the source data.
   * @param len Number of bytes to write.
   */
  virtual void write(uint8_t command, const uint8_t *tx, size_t len) = 0;
};

} // namespace INA229