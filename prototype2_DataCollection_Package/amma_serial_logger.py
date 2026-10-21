#!/usr/bin/env python3
"""AMMA Prototype 2 USB Serial CSV logger.

Run one 10-minute session per activity. Firmware must be flashed with
AMMA_Prototype_2_DataLogger.ino first. Diagnostic lines are ignored; only
firmware lines beginning with DATA, are written as measurements.
"""

import argparse
import csv
import datetime as dt
import re
import sys
import time
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Missing dependency. Install it with: python -m pip install pyserial")
    raise SystemExit(1)

ACTIVITIES = ("LYING", "SITTING", "STANDING", "WALKING", "TRANSITION")
DURATION_SECONDS = 600
BAUD_RATE = 115200

FIRMWARE_COLUMNS = [
    "device_millis", "elapsed_ms", "activity", "PL_raw", "PR_raw",
    "PL_voltage_V", "PR_voltage_V", "ML_X_m_s2", "ML_Y_m_s2",
    "ML_Z_m_s2", "ML_magnitude_m_s2", "MR_X_m_s2", "MR_Y_m_s2",
    "MR_Z_m_s2", "MR_magnitude_m_s2",
]
CSV_COLUMNS = ["host_timestamp_local"] + FIRMWARE_COLUMNS


def choose_port():
    ports = list(list_ports.comports())
    if ports:
        print("Available serial ports:")
        for p in ports:
            print(f"  {p.device}: {p.description}")
    return input("Enter ESP32-S3 serial port (for example COM5 or /dev/ttyACM0): ").strip()


def main():
    parser = argparse.ArgumentParser(description="Record one AMMA activity for 10 minutes.")
    parser.add_argument("--port", help="Serial port, e.g. COM5")
    parser.add_argument("--activity", choices=ACTIVITIES, help="Activity label")
    parser.add_argument("--out", default="amma_data", help="Output folder (default: amma_data)")
    args = parser.parse_args()

    port = args.port or choose_port()
    activity = args.activity or input("Activity (LYING/SITTING/STANDING/WALKING/TRANSITION): ").strip().upper()
    if activity not in ACTIVITIES:
        print("Invalid activity. Choose one of: " + ", ".join(ACTIVITIES))
        return 2

    output_dir = Path(args.out)
    output_dir.mkdir(parents=True, exist_ok=True)
    stamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    output_path = output_dir / f"AMMA_{activity}_{stamp}.csv"

    print("\nBefore recording:")
    print("  - Fix the sensors firmly in the same position and orientation.")
    print("  - For LYING, decide whether this is awake resting or actual sleep and note it separately.")
    print("  - Close Arduino Serial Monitor/Plotter; only one program can own the port.")
    input("Press Enter when ready to connect and start the 10-minute session...")

    try:
        with serial.Serial(port, BAUD_RATE, timeout=1, write_timeout=2) as ser:
            # Opening the ESP32 serial port may reset the board. Allow it to boot.
            time.sleep(3)
            ser.reset_input_buffer()
            ser.write(f"ACT={activity}\n".encode("ascii"))
            ser.flush()
            time.sleep(0.2)
            ser.write(b"START\n")
            ser.flush()

            start = time.monotonic()
            rows = 0
            malformed = 0
            last_report = start

            with output_path.open("w", newline="", encoding="utf-8") as csv_file:
                writer = csv.DictWriter(csv_file, fieldnames=CSV_COLUMNS)
                writer.writeheader()
                csv_file.flush()
                print(f"\nRecording {activity} for 10 minutes.")
                print(f"CSV: {output_path.resolve()}")
                print("Do not disconnect USB or move the sensors except as required by the activity.\n")

                while time.monotonic() - start < DURATION_SECONDS:
                    raw = ser.readline().decode("utf-8", errors="replace").strip()
                    if not raw.startswith("DATA,"):
                        continue
                    values = raw.split(",")
                    if len(values) != 16:
                        malformed += 1
                        continue
                    values = values[1:]  # Remove DATA marker.
                    if values[2] != activity:
                        malformed += 1
                        continue
                    record = {"host_timestamp_local": dt.datetime.now().astimezone().isoformat(timespec="milliseconds")}
                    record.update(dict(zip(FIRMWARE_COLUMNS, values)))
                    writer.writerow(record)
                    rows += 1

                    now = time.monotonic()
                    if now - last_report >= 5:
                        elapsed = int(now - start)
                        print(f"Progress: {elapsed:>3}/600 s | rows: {rows}", end="\r", flush=True)
                        last_report = now
                    if rows % 100 == 0:
                        csv_file.flush()

                ser.write(b"STOP\n")
                ser.flush()
                csv_file.flush()

            print(f"\n\nSession complete: {activity}")
            print(f"Rows saved: {rows}")
            print(f"Malformed DATA rows ignored: {malformed}")
            print(f"File: {output_path.resolve()}")
            if rows < 5000:
                print("WARNING: Fewer than 5,000 rows were captured. Check USB connection, serial port, and firmware timing before using this session for analysis.")
            else:
                print("Capture count is broadly consistent with a nominal 10 Hz, 10-minute session; inspect timestamp gaps before analysis.")
    except serial.SerialException as exc:
        print(f"Serial error: {exc}", file=sys.stderr)
        print("Check the COM port, USB cable, driver, and that Arduino Serial Monitor is closed.", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
