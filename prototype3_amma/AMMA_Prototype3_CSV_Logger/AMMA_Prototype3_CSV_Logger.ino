/*
  AMMA Prototype 3 — 5-minute CSV logger
  Board: ESP32-S3
  I2C: SDA GPIO 8, SCL GPIO 9, 100 kHz
  MPU6050: 0x68
  ADS1115: 0x48
  Piezo left: ADS1115 A1
  Piezo right: ADS1115 A0

  Serial protocol:
    - Firmware prints READY on boot.
    - Send 's' to start one 5-minute recording.
    - Send 'x' to stop early.
    - CSV data is emitted at a target 100 Hz.

  IMPORTANT:
  Do not connect piezo elements directly to ADS1115 inputs unless
  the input circuit has been checked for voltage/transient protection.
*/

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

static const int I2C_SDA_PIN = 8;
static const int I2C_SCL_PIN = 9;

static const uint8_t ADS1115_ADDR = 0x48;
static const uint8_t MPU6050_ADDR = 0x68;

static const uint32_t SAMPLE_RATE_HZ = 100;
static const uint32_t SAMPLE_PERIOD_US = 1000000UL / SAMPLE_RATE_HZ;
static const uint32_t RECORD_DURATION_MS = 5UL * 60UL * 1000UL;

Adafruit_ADS1115 ads;
Adafruit_MPU6050 mpu;

bool adsReady = false;
bool mpuReady = false;
bool recording = false;

uint32_t recordingStartUs = 0;
uint32_t nextSampleUs = 0;
uint32_t rowsWritten = 0;

void printCsvHeader() {
  Serial.println(
    "time_ms,acc_x_m_s2,acc_y_m_s2,acc_z_m_s2,"
    "gyro_x_rad_s,gyro_y_rad_s,gyro_z_rad_s,"
    "piezo_left_a1_counts,piezo_right_a0_counts"
  );
}

void startRecording() {
  if (!adsReady || !mpuReady) {
    Serial.println("#ERROR,sensor_initialization_failed");
    return;
  }

  rowsWritten = 0;
  recordingStartUs = micros();
  nextSampleUs = recordingStartUs;
  recording = true;

  printCsvHeader();
}

void stopRecording(const char *reason) {
  recording = false;
  Serial.print("#END,reason=");
  Serial.print(reason);
  Serial.print(",rows=");
  Serial.print(rowsWritten);
  Serial.print(",elapsed_ms=");
  Serial.println((uint32_t)(micros() - recordingStartUs) / 1000UL);
  Serial.println("#READY,send_s_to_start_another_recording");
}

void setup() {
  Serial.begin(460800);

  // Allow USB serial host to connect; do not wait forever for Serial.
  delay(1200);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);

  adsReady = ads.begin(ADS1115_ADDR, &Wire);
  if (adsReady) {
    ads.setGain(GAIN_ONE);
    ads.setDataRate(RATE_ADS1115_860SPS);
  }

  mpuReady = mpu.begin(MPU6050_ADDR, &Wire);
  if (mpuReady) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  Serial.print("#STATUS,ADS1115=");
  Serial.print(adsReady ? "OK" : "FAIL");
  Serial.print(",MPU6050=");
  Serial.println(mpuReady ? "OK" : "FAIL");
  Serial.println("#READY,send_s_to_start_recording");
}

void loop() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if ((c == 's' || c == 'S') && !recording) {
      startRecording();
    } else if ((c == 'x' || c == 'X') && recording) {
      stopRecording("user_stop");
    }
  }

  if (!recording) {
    delay(2);
    return;
  }

  uint32_t nowUs = micros();
  if ((int32_t)(nowUs - nextSampleUs) < 0) {
    return;
  }

  // Keep a stable schedule. If an iteration overruns, start from now
  // rather than emitting a burst of catch-up samples.
  nextSampleUs += SAMPLE_PERIOD_US;
  if ((int32_t)(micros() - nextSampleUs) >= 0) {
    nextSampleUs = micros() + SAMPLE_PERIOD_US;
  }

  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  // Single-ended raw ADC counts. These are not calibrated force units.
  int16_t piezoLeft = ads.readADC_SingleEnded(1);  // A1 = left
  int16_t piezoRight = ads.readADC_SingleEnded(0); // A0 = right

  uint32_t elapsedUs = micros() - recordingStartUs;
  uint32_t elapsedMs = elapsedUs / 1000UL;

  // One CSV row per acquired sample set.
  Serial.print(elapsedMs);
  Serial.print(',');
  Serial.print(accel.acceleration.x, 4);
  Serial.print(',');
  Serial.print(accel.acceleration.y, 4);
  Serial.print(',');
  Serial.print(accel.acceleration.z, 4);
  Serial.print(',');
  Serial.print(gyro.gyro.x, 5);
  Serial.print(',');
  Serial.print(gyro.gyro.y, 5);
  Serial.print(',');
  Serial.print(gyro.gyro.z, 5);
  Serial.print(',');
  Serial.print(piezoLeft);
  Serial.print(',');
  Serial.println(piezoRight);

  rowsWritten++;

  if (elapsedMs >= RECORD_DURATION_MS) {
    stopRecording("duration_complete");
  }
}
