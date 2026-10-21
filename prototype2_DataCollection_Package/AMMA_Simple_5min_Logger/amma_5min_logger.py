#!/usr/bin/env python3
import argparse
import csv
import datetime as dt
import time
from pathlib import Path

try:
    import serial
except ImportError:
    raise SystemExit("Install dependency first: python -m pip install pyserial")

DURATION_SECONDS = 300
BAUD_RATE = 115200
COLUMNS = [
    "timestamp_local", "elapsed_seconds", "PL_raw", "PR_raw",
    "ML_X_m_s2", "ML_Y_m_s2", "ML_Z_m_s2",
    "MR_X_m_s2", "MR_Y_m_s2", "MR_Z_m_s2",
]

def main():
    parser = argparse.ArgumentParser(description="Record AMMA sensor values for 5 minutes.")
    parser.add_argument("--port", required=True, help="ESP32 port, e.g. COM4")
    parser.add_argument("--out", default="amma_data", help="CSV output folder")
    args = parser.parse_args()

    folder = Path(args.out)
    folder.mkdir(parents=True, exist_ok=True)
    stamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    output = folder / f"AMMA_5MIN_{stamp}.csv"

    print("Close Arduino Serial Monitor/Plotter before continuing.")
    input("Connect the ESP32, keep sensors positioned, then press Enter...")
    try:
        with serial.Serial(args.port, BAUD_RATE, timeout=1) as ser:
            time.sleep(2)  # ESP32 may reboot when serial opens
            ser.reset_input_buffer()
            start = time.monotonic()
            rows = 0
            with output.open("w", newline="", encoding="utf-8") as f:
                writer = csv.DictWriter(f, fieldnames=COLUMNS)
                writer.writeheader()
                f.flush()
                print(f"Recording for 5 minutes. CSV: {output.resolve()}")
                while time.monotonic() - start < DURATION_SECONDS:
                    line = ser.readline().decode("utf-8", errors="replace").strip()
                    if not line.startswith("DATA,"):
                        continue
                    values = line.split(",")
                    if len(values) != 9:
                        continue
                    try:
                        pl, pr = int(values[1]), int(values[2])
                        axes = [float(x) for x in values[3:9]]
                    except ValueError:
                        continue
                    elapsed = time.monotonic() - start
                    writer.writerow({
                        "timestamp_local": dt.datetime.now().astimezone().isoformat(timespec="milliseconds"),
                        "elapsed_seconds": f"{elapsed:.3f}",
                        "PL_raw": pl, "PR_raw": pr,
                        "ML_X_m_s2": axes[0], "ML_Y_m_s2": axes[1], "ML_Z_m_s2": axes[2],
                        "MR_X_m_s2": axes[3], "MR_Y_m_s2": axes[4], "MR_Z_m_s2": axes[5],
                    })
                    rows += 1
                    if rows % 50 == 0:
                        f.flush()
                    if rows % 100 == 0:
                        print(f"Elapsed {int(elapsed):>3}/300 s | rows saved: {rows}", end="\r", flush=True)
                f.flush()
        print(f"\nRecording complete. Rows saved: {rows}")
        print(f"CSV file: {output.resolve()}")
        if rows < 2000:
            print("WARNING: Fewer than 2,000 rows. Check firmware, USB connection, and serial output.")
        else:
            print("Capture finished. Inspect timestamp gaps before analysis.")
    except serial.SerialException as exc:
        raise SystemExit(f"Serial error: {exc}\nClose Serial Monitor/Plotter and verify the COM port.")

if __name__ == "__main__":
    main()
