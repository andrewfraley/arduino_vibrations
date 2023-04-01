# Arduino Vibrations

This is an arduino program for an Adafruit ESP32-S2 TFT Feather, that uses an Adafruit LSM303AGR Accelerometer to measure vibrations.  Vibration is quantified via the LSM303 accelerometer data, which is used to calculate the root-mean-square (RMS) value of the acceleration.  This data is then sent to Influx DB for visualization.  I successfully used this to monitor the activity of a sewage pump by attaching the accelerometer to a pipe.
