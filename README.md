# AMMA — Prototype 2: IoT-Based Maternal Monitoring System

**Project:** AMMA — AI-Enabled Maternal Monitoring Assistance  
**Prototype:** 2 — ESP32-S3 Sensor Integration and ThingSpeak Cloud Monitoring  
**Platform:** ESP32-S3  
**Cloud Platform:** ThingSpeak  
**Communication:** Wi-Fi over HTTPS

---

## 1. Project Overview

AMMA (Maternal Wellness Suite) is a wearable maternal monitoring project designed to explore continuous sensing and remote monitoring during the third trimester of pregnancy.

The second prototype integrates multiple sensors with an ESP32-S3 microcontroller to acquire mechanical activity and motion-related signals. The collected measurements are processed locally, displayed on an OLED screen, and transmitted wirelessly to ThingSpeak for cloud-based storage and visualization.

This prototype establishes the embedded sensing and IoT communication foundation for future fetal movement analysis and AI-assisted maternal monitoring.

> **Current scope:** Sensor data acquisition, local display, Wi-Fi connectivity, cloud data logging, and remote visualization. Clinical validation and reliable fetal-kick classification are future development tasks.

---

## 2. Objectives

- Integrate two piezoelectric sensors and two MPU6050 inertial measurement units (IMUs).
- Acquire analog piezoelectric signals using an external ADS1115 ADC.
- Read acceleration data from both MPU6050 sensors.
- Calculate acceleration magnitude for movement-related analysis.
- Display sensor measurements locally using an OLED display.
- Transmit sensor data to ThingSpeak over Wi-Fi.
- Store and visualize sensor measurements in the cloud.
- Establish a foundation for future signal processing, event detection, and AI-based analysis.

---

## 3. System Architecture

```text
                 AMMA PROTOTYPE 2

       LEFT-SIDE SENSING     RIGHT-SIDE SENSING
       ┌───────────────┐     ┌───────────────┐
       │ Left Piezo    │     │ Right Piezo   │
       │ Left MPU6050  │     │ Right MPU6050 │
       └───────┬───────┘     └───────┬───────┘
               │                     │
               └──────────┬──────────┘
                          │
                     ESP32-S3
                          │
              ┌───────────┼───────────┐
              │           │           │
          Processing     OLED        Wi-Fi
                                      │
                                   HTTPS
                                      │
                                  ThingSpeak
                                      │
                             Cloud Data & Graphs
```

### Data Flow

1. Sensors acquire motion- and vibration-related signals.
2. The ADS1115 converts the analog piezoelectric signals into digital readings.
3. The ESP32-S3 reads the sensors over I²C.
4. Sensor values are processed locally.
5. Measurements are displayed on the OLED and Serial Monitor.
6. The ESP32-S3 periodically uploads the configured sensor fields to ThingSpeak.
7. ThingSpeak stores the uploaded measurements for remote visualization and analysis.

---

## 4. Hardware Components

| Component | Quantity | Purpose |
|---|---:|---|
| ESP32-S3 development board | 1 | Main controller and Wi-Fi connectivity |
| MPU6050 IMU | 2 | Three-axis acceleration and gyroscope measurements |
| Piezoelectric sensor | 2 | Mechanical vibration/activity sensing |
| ADS1115 ADC | 1 | External analog-to-digital conversion |
| I²C OLED display | 1 | Local sensor monitoring |
| Connecting wires and supporting circuitry | As required | Electrical connections and prototyping |

### Sensor Configuration

| Device | Designation | I²C Address / Channel |
|---|---|---|
| OLED display | OLED | `0x3C` |
| ADS1115 ADC | ADC | `0x48` |
| Left MPU6050 | ML | `0x69` |
| Right MPU6050 | MR | `0x68` |
| Left piezoelectric sensor | PL | ADS1115 A1 |
| Right piezoelectric sensor | PR | ADS1115 A0 |

**I²C pin configuration:**

- SDA: GPIO 8
- SCL: GPIO 9

The two MPU6050 devices use different I²C addresses, allowing them to share the same I²C bus. The OLED and ADS1115 also communicate over this bus.

**Important:** Verify the actual wiring, module addresses, voltage requirements, and firmware configuration before assembling or manufacturing the PCB.

---

## 5. Sensor Data Acquisition

### 5.1 Piezoelectric Sensors

Two piezoelectric sensors are used to capture mechanical vibration and deformation-related signals.

- `PL`: Left piezoelectric sensor.
- `PR`: Right piezoelectric sensor.

The sensors provide analog signals that are acquired through the ADS1115 external ADC.

The readings can be investigated for transient changes, peaks, and other signal characteristics that may be relevant to movement-event detection.

Raw piezo readings are sensor-dependent and should not be interpreted directly as calibrated force, pressure, or fetal kicks without appropriate signal conditioning and validation.

### 5.2 MPU6050 Sensors

Each MPU6050 provides:

