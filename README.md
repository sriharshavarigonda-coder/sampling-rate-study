# sampling-rate-study

Code and data for a research study on how sensor sampling rate affects apogee estimation accuracy on a model rocket.

**Paper:** *Effect of Sensor Sampling Frequency (10–100 Hz) on Apogee Estimation Accuracy in a Model Rocket Flight*, engrXiv. DOI: [add when assigned]

## Overview

I built a custom flight computer that logged a barometer and an accelerometer at 100 Hz during one flight of an Estes Pro Series II Lil' Spite on an AeroTech G74W-4 motor. The analysis script downsamples the 100 Hz data to 50 Hz and 10 Hz, calculates the apogee at each rate, and compares the results with a Jolly Logic AltimeterTwo that flew in the same payload bay as a reference.

## Files

| File | Purpose |
|---|---|
| `flight_logger.ino` | Flight firmware: logs both sensors at 100 Hz to the microSD card |
| `sensor_test.ino` | Bench test: prints live sensor values to the Serial Monitor without saving anything |
| `i2c_scan.ino` | Checks that both sensors are detected on the I²C bus |
| `analyze_flight.py` | Downsamples the log to 50 Hz and 10 Hz and calculates apogee at each rate |
| `LOG000.CSV` | Raw 100 Hz flight log from the microSD card |
| `Flight 1.xlsx` | Spreadsheet used for the graphs and analysis in the paper |

## Data columns (`LOG000.CSV`)

| Column | Meaning | Unit |
|---|---|---|
| `t_s` | Time since the logger started | s |
| `pressure_Pa` | Barometric pressure (BMP388) | Pa |
| `temp_C` | Barometer temperature | °C |
| `baro_alt_m` | Rough altitude using standard sea-level pressure (1013.25 hPa) | m |
| `ax_mps2`, `ay_mps2`, `az_mps2` | Acceleration on each axis (LSM6DSO32) | m/s² |
| `gx_rads`, `gy_rads`, `gz_rads` | Angular rate on each axis (LSM6DSO32) | rad/s |

Note: the barometer library returns the result of the previous reading, so each pressure value is about 10 ms older than its timestamp. `analyze_flight.py` corrects for this.

## Sensor settings

- **Accelerometer (LSM6DSO32):** ±32 g range, 416 Hz internal data rate, logged at 100 Hz
- **Gyroscope (LSM6DSO32):** ±2000 °/s range, 416 Hz internal data rate
- **Barometer (BMP388):** no pressure or temperature oversampling, IIR filter disabled (raw data)
- **Logging rate:** 100 Hz (10 ms interval)

## Hardware

- Adafruit Feather RP2040 Adalogger (product 5980)
- Adafruit BMP388 barometric pressure sensor (product 3966)
- Adafruit LSM6DSO32 accelerometer/gyroscope (product 4692)
- 3.7 V 400 mAh LiPo battery (product 3898)
- 2 × STEMMA QT / Qwiic cables, 100 mm (product 4210)
- 32 GB microSD card

## Software

Arduino IDE with the **Raspberry Pi Pico/RP2040** board package (Earle Philhower), plus these libraries:

- Adafruit BMP3XX Library
- Adafruit LSM6DS
- SdFat – Adafruit Fork

## Running the analysis

Requires Python 3 (no extra packages):

```
python analyze_flight.py LOG000.CSV
```

## AI assistance

The firmware and analysis code were written with the help of Claude (Anthropic). I specified what the code needed to do, tested it on the hardware and on the flight data, and verified the results.

## License

MIT License. See `LICENSE`.

## How to cite

Varigonda, S. H. (2026). *Sampling-rate study* (Version 1.1) [Computer software]. GitHub. https://github.com/sriharshavarigonda-coder/sampling-rate-study
