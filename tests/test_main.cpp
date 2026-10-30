#include <gtest/gtest.h>

#if defined(ARDUINO)
#include <Arduino.h>

void setup() {
  // Match the board test runner's serial speed.
  Serial.begin(115200);

  ::testing::InitGoogleTest();
}

void loop() {
  // Embedded runners parse the serial result; there is no process exit status.
  (void)RUN_ALL_TESTS();

  // sleep for 1 sec
  delay(1000);
}

#else
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
#endif
