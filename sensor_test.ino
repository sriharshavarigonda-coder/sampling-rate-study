// SENSOR TEST: shows live readings from both sensors in the Serial Monitor.
// Use it to check the sensors work and to find which axis points "up".
// Nothing is saved to the microSD card. This program is NOT flown.

#include <Wire.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_LSM6DSO32.h>

Adafruit_BMP3XX bmp;        // the barometer
Adafruit_LSM6DSO32 imu;     // the accelerometer + gyroscope

float groundPressure = 0;   // pressure at the start, used as "zero altitude"

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);          // wait until Serial Monitor is open

  if (!bmp.begin_I2C()) {
    Serial.println("BMP388 NOT found - check cable 1");
    while (true) delay(1000);
  }
  if (!imu.begin_I2C()) {
    Serial.println("LSM6DSO32 NOT found - check cable 2");
    while (true) delay(1000);
  }
  imu.setAccelRange(LSM6DSO32_ACCEL_RANGE_32_G);

  bmp.performReading();               // first reading is thrown away
  delay(100);
  bmp.performReading();
  groundPressure = bmp.pressure;      // remember starting pressure
  Serial.println("Both sensors found. Readings every half second:");
}

void loop() {
  bmp.performReading();
  // Height above the starting point, from the pressure change
  float heightM = 44330.0 * (1.0 - pow(bmp.pressure / groundPressure, 0.1903));

  sensors_event_t accel, gyro, temp;
  imu.getEvent(&accel, &gyro, &temp);

  Serial.print("Height (m): ");  Serial.print(heightM, 2);
  Serial.print("   Accel X: ");  Serial.print(accel.acceleration.x, 2);
  Serial.print("   Y: ");        Serial.print(accel.acceleration.y, 2);
  Serial.print("   Z: ");        Serial.print(accel.acceleration.z, 2);
  Serial.println("  (m/s^2)");
  delay(500);
}
