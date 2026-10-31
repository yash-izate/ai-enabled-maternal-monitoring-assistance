/*
  AMMA Prototype 3 - Preliminary Activity Recognition
  Hardware:
    ESP32-S3
    MPU6050 at 0x68
    SSD1306 128x64 I2C OLED at 0x3C (assumed)
    Shared I2C: SDA GPIO 8, SCL GPIO 9

  Classifies likely activity using the user's first labelled trial recordings:
    walking, standing still, sitting still, stand/sit movement, lying still.

  IMPORTANT:
  - These are trial-tuned heuristics, not a clinically validated model.
  - Sitting/standing depends on sensor orientation and belt mounting.
  - "LYING STILL" is not sleep detection.
  - Piezo channels are not used for activity classification in this first version.
*/

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define I2C_SDA 8
#define I2C_SCL 9
#define MPU_ADDR 0x68
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// The supplied recordings are approximately 100 Hz.
const uint16_t SAMPLE_RATE_HZ = 100;
const uint16_t WINDOW_N = 100;       // 1 second of samples
const uint32_t SAMPLE_PERIOD_US = 1000000UL / SAMPLE_RATE_HZ;
const uint32_t DISPLAY_PERIOD_MS = 500;

Adafruit_MPU6050 mpu;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

float accMagBuf[WINDOW_N];
float gyroMagBuf[WINDOW_N];
float axBuf[WINDOW_N], ayBuf[WINDOW_N], azBuf[WINDOW_N];
uint16_t sampleIndex = 0;
bool windowReady = false;

uint32_t nextSampleUs = 0;
uint32_t lastDisplayMs = 0;

String activity = "STARTING";
String previousActivity = "";
float accMagSD = 0.0f;
float gyroRMS = 0.0f;
float meanAX = 0.0f, meanAY = 0.0f, meanAZ = 0.0f;
float postureAngleDeg = 0.0f;

void showMessage(const char* line1, const char* line2 = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(line1);
  display.println();
  display.println(line2);
  display.display();
}

void calculateWindowFeatures() {
  float sumMag = 0.0f, sumMagSq = 0.0f;
  float sumGyroSq = 0.0f;
  meanAX = meanAY = meanAZ = 0.0f;

  for (uint16_t i = 0; i < WINDOW_N; i++) {
    sumMag += accMagBuf[i];
    sumMagSq += accMagBuf[i] * accMagBuf[i];
    sumGyroSq += gyroMagBuf[i] * gyroMagBuf[i];
    meanAX += axBuf[i];
    meanAY += ayBuf[i];
    meanAZ += azBuf[i];
  }

  float meanMag = sumMag / WINDOW_N;
  float variance = (sumMagSq / WINDOW_N) - (meanMag * meanMag);
  if (variance < 0.0f) variance = 0.0f;
  accMagSD = sqrtf(variance);
  gyroRMS = sqrtf(sumGyroSq / WINDOW_N);

  meanAX /= WINDOW_N;
  meanAY /= WINDOW_N;
  meanAZ /= WINDOW_N;

  // For the supplied upright recordings, gravity is mainly on +Y.
  // For the supplied lying-front-up recording, gravity is mainly on +Z.
  // This posture estimate is only valid if mounting/orientation is consistent.
  postureAngleDeg = atan2f(fabsf(meanAZ), fabsf(meanAY)) * 180.0f / PI;
}

String classifyActivity() {
  float gravityMagnitude = sqrtf(meanAX * meanAX + meanAY * meanAY + meanAZ * meanAZ);

  // Detect clear, sustained locomotion using both movement and rotation.
  // Thresholds are initial heuristics based on this first dataset.
  if (accMagSD > 0.80f && gyroRMS > 0.25f) {
    return "WALKING";
  }

  // Moderate motion can be posture change, fidgeting, or other movement.
  // Do not force it into a posture label.
  if (accMagSD > 0.16f || gyroRMS > 0.13f) {
    return "MOVING / TRANSITION";
  }

  // If gravity is mainly on Z, this matches the supplied lying/front-up trial.
  if (fabsf(meanAZ) > fabsf(meanAY) * 1.15f &&
      fabsf(meanAZ) > fabsf(meanAX) * 1.15f) {
    return "LYING STILL";
  }

  // Sitting vs standing heuristic fitted to the supplied recordings:
  // standing trial: atan2(|Z|, |Y|) was about 6 degrees
  // sitting trial:  atan2(|Z|, |Y|) was about 17 degrees
  // This separation may fail if the belt or sensor is mounted differently.
  if (fabsf(meanAY) > fabsf(meanAZ) && fabsf(meanAY) > fabsf(meanAX)) {
    if (postureAngleDeg < 12.0f) return "STANDING";
    return "SITTING (LIKELY)";
  }

  // If the sensor is rotated differently than in the training recordings,
  // avoid making a confident posture guess.
  if (gravityMagnitude < 7.0f || gravityMagnitude > 13.0f) {
    return "CHECK SENSOR";
  }
  return "STILL / UNKNOWN";
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("AMMA | ACTIVITY");

  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 17);
  display.println(activity);

  display.setTextSize(1);
  display.setCursor(0, 43);
  display.print("Acc SD: ");
  display.print(accMagSD, 2);
  display.println(" m/s2");

  display.setCursor(0, 54);
  display.print("Gyro RMS: ");
  display.print(gyroRMS, 2);
  display.println(" rad/s");

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: SSD1306 OLED not found at 0x3C.");
    while (true) delay(1000);
  }

  showMessage("AMMA Activity", "Starting sensors...");

  if (!mpu.begin(MPU_ADDR, &Wire)) {
    showMessage("MPU6050 ERROR", "Check 0x68 / wiring");
    Serial.println("ERROR: MPU6050 not found at 0x68.");
    while (true) delay(1000);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  showMessage("AMMA Activity", "Collecting 1 sec...");
  nextSampleUs = micros();
  Serial.println("AMMA preliminary activity recognition started.");
}

void loop() {
  uint32_t nowUs = micros();
  if ((int32_t)(nowUs - nextSampleUs) >= 0) {
    nextSampleUs += SAMPLE_PERIOD_US;

    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    float ax = accel.acceleration.x;
    float ay = accel.acceleration.y;
    float az = accel.acceleration.z;
    float gx = gyro.gyro.x;
    float gy = gyro.gyro.y;
    float gz = gyro.gyro.z;

    axBuf[sampleIndex] = ax;
    ayBuf[sampleIndex] = ay;
    azBuf[sampleIndex] = az;
    accMagBuf[sampleIndex] = sqrtf(ax * ax + ay * ay + az * az);
    gyroMagBuf[sampleIndex] = sqrtf(gx * gx + gy * gy + gz * gz);

    sampleIndex++;
    if (sampleIndex >= WINDOW_N) {
      sampleIndex = 0;
      windowReady = true;
      calculateWindowFeatures();
      activity = classifyActivity();

      // CSV-like debug output for tuning. Open Serial Monitor at 115200.
      Serial.print(millis());
      Serial.print(',');
      Serial.print(activity);
      Serial.print(',');
      Serial.print(accMagSD, 4);
      Serial.print(',');
      Serial.print(gyroRMS, 4);
      Serial.print(',');
      Serial.print(meanAX, 3);
      Serial.print(',');
      Serial.print(meanAY, 3);
      Serial.print(',');
      Serial.print(meanAZ, 3);
      Serial.print(',');
      Serial.println(postureAngleDeg, 2);
    }
  }

  if (windowReady && millis() - lastDisplayMs >= DISPLAY_PERIOD_MS) {
    lastDisplayMs = millis();
    updateDisplay();
  }
}
