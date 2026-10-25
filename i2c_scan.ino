// I2C SCANNER: lists every sensor plugged into the STEMMA QT chain
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);   // waits for Serial Monitor to open
  Wire.begin();
}

void loop() {
  Serial.println("Scanning...");
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
    }
  }
  Serial.println("Done. Next scan in 5 seconds.");
  delay(5000);
}
