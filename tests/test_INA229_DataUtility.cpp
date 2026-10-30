#include <INA229/Utility.hpp>
#include <gtest/gtest.h>

using namespace INA229;

/**
 * @file DataUtilityTest.cpp
 * @brief Unit tests for INA229 low-level bit manipulation and protocol logic.
 */

// --- Command Byte Tests ---

TEST(DataUtilityTest, MakeCommandByte_Read) {
  // Register CONFIG (0x00), Read bit (1) -> (0x00 << 2) | 1 = 0x01
  EXPECT_EQ(DataUtility::makeCommandByte(Register::CONFIG, true), 0x01);
  // Register DEVICE_ID (0x3F), Read bit (1) -> (0x3F << 2) | 1 = 0xFD
  EXPECT_EQ(DataUtility::makeCommandByte(Register::DEVICE_ID, true), 0xFD);
}

TEST(DataUtilityTest, MakeCommandByte_Write) {
  // Register SHUNT_CAL (0x02), Write bit (0) -> (0x02 << 2) | 0 = 0x08
  EXPECT_EQ(DataUtility::makeCommandByte(Register::SHUNT_CAL, false), 0x08);
}

// --- Byte Packing Tests (Big Endian) ---

TEST(DataUtilityTest, PackBytes_MultiWidth) {
  uint8_t buf2[] = {0x12, 0x34};
  EXPECT_EQ(DataUtility::packBytes(buf2, 2), 0x1234ull);

  uint8_t buf3[] = {0x12, 0x34, 0x56};
  EXPECT_EQ(DataUtility::packBytes(buf3, 3), 0x123456ull);

  uint8_t buf5[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
  EXPECT_EQ(DataUtility::packBytes(buf5, 5), 0xAABBCCDDEEull);
}

// --- 20-Bit Sign Extension Tests (Left Aligned in 24-bit) ---

TEST(DataUtilityTest, SignExtend20Bit_Positive) {
  // 0x000010 >> 4 = 1
  EXPECT_EQ(DataUtility::signExtend20Bit(0x000010), 1);
  // 0x7FFFF0 >> 4 = 0x7FFFF (Max Positive 20-bit: 524287)
  EXPECT_EQ(DataUtility::signExtend20Bit(0x7FFFF0), 524287);
}

TEST(DataUtilityTest, SignExtend20Bit_Negative) {
  // 0xFFFFF0 >> 4 = 0xFFFFF (-1 in 20-bit two's complement)
  EXPECT_EQ(DataUtility::signExtend20Bit(0xFFFFF0), -1);
  // 0x800000 >> 4 = 0x80000 (Min Negative 20-bit: -524288)
  EXPECT_EQ(DataUtility::signExtend20Bit(0x800000), -524288);
}

TEST(DataUtilityTest, SignExtend20Bit_Zero) {
  EXPECT_EQ(DataUtility::signExtend20Bit(0x000000), 0);
}

// --- 40-Bit Sign Extension Tests (Energy/Charge) ---

TEST(DataUtilityTest, SignExtend40Bit_Positive) {
  // Mid-range positive value
  uint64_t val = 0x0000001234567890ull;
  EXPECT_EQ(DataUtility::signExtend40Bit(val), 0x1234567890ll);

  // Max positive 40-bit: (2^39 - 1)
  uint64_t maxPos = 0x0000007FFFFFFFFFull;
  EXPECT_EQ(DataUtility::signExtend40Bit(maxPos), 549755813887ll);
}

TEST(DataUtilityTest, SignExtend40Bit_Negative) {
  // -1 in 40-bit
  uint64_t negOne = 0x000000FFFFFFFFFFull;
  EXPECT_EQ(DataUtility::signExtend40Bit(negOne), -1ll);

  // Min negative 40-bit: -2^39
  uint64_t minNeg = 0x0000008000000000ull;
  EXPECT_EQ(DataUtility::signExtend40Bit(minNeg), -549755813888ll);
}

TEST(DataUtilityTest, SignExtend20Bit_IgnoresReservedBits) {
  // 0x00001F: The 'F' is in the reserved bit range [3:0]
  // Result should still be 1 (0x00001)
  EXPECT_EQ(DataUtility::signExtend20Bit(0x00001F), 1);
}

TEST(DataUtilityTest, SignExtend40Bit_HandlesDirtyUpperBits) {
  // Input has '0xAA' in the upper bits which shouldn't be there
  uint64_t dirtyInput = 0xAA00000012345678ull;
  // If your code doesn't mask the 0xAA, this might fail or return garbage
  // Note: You may need to update your utility to: val &= 0xFFFFFFFFFFull;
  EXPECT_EQ(DataUtility::signExtend40Bit(dirtyInput), 0x12345678ll);
}

TEST(DataUtilityTest, PackBytes_ZeroLength) {
  uint8_t buf[] = {0x12};
  EXPECT_EQ(DataUtility::packBytes(buf, 0), 0);
}

TEST(DataUtilityTest, SignExtend40Bit_Zero) {
  EXPECT_EQ(DataUtility::signExtend40Bit(0x0000000000000000ull), 0ll);
}

TEST(DataUtilityTest, PackBytes_SingleByte) {
  uint8_t buf[] = {0xAB};
  EXPECT_EQ(DataUtility::packBytes(buf, 1), 0xABull);
}