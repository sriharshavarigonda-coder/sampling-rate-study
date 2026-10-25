// ROCKET FLIGHT LOGGER  (version 2)
// Board:   Adafruit Feather RP2040 Adalogger
// Sensors: BMP388 barometer + LSM6DSO32 accelerometer/gyroscope
// Records every sensor 100 times per second into a new CSV file on the
// microSD card (LOG000.CSV, LOG001.CSV, ...). One new file per power-on.

#include <Wire.h>
#include <SPI.h>
#include "SdFat.h"
#include <Adafruit_BMP3XX.h>
#include <Adafruit_LSM6DSO32.h>

// ---------- SETTINGS ----------
#define SD_CS_PIN 23                        // microSD pin on the Adalogger
const uint32_t SAMPLE_INTERVAL_US = 10000;  // 10,000 microseconds = 100 Hz
const float SEA_LEVEL_HPA = 1013.25;        // only for the rough altitude column

// ---------- OBJECTS ----------
SdFat SD;
FsFile logFile;
SdSpiConfig config(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(16), &SPI1);
Adafruit_BMP3XX bmp;
Adafruit_LSM6DSO32 imu;

// ---------- VARIABLES ----------
uint32_t nextSampleUs = 0;   // when the next sample is due
uint32_t sampleCount = 0;    // samples written so far
uint32_t lateSamples = 0;    // samples that were more than 10 ms late
uint64_t startUs = 0;        // time logging started

// micros() rolls over to 0 after about 71 minutes. This keeps counting
// past that point so the time column never jumps backwards.
uint64_t microsLong() {
  static uint32_t last = 0;
  static uint64_t high = 0;
  uint32_t now = micros();
  if (now < last) high += 0x100000000ULL;
  last = now;
  return high + now;
}

// If something is wrong: blink the red LED fast forever and print why.
void stopWithError(const char *msg) {
  while (true) {
    Serial.println(msg);
    digitalWrite(LED_BUILTIN, HIGH); delay(100);
    digitalWrite(LED_BUILTIN, LOW);  delay(100);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay(2000);  // time to open Serial Monitor (the logger never waits for a computer)

  if (!SD.begin(config))  stopWithError("ERROR: microSD card not found. Inserted? FAT32?");
  if (!bmp.begin_I2C())   stopWithError("ERROR: BMP388 not found. Check cable 1.");
  if (!imu.begin_I2C())   stopWithError("ERROR: LSM6DSO32 not found. Check cable 2.");
  Wire.setClock(400000);  // fast I2C so both sensors fit inside 10 ms

  // Barometer: raw, unfiltered readings (filtering would hide the effect we study)
  bmp.setTemperatureOversampling(BMP3_NO_OVERSAMPLING);
  bmp.setPressureOversampling(BMP3_NO_OVERSAMPLING);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_DISABLE);
  bmp.setOutputDataRate(BMP3_ODR_200_HZ);
  bmp.performReading();   // warm-up: the very first reading after power-on is junk
  delay(20);

  // Accelerometer: +/-32 g so motor burn never maxes it out
  imu.setAccelRange(LSM6DSO32_ACCEL_RANGE_32_G);
  imu.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  imu.setAccelDataRate(LSM6DS_RATE_416_HZ);
  imu.setGyroDataRate(LSM6DS_RATE_416_HZ);

  // Pick the next unused file name: LOG000.CSV, LOG001.CSV, ...
  char name[13];
  for (int i = 0; i < 1000; i++) {
    snprintf(name, sizeof(name), "LOG%03d.CSV", i);
    if (!SD.exists(name)) break;
  }
  logFile = SD.open(name, FILE_WRITE);
  if (!logFile) stopWithError("ERROR: could not create log file.");
  logFile.println("t_s,pressure_Pa,temp_C,baro_alt_m,ax_mps2,ay_mps2,az_mps2,gx_rads,gy_rads,gz_rads");
  logFile.flush();

  Serial.print("Logging at 100 Hz to ");
  Serial.println(name);
  startUs = microsLong();
  nextSampleUs = micros();
}

void loop() {
  uint64_t nowLong = microsLong();                       // keep the long clock ticking
  if ((int32_t)(micros() - nextSampleUs) < 0) return;    // not time for a sample yet

  uint32_t t = micros();
  nextSampleUs += SAMPLE_INTERVAL_US;
  if ((int32_t)(t - nextSampleUs) > 0) {   // fell more than one sample behind
    lateSamples++;
    nextSampleUs = t + SAMPLE_INTERVAL_US;
  }

  // ---- read the sensors ----
  sensors_event_t accel, gyro, temp;
  imu.getEvent(&accel, &gyro, &temp);
  // Note: the barometer library returns the result of the PREVIOUS reading,
  // so pressure in each row is about 10 ms older than the row's time.
  // The analysis script corrects for this.
  bmp.performReading();
  float altM = 44330.0 * (1.0 - pow((bmp.pressure / 100.0) / SEA_LEVEL_HPA, 0.1903));
  double tSec = (double)(nowLong - startUs) / 1000000.0;

  // ---- write one row to the CSV file ----
  logFile.print(tSec, 6);                 logFile.print(',');
  logFile.print(bmp.pressure, 1);         logFile.print(',');
  logFile.print(bmp.temperature, 2);      logFile.print(',');
  logFile.print(altM, 2);                 logFile.print(',');
  logFile.print(accel.acceleration.x, 3); logFile.print(',');
  logFile.print(accel.acceleration.y, 3); logFile.print(',');
  logFile.print(accel.acceleration.z, 3); logFile.print(',');
  logFile.print(gyro.gyro.x, 4);          logFile.print(',');
  logFile.print(gyro.gyro.y, 4);          logFile.print(',');
  logFile.println(gyro.gyro.z, 4);

  // ---- once per second: save to card, blink LED, print status ----
  sampleCount++;
  if (sampleCount % 100 == 0) {
    logFile.flush();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // slow blink = logging OK
    if (Serial) {
      Serial.print("Samples: ");   Serial.print(sampleCount);
      Serial.print("  Late: ");    Serial.print(lateSamples);
      Serial.print("  Alt (m): "); Serial.print(altM, 2);
      Serial.print("  Accel X/Y/Z: ");
      Serial.print(accel.acceleration.x, 1); Serial.print(" / ");
      Serial.print(accel.acceleration.y, 1); Serial.print(" / ");
      Serial.println(accel.acceleration.z, 1);
    }
  }
}
