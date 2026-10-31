"""
AMMA Prototype 3 serial-to-CSV logger.

Install once:
    py -m pip install pyserial

Run from this folder:
    py amma_serial_to_csv.py
Or specify the port:
    py amma_serial_to_csv.py --port COM4

Default recording duration is 5 minutes. Press Ctrl+C to stop early.
CSV files are saved in the amma_csv subfolder beside this script.
"""
import argparse
import csv
import time
from datetime import datetime
from pathlib import Path

import serial
from serial.tools import list_ports

COLUMNS = [
    "time_ms",
    "acc_x_m_s2",
    "acc_y_m_s2",
    "acc_z_m_s2",
    "gyro_x_rad_s",
    "gyro_y_rad_s",
    "gyro_z_rad_s",
    "piezo_left_a1_counts",
    "piezo_right_a0_counts",
]

def is_numeric_row(line):
    fields = line.strip().split(",")
    if len(fields) != len(COLUMNS):
        return False
    try:
        for field in fields:
            float(field)
        return True
    except ValueError:
        return False

def main():
    parser = argparse.ArgumentParser(description="Save AMMA sensor data to CSV.")
    parser.add_argument("--port", default="COM4", help="Serial port (default: COM4)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--minutes", type=float, default=5.0, help="Recording duration in minutes (default: 5)")
    args = parser.parse_args()

    print("Available serial ports:")
    ports = list(list_ports.comports())
    for p in ports:
        print(f"  {p.device}: {p.description}")
    if not ports:
        print("  No ports detected.")

    out_dir = Path(__file__).resolve().parent / "amma_csv"
    out_dir.mkdir(parents=True, exist_ok=True)
    out_file = out_dir / f"AMMA_recording_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

    print(f"\nConnecting to {args.port} at {args.baud} baud.")
    print("Close Arduino Serial Monitor/Plotter before running.")
    print(f"CSV destination: {out_file}")

    rows = 0
    start = None

    try:
        with serial.Serial(args.port, args.baud, timeout=1) as ser:
            # The ESP32 may reboot when the serial port opens.
            time.sleep(2.0)
            ser.reset_input_buffer()

            with out_file.open("w", newline="", encoding="utf-8") as f:
                writer = csv.writer(f)
                writer.writerow(COLUMNS)
                f.flush()

                start = time.monotonic()
                deadline = start + max(1.0, args.minutes * 60.0)
                print(f"Recording for {args.minutes:g} minute(s). Press Ctrl+C to stop.")

                while time.monotonic() < deadline:
                    raw = ser.readline()
                    if not raw:
                        continue
                    line = raw.decode("utf-8", errors="ignore").strip()
                    if not is_numeric_row(line):
                        continue
                    writer.writerow(line.split(","))
                    rows += 1
                    if rows % 25 == 0:
                        f.flush()
                    if rows % 100 == 0:
                        elapsed = time.monotonic() - start
                        rate = rows / elapsed if elapsed > 0 else 0
                        print(f"Saved {rows} rows ({rate:.1f} rows/s)", end="\\r")
                f.flush()

    except KeyboardInterrupt:
        print("\nRecording stopped.")
    except serial.SerialException as exc:
        print(f"\nSerial error: {exc}")
        print("Check COM port and close Arduino Serial Monitor.")
        return

    elapsed = time.monotonic() - start if start is not None else 0.0
    print(f"\nDone. Saved {rows} rows in {elapsed:.1f} seconds.")
    print(f"File: {out_file}")
    if rows == 0:
        print("No numeric rows received. Verify that the sketch is uploaded and both sensors initialize.")

if __name__ == "__main__":
    main()
