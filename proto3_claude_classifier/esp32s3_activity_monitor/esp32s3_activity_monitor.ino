// ESP32-S3 activity monitor: IMU -> classifier -> SSD1306 OLED.
// Libraries (Library Manager): Adafruit MPU6050, Adafruit SSD1306, Adafruit GFX.
// Board: "ESP32S3 Dev Module". Wiring: OLED+IMU share I2C (SDA=GPIO8, SCL=GPIO9).
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include "activity_classifier.h"

namespace cfg {
constexpr int      kSda = 8, kScl = 9;
constexpr uint32_t kI2cHz = 400000;
constexpr uint8_t  kOledAddr = 0x3C, kImuAddr = 0x68;
constexpr uint32_t kSamplePeriodUs = 10000;   // 100 Hz, matches training data
constexpr uint32_t kDisplayPeriodMs = 250;    // 4 Hz screen refresh
}  // namespace cfg

Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_MPU6050 imu;
act::Classifier classifier;
act::Sample last{};

// Only place that knows the IMU driver: swap here for another sensor.
bool readImu(act::Sample& s) {
  sensors_event_t a, g, t;
  if (!imu.getEvent(&a, &g, &t)) return false;
  s = {a.acceleration.x, a.acceleration.y, a.acceleration.z, g.gyro.x, g.gyro.y, g.gyro.z};
  return true;
}

void halt(const char* msg) {
  Serial.println(msg);
  for (;;) delay(1000);
}

void drawScreen(const act::Sample& s, act::Activity a) {
  char buf[24];
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(act::label(a));
  display.drawFastHLine(0, 18, 128, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 22); display.print(F("ACC m/s2"));
  snprintf(buf, sizeof buf, "%+5.1f %+5.1f %+5.1f", s.ax, s.ay, s.az);
  display.setCursor(0, 32); display.print(buf);
  display.setCursor(0, 44); display.print(F("GYRO rad/s"));
  snprintf(buf, sizeof buf, "%+5.2f %+5.2f %+5.2f", s.gx, s.gy, s.gz);
  display.setCursor(0, 54); display.print(buf);
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(cfg::kSda, cfg::kScl);
  Wire.setClock(cfg::kI2cHz);
  if (!display.begin(SSD1306_SWITCHCAPVCC, cfg::kOledAddr)) halt("OLED not found");
  if (!imu.begin(cfg::kImuAddr, &Wire)) halt("IMU not found");
  imu.setAccelerometerRange(MPU6050_RANGE_8_G);
  imu.setGyroRange(MPU6050_RANGE_500_DEG);
  imu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  static uint32_t nextSampleUs = micros();
  static uint32_t nextDrawMs = 0;

  if ((int32_t)(micros() - nextSampleUs) < 0) return;
  nextSampleUs += cfg::kSamplePeriodUs;

  if (readImu(last) && classifier.push(last)) {
    Serial.println(act::label(classifier.activity()));
  }

  if (millis() >= nextDrawMs) {
    drawScreen(last, classifier.activity());
    nextDrawMs = millis() + cfg::kDisplayPeriodMs;
    nextSampleUs = micros() + cfg::kSamplePeriodUs;  // skip backlog after the ~25 ms I2C draw
  }
}
