/*
  AMMA Prototype 3 - Continuous CSV logger
  ESP32-S3 + MPU6050 + ADS1115
  SDA GPIO 8, SCL GPIO 9
  MPU6050 address 0x68; ADS1115 address 0x48
  Left piezo -> ADS1115 A1; right piezo -> ADS1115 A0

  Numeric CSV rows are printed continuously at 115200 baud.
  No READY handshake or start command is used.
*/
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

constexpr int I2C_SDA = 8;
constexpr int I2C_SCL = 9;
constexpr uint32_t SAMPLE_INTERVAL_US = 10000; // target 100 Hz

Adafruit_ADS1115 ads;
Adafruit_MPU6050 mpu;
uint32_t nextSampleUs;
uint32_t startMs;

void setup() {
  Serial.begin(115200);
  delay(1200);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  if (!ads.begin(0x48, &Wire)) {
    while (true) delay(1000); // Stop if ADS1115 is not detected
  }
  if (!mpu.begin(0x68, &Wire)) {
    while (true) delay(1000); // Stop if MPU6050 is not detected
  }

  ads.setGain(GAIN_ONE); // +/-4.096 V range
  ads.setDataRate(RATE_ADS1115_860SPS);

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  startMs = millis();
  nextSampleUs = micros();
}

void loop() {
  uint32_t nowUs = micros();
  if ((int32_t)(nowUs - nextSampleUs) < 0) return;
  nextSampleUs += SAMPLE_INTERVAL_US;

  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  // Existing wiring: left piezo on A1; right piezo on A0.
  int16_t piezoLeft = ads.readADC_SingleEnded(1);
  int16_t piezoRight = ads.readADC_SingleEnded(0);

  // Columns: time_ms, ax, ay, az, gx, gy, gz, left piezo counts, right piezo counts
  Serial.print(millis() - startMs);
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
}