- Three-axis accelerometer measurements: X, Y, and Z.
- Three-axis gyroscope measurements: angular velocity around X, Y, and Z.

The prototype uses two IMUs to collect measurements from two sensing locations.

The acceleration magnitude is calculated as:

\[
A_{\mathrm{mag}}=\sqrt{A_x^2+A_y^2+A_z^2}
\]

This provides a combined acceleration measure that can help characterize changes in movement intensity.

Acceleration magnitude includes the gravitational component unless gravity is compensated. It is not, by itself, a validated fetal movement measurement.

---

## 6. ThingSpeak Cloud Integration

ThingSpeak is used as the cloud platform for receiving, storing, and visualizing sensor measurements.

### Communication Workflow

```text
ESP32-S3
    |
    | Connect to Wi-Fi
    v
Internet Connection
    |
    | HTTPS request
    v
ThingSpeak API
    |
    v
Channel Data Storage
    |
    v
Time-Series Charts and Remote Monitoring
```

### ThingSpeak Channel

- **Channel ID:** `3465279`
- **Protocol:** HTTPS
- **Transport:** Wi-Fi
- **Upload interval:** Approximately 20 seconds in the prototype configuration.

The firmware uses the ESP32 networking libraries to establish connectivity and submit sensor values to ThingSpeak.

A successful upload is confirmed by the HTTP response and the appearance of a new entry in the ThingSpeak channel.

### Configured Channel Fields

| ThingSpeak Field | Variable | Description |
|---|---|---|
| Field 1 | `PL` | Left piezo raw reading |
| Field 2 | `PR` | Right piezo raw reading |
| Field 3 | `ML magnitude` | Left MPU6050 acceleration magnitude |
| Field 4 | `MR magnitude` | Right MPU6050 acceleration magnitude |
| Field 5 | `ML X` | Left MPU6050 X-axis acceleration |
| Field 6 | `ML Y` | Left MPU6050 Y-axis acceleration |
| Field 7 | `MR X` | Right MPU6050 X-axis acceleration |
| Field 8 | `MR Y` | Right MPU6050 Y-axis acceleration |

These field assignments describe the intended firmware mapping. Confirm the channel field labels and uploaded payload against the current firmware before relying on the dataset.

The current eight-field payload does not include every available measurement. For example, the MPU6050 Z-axis acceleration and gyroscope measurements are not included in the field mapping above.

---

## 7. Sampling and Upload Strategy

The prototype uses two separate timing concepts:

- **Sensor acquisition:** Approximately 10 Hz, or around 10 readings per second.
- **Cloud upload:** Approximately every 20 seconds.

The acquisition loop collects sensor measurements locally, while cloud transmission occurs less frequently.

This reduces unnecessary network traffic compared with uploading every sensor sample individually.

The exact sampling frequency and upload interval depend on the current firmware implementation and should be verified against the configured loop timing.

**Data limitation:** If only the latest measurement is uploaded every 20 seconds, ThingSpeak stores periodic snapshots rather than the complete 10 Hz sensor waveform. Full-resolution signal analysis requires buffering and uploading batches of timestamped samples or using a separate data-streaming/storage mechanism.

---

## 8. Software and Libraries

The firmware is developed for the ESP32-S3 using the Arduino programming environment.

### Main Software Components

- `WiFi.h` — Wi-Fi connectivity.
- `WiFiClientSecure.h` — TLS/secure client support.
- `HTTPClient.h` — HTTP/HTTPS requests.
- `Adafruit_GFX` — Graphics support for the OLED.
- `Adafruit_SSD1306` — OLED display control.
- `Adafruit_ADS1X15` — ADS1115 ADC interface.
- `Adafruit_MPU6050` — MPU6050 sensor interface.
- `Adafruit_Unified_Sensor` — Common sensor data structures.

### Firmware Responsibilities

1. Initialize the I²C bus.
2. Initialize the OLED, ADS1115, and both MPU6050 sensors.
3. Connect to the configured Wi-Fi network.
4. Acquire piezoelectric and inertial sensor readings.
5. Calculate acceleration magnitudes.
6. Display relevant measurements locally.
7. Submit the configured eight-field payload to ThingSpeak.
8. Handle communication failures and continue sensor acquisition where supported by the firmware.

---

## 9. Setup and Installation

### Prerequisites

- ESP32 board support package installed in the Arduino IDE.
- Required sensor and display libraries installed.
- USB cable and ESP32-S3 development board.
- Working Wi-Fi connection with internet access.
- ThingSpeak account and configured channel.
- ThingSpeak Write API key.

### Setup Procedure

1. Connect the sensors and OLED according to the verified hardware pin mapping.
2. Install the required Arduino libraries.
3. Open the prototype firmware in the Arduino IDE.
4. Configure the Wi-Fi credentials.
5. Configure the ThingSpeak Channel ID and Write API key.
6. Verify the selected ESP32-S3 board and port.
7. Compile and upload the firmware.
8. Open the Serial Monitor to inspect initialization and sensor readings.
9. Confirm that the OLED displays the expected measurements.
10. Open the ThingSpeak channel and verify that new entries appear.

