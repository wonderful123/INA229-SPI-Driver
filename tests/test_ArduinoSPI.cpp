#include <INA229/ArduinoSPI.hpp>
#include <gtest/gtest.h>

namespace {

class ArduinoSPITest : public ::testing::Test {
protected:
  void SetUp() override { ArduinoTest::events.clear(); }
  SPIClass bus;
};

TEST_F(ArduinoSPITest, InitializesChipSelectIdleHigh) {
  INA229::ArduinoSPI adapter{bus, 5};
  EXPECT_TRUE(ArduinoTest::events.empty());
  adapter.begin();
  const std::vector<std::string> expected{"pin:5:1", "cs:5:1"};
  EXPECT_EQ(ArduinoTest::events, expected);
}

TEST_F(ArduinoSPITest, KeepsCommandAndReadPayloadInOneTransaction) {
  INA229::ArduinoSPI adapter{bus, 5};
  bus.incoming = {0, 0x54, 0x49}; // Command response is discarded.
  uint8_t received[2]{};
  adapter.read(0xf9, received, 2);
  EXPECT_EQ(received[0], 0x54);
  EXPECT_EQ(received[1], 0x49);
  const std::vector<std::string> expected{
      "begin",      "cs:5:0", "delay:1", "transfer:249", "transfer:0",
      "transfer:0", "cs:5:1", "delay:1", "end"};
  EXPECT_EQ(ArduinoTest::events, expected);
  EXPECT_EQ(bus.lastSettings.clock, 1000000u);
  EXPECT_EQ(bus.lastSettings.order, MSBFIRST);
  EXPECT_EQ(bus.lastSettings.mode, SPI_MODE1);
}

TEST_F(ArduinoSPITest, SendsWritePayloadInOrderAtRequestedClock) {
  INA229::ArduinoSPI adapter{bus, 5, 2000000};
  const uint8_t payload[]{0x12, 0x34};
  adapter.write(0x08, payload, 2);
  const std::vector<std::string> expected{
      "begin",       "cs:5:0", "delay:1", "transfer:8", "transfer:18",
      "transfer:52", "cs:5:1", "delay:1", "end"};
  EXPECT_EQ(ArduinoTest::events, expected);
  EXPECT_EQ(bus.lastSettings.clock, 2000000u);
}

} // namespace
