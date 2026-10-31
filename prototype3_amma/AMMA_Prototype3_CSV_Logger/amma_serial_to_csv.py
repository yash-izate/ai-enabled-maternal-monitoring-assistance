"""
AMMA Prototype 3 — Windows USB-serial CSV capture

Install once:
    py -m pip install pyserial

Run:
    py amma_serial_to_csv.py --port COM5

Replace COM5 with the ESP32-S3 COM port shown in Windows Device Manager.
The script waits for the firmware's READY message, sends 's', then saves
numeric CSV rows for the five-minute recording to a timestamped CSV file.
"""

import argparse
import csv
import re
import sys
import time
from datetime import datetime
from pathlib import Path

import serial
from serial.tools import list_ports

EXPECTED_COLUMNS = 9
HEADER_PREFIX = "time_ms,acc_x_m_s2"
ROW_RE = re.compile(r"^-?\d+(?:\.\d+)?(?:,-?\d+(?:\.\d+)?){8}$")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True, help="ESP32 serial port, e.g. COM5")
    parser.add_argument("--baud", type=int, default=460800)
    parser.add_argument("--outdir", default="amma_csv", help="Output directory")
    args = parser.parse_args()

    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    outfile = outdir / f"amma_recording_{stamp}.csv"

    print("Available serial ports:")
    for port in list_ports.comports():
        print(f"  {port.device}: {port.description}")
    print(f"\nConnecting to {args.port} at {args.baud} baud...")
    print("The board may reset when the serial port opens; waiting for READY.")

    try:
        with serial.Serial(args.port, args.baud, timeout=1) as ser:
            # Opening a serial port can reset many ESP32 boards.
            deadline = time.monotonic() + 15
            ready = False

            while time.monotonic() < deadline:
                line = ser.readline().decode("utf-8", errors="replace").strip()
                if line:
                    print(f"ESP32: {line}")
                    if "READY" in line:
                        ready = True
                        break

            if not ready:
                print("ERROR: No READY message received. Check port, cable, firmware, and baud rate.")
                sys.exit(2)

            input("\nEnsure the piezos are in the intended test condition, then press Enter to record...")
            ser.reset_input_buffer()
            ser.write(b"s")
            ser.flush()

            header = None
            with outfile.open("w", newline="", encoding="utf-8") as f:
                writer = csv.writer(f)
                started = False
                row_count = 0
                start_wait = time.monotonic()
                last_status = start_wait

                while True:
                    raw = ser.readline()
                    if not raw:
                        # Continue waiting; USB serial can occasionally pause.
                        continue

                    line = raw.decode("utf-8", errors="replace").strip()
                    if not line:
                        continue

                    if line.startswith("#"):
                        print(f"ESP32: {line}")
                        if line.startswith("#END"):
                            break
                        continue

                    if line.startswith(HEADER_PREFIX):
                        header = next(csv.reader([line]))
                        if len(header) != EXPECTED_COLUMNS:
                            print(f"ERROR: Expected {EXPECTED_COLUMNS} columns, received {len(header)}.")
                            sys.exit(3)
                        writer.writerow(header)
                        f.flush()
                        started = True
                        print("Recording started.")
                        continue

                    if started and ROW_RE.match(line):
                        values = next(csv.reader([line]))
                        if len(values) == EXPECTED_COLUMNS:
                            writer.writerow(values)
                            row_count += 1

                    now = time.monotonic()
                    if now - last_status >= 5:
                        print(f"Rows saved: {row_count}")
                        last_status = now

            print(f"\nFinished. Saved {row_count} data rows to:\n{outfile.resolve()}")
            print("Check the row count and time_ms column before analyzing the recording.")

    except serial.SerialException as exc:
        print(f"Serial error: {exc}")
        print("Close Arduino Serial Monitor/Plotter and verify the COM port.")
        sys.exit(1)


if __name__ == "__main__":
    main()
