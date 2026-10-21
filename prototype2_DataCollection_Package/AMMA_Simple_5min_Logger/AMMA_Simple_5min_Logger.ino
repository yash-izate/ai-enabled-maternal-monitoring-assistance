#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// AMMA simple 5-minute logger: no Wi-Fi, ThingSpeak, or OLED.
// PL=ADS A1, PR=ADS A0; ML=MPU6050 0x69; MR=MPU6050 0x68.
// Serial DATA columns: PL,PR,ML_X,ML_Y,ML_Z,MR_X,MR_Y,MR_Z

#define SDA_PIN 8
#define SCL_PIN 9
#define ADS_ADDR 0x48
#define ML_ADDR 0x69
#define MR_ADDR 0x68

const unsigned long SAMPLE_INTERVAL_MS = 100; // nominal 10 Hz
Adafruit_ADS1115 ads;
Adafruit_MPU6050 mpuLeft, mpuRight;
unsigned long lastSample = 0;

void setup() {
  Serial.begin(115200);
  delay(1200);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  if (!ads.begin(ADS_ADDR, &Wire)) {
    Serial.println("#ERROR,ADS1115_NOT_FOUND");
    while (true) delay(1000);
  }
  ads.setGain(GAIN_ONE);
  ads.setDataRate(RATE_ADS1115_860SPS);

  if (!mpuLeft.begin(ML_ADDR, &Wire)) {
    Serial.println("#ERROR,LEFT_MPU6050_NOT_FOUND_0x69");
    while (true) delay(1000);
  }
  if (!mpuRight.begin(MR_ADDR, &Wire)) {
    Serial.println("#ERROR,RIGHT_MPU6050_NOT_FOUND_0x68");
    while (true) delay(1000);
  }

  mpuLeft.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpuRight.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpuLeft.setFilterBandwidth(MPU6050_BAND_21_HZ);
  mpuRight.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("#AMMA_READY");
  lastSample = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSample >= SAMPLE_INTERVAL_MS) {
    lastSample = now;
    int16_t pl = ads.readADC_SingleEnded(1);
    int16_t pr = ads.readADC_SingleEnded(0);

    sensors_event_t aL, gL, tL, aR, gR, tR;
    mpuLeft.getEvent(&aL, &gL, &tL);
    mpuRight.getEvent(&aR, &gR, &tR);

    Serial.print("DATA,");
    Serial.print(pl); Serial.print(',');
    Serial.print(pr); Serial.print(',');
    Serial.print(aL.acceleration.x, 4); Serial.print(',');
    Serial.print(aL.acceleration.y, 4); Serial.print(',');
    Serial.print(aL.acceleration.z, 4); Serial.print(',');
    Serial.print(aR.acceleration.x, 4); Serial.print(',');
    Serial.print(aR.acceleration.y, 4); Serial.print(',');
    Serial.println(aR.acceleration.z, 4);
  }
}
