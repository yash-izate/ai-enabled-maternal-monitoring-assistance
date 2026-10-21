# AMMA Simple 5-Minute Data Logger

This version removes Wi-Fi, ThingSpeak, and OLED. It records the requested eight sensor values:
- PL piezo raw value
- PR piezo raw value
- ML X, Y, Z acceleration (m/s²)
- MR X, Y, Z acceleration (m/s²)

The Python logger adds local timestamps and elapsed seconds and automatically saves a timestamped CSV after five minutes.

## Wiring / addresses
- ESP32-S3 I2C: SDA GPIO 8, SCL GPIO 9
- ADS1115: address 0x48; PL on A1, PR on A0
- Left MPU6050 (ML): address 0x69
- Right MPU6050 (MR): address 0x68

## Steps
1. Open `AMMA_Simple_5min_Logger.ino` in Arduino IDE.
2. Install Adafruit ADS1X15, Adafruit MPU6050, and Adafruit Unified Sensor libraries.
3. Select the ESP32-S3 board and its COM port, then upload the sketch.
4. Close Arduino Serial Monitor/Plotter.
5. Install Python dependency if needed: `python -m pip install pyserial`
6. Run from this folder: `python amma_5min_logger.py --port COM4` (replace COM4 if needed).
7. Press Enter. CSV is saved automatically in the `amma_data` folder.

Nominal sampling is 10 Hz, so five minutes should produce about 3,000 rows; actual count may be lower due to timing and serial overhead. MPU6050 axis readings include gravity. This is a prototype data logger, not a clinically validated fetal-movement detector.