### Configuration Example

Keep credentials separate from publicly shared source code wherever possible.

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const unsigned long CHANNEL_ID = 3465279;
const char* WRITE_API_KEY = "YOUR_THINGSPEAK_WRITE_API_KEY";
```

Adapt the variable types and names to match the actual firmware.

**Security note:** Do not commit real Wi-Fi passwords or ThingSpeak API keys to a public repository. If credentials have already been exposed, rotate them. For HTTPS, use certificate validation rather than disabling TLS verification in any production deployment.

---

## 10. Testing and Verification

The prototype should be verified in stages.

### Hardware Tests

- Confirm that the OLED responds at I²C address `0x3C`.
- Confirm that the ADS1115 responds at `0x48`.
- Confirm that the two MPU6050 sensors respond at `0x68` and `0x69`.
- Check that both piezo channels produce readings.
- Confirm that sensor values change when the sensors are moved or mechanically stimulated.

### Firmware Tests

- Verify successful sensor initialization.
- Check that sensor readings update continuously.
- Confirm that acceleration magnitudes are calculated correctly.
- Verify that OLED and Serial Monitor outputs are readable.
- Check that sensor acquisition continues during Wi-Fi or cloud communication failures, where supported.

### Cloud Tests

- Verify Wi-Fi connectivity.
- Confirm successful HTTPS requests.
- Check that the ThingSpeak channel receives new entries.
- Confirm that the eight fields contain the expected measurements.
- Compare uploaded values with local Serial Monitor readings.
- Confirm the actual upload interval and ensure the channel update-rate restrictions are respected.

---

## 11. Achievements

The second prototype establishes the following technical capabilities:

- Integration of two piezoelectric sensing channels.
- Integration of two MPU6050 IMUs on a shared I²C bus.
- External ADC acquisition using the ADS1115.
- Local sensor-data processing using the ESP32-S3.
- OLED-based local monitoring.
- Wireless data transmission over Wi-Fi.
- Cloud logging through ThingSpeak.
- Time-series visualization of eight sensor-data fields.
- A foundation for collecting experimental datasets for subsequent signal-processing and AI research.

---

## 12. Current Limitations

This prototype is an experimental engineering system, not a clinically validated medical device.

Current limitations include:

- Raw piezoelectric readings are not calibrated force or pressure measurements.
- Motion and vibration signals may contain maternal movement, environmental interference, sensor-placement effects, and noise.
- Acceleration measurements alone cannot reliably distinguish fetal movement from other sources of motion.
- Periodic cloud snapshots do not preserve the full high-frequency sensor waveform.
- The current ThingSpeak field mapping does not include all available IMU measurements.
- No validated fetal-kick classification or clinical decision algorithm is established by this prototype alone.
- No clinical safety or diagnostic performance claims should be made without appropriate validation.

---

## 13. Future Improvements

Potential improvements for subsequent prototypes include:

- Design a robust PCB with appropriate power regulation, protection, filtering, and connectors.
- Add sensor calibration and signal-conditioning stages.
- Implement timestamped buffering and higher-resolution data logging.
- Develop digital filtering and signal-feature extraction.
- Investigate movement-event detection using labelled experimental data.
- Add an application dashboard for visualization and data access.
- Explore AI-assisted activity classification with suitable ground-truth validation.
- Improve connectivity recovery, error handling, credential management, and secure TLS configuration.
- Conduct systematic bench testing and, where appropriate, supervised clinical validation.

Any future maternal or fetal monitoring features should be developed and validated with qualified clinical guidance.

---

## 14. Repository Structure

A suggested structure for organizing the firmware and documentation is:

```text
AMMA-Prototype-2/
├── README.md
├── firmware/
│   └── AMMA_Prototype_2.ino
├── hardware/
│   ├── circuit_diagram.png
│   └── pin_mapping.md
├── documentation/
│   └── thingspeak_setup.md
└── images/
    └── prototype_setup.jpg
```

Replace the example filenames with the actual files present in the repository.

---

## 15. Conclusion

AMMA Prototype 2 integrates bilateral piezoelectric sensing, dual MPU6050 motion sensing, external ADC acquisition, local OLED monitoring, and ThingSpeak cloud connectivity using an ESP32-S3.

The prototype demonstrates the core embedded and IoT pipeline required to collect, transmit, store, and visualize experimental sensor measurements. It provides a foundation for subsequent work on signal processing, movement-event detection, dataset generation, and AI-assisted maternal monitoring.

**Project status:** Functional IoT sensing and cloud-logging prototype; further algorithm development and validation are required before making fetal-monitoring or clinical claims.

---

**Project:** AMMA — AI-Enabled Maternal Monitoring Assistance  
**Prototype:** 2 — ESP32-S3 + Multi-Sensor Integration + ThingSpeak  
**Primary purpose:** Experimental maternal movement sensing and remote sensor-data monitoring
