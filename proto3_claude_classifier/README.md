# ESP32-S3 Activity Monitor

IMU (100 Hz) -> `activity_classifier.h` -> SSD1306 OLED (ACC, GYRO, activity).

## Files
- `esp32s3_activity_monitor.ino` – sampling, display (Arduino-only code)
- `activity_classifier.h` – pure C++ classifier (host-testable)
- `tests/test_classifier.cpp` – replays the recorded CSVs through the real classifier

## Setup
Install: Adafruit MPU6050, Adafruit SSD1306, Adafruit GFX. Board: ESP32S3 Dev Module.
I2C: SDA=GPIO8, SCL=GPIO9, OLED 0x3C, IMU 0x68 (assumed MPU6050; change `readImu()` for another IMU).
Mount the IMU with the same axes as the recordings (Y up when upright, Z up when lying face-up).

## Algorithm (1 s window, decision every 0.5 s, majority vote of last 5)
1. mean az > 7 m/s2 -> LYING
2. std(|acc|) >= 0.5 -> WALKING if ax peak-to-peak >= 2.3 or |gy| > |gx| (lateral sway), else SIT/STAND
3. std(|acc|) >= 0.12 -> MOVING
4. otherwise still: SITTING vs STANDING from posture tilt atan2(az, ay) >= 13 deg initially;
   after any movement, az rising >= 0.6 => SITTING, falling => STANDING (static tilt alone overlaps between sessions).
Thresholds live in `act::Config`.

## Test
    cd tests && g++ -std=c++17 -O2 -I.. test_classifier.cpp -o t && ./t <folder_with_csvs>
