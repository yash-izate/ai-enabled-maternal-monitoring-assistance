# AMMA Prototype 2 — 10-minute activity data collection

## Files
- `AMMA_Prototype_2_DataLogger.ino`: modified ESP32-S3 firmware. Keeps the original sensor mappings, OLED, sensor acquisition, and ThingSpeak uploads every 20 seconds; adds labelled USB serial data rows.
- `amma_serial_logger.py`: laptop script that records one 10-minute activity session to CSV and ignores diagnostic/ThingSpeak messages.

## 1. Before flashing
1. Open the `.ino` file in Arduino IDE.
2. Enter your own Wi-Fi SSID/password and ThingSpeak Write API key in the configuration section. Do not publish real credentials.
3. Keep the board selection, I²C wiring, and libraries the same as the working Prototype 2 setup.
4. Upload the firmware. Check startup output once in Serial Monitor, then close Serial Monitor completely.

## 2. Prepare laptop logger
Install Python 3 if needed, then run:

```bash
python -m pip install pyserial
```

Save both files in a folder. Connect the ESP32-S3 by USB. Find the serial port in Arduino IDE's Tools > Port, Device Manager on Windows, or the listed ports from the script.

## 3. Record a session
Run, for example:

```bash
python amma_serial_logger.py --port COM5 --activity LYING
```

Replace `COM5` with the correct port. Supported activity labels are `LYING`, `SITTING`, `STANDING`, `WALKING`, and `TRANSITION`. If `--port` or `--activity` is omitted, the script asks for it.

Each run records one activity for 600 seconds (10 minutes) and creates a separate CSV file in the `amma_data` folder. Repeat the command once for each activity. Do not open Arduino Serial Monitor at the same time as the logger.

## 4. Session protocol
- Keep sensor placement, belt tension, and orientation consistent across sessions.
- Record whether LYING means awake resting or actual sleep; those are different conditions. Do not compromise personal safety to collect a sleep session.
- For WALKING, use a safe, flat surface and a consistent comfortable pace.
- For TRANSITION, perform controlled sit-to-stand/stand-to-sit transitions and document the pattern in a separate experiment log.
- Note start/end time, sensor placement, posture, pace, interruptions, and any cable movement.
- Keep ThingSpeak enabled. Its diagnostic messages are ignored by the laptop logger, while the firmware continues its approximately 20-second cloud uploads.

## 5. CSV columns
`host_timestamp_local`, `device_millis`, `elapsed_ms`, `activity`, `PL_raw`, `PR_raw`, `PL_voltage_V`, `PR_voltage_V`, `ML_X_m_s2`, `ML_Y_m_s2`, `ML_Z_m_s2`, `ML_magnitude_m_s2`, `MR_X_m_s2`, `MR_Y_m_s2`, `MR_Z_m_s2`, `MR_magnitude_m_s2`.

The firmware emits samples nominally every 100 ms (10 Hz). Wi-Fi/ThingSpeak HTTP requests are blocking in the inherited firmware and can cause occasional gaps; inspect `elapsed_ms` and host timestamps rather than assuming every session contains exactly 6,000 rows.

## 6. Important limitations
This is experimental sensor characterization, not clinical fetal movement detection. Piezo raw values are not calibrated force/pressure values. MPU acceleration magnitude includes gravity and changes with orientation. Do not interpret fluctuations as fetal kicks without appropriate signal processing and validation.
