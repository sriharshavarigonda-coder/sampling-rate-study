# sampling-rate-study
Code for a research study pertaining to sampling rates vs apogee accuracy on a model rocket

This Study concerns about apogee estimation accuracy through sampling rate. In this study, I first complete a test run using a 100 Hz sample in a custom flight computer built by me, which has flight_logger.ino loaded into it. 

i2c scan: is used to scan if the sensors are connected, in this study we use a barometer and a accelerometer. 

sensor test: Reads both sensors and prints the live values to the serial monitor, without saving anything. 

flight logger: records the flight

analyze flight: comprehends the LOG000 csv file output given by the fligh logger and gives the results of the flight. 

How to run the analysis: python analyze_flight.py LOG000.CSV (after unzipping), with Python 3 and no extra installs.

Hardware: the Feather RP2040 Adalogger, BMP388, LSM6DSO32, and in arduino IDE I used Adafruit BMP3XXX Library, Adafruit LSM6DS, SdFat - Adafruit Fork. 

AI assistance: The code that i generated was written by claude opus 5.5, but I made sure to test that it worked and ran smooth and gave accurate results. 
