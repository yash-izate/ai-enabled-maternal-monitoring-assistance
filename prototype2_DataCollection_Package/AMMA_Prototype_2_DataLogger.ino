#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>


// =====================================================
// AMMA PROTOTYPE 2
// SENSOR MONITOR + THINGSPEAK + USB SERIAL CSV LOGGING
//
// FINAL SENSOR MAPPING
//
// PL = LEFT PIEZO  -> ADS1115 A1
// PR = RIGHT PIEZO -> ADS1115 A0
//
// ML = LEFT MPU    -> 0x69
// MR = RIGHT MPU   -> 0x68
//
// OLED             -> 0x3C
// ADS1115          -> 0x48
// =====================================================


// =====================================================
// WIFI SETTINGS
// =====================================================

const char* WIFI_SSID = "moto101";
const char* WIFI_PASSWORD = "223334444";


// =====================================================
// THINGSPEAK SETTINGS
// =====================================================

const unsigned long CHANNEL_ID = 3465279;

const char* WRITE_API_KEY = "5DSIU3NBIDPW7OGX";


// =====================================================
// I2C PINS
// =====================================================

#define SDA_PIN 8
#define SCL_PIN 9


// =====================================================
// I2C ADDRESSES
// =====================================================

#define OLED_ADDR 0x3C
#define ADS_ADDR  0x48

#define ML_ADDR   0x69
#define MR_ADDR   0x68


// =====================================================
// OLED
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);


// =====================================================
// SENSOR OBJECTS
// =====================================================

// ADS1115
Adafruit_ADS1115 ads;


// ML = LEFT MPU = 0x69
Adafruit_MPU6050 mpuLeft;


// MR = RIGHT MPU = 0x68
Adafruit_MPU6050 mpuRight;


// =====================================================
// SENSOR VARIABLES
// =====================================================

// Piezo
int16_t plRaw = 0;
int16_t prRaw = 0;

float plVoltage = 0.0;
float prVoltage = 0.0;


// Left MPU
float mlX = 0.0;
float mlY = 0.0;
float mlZ = 0.0;

float mlMagnitude = 0.0;


// Right MPU
float mrX = 0.0;
float mrY = 0.0;
float mrZ = 0.0;

float mrMagnitude = 0.0;


// =====================================================
// TIMING
// =====================================================

// Sensor reading:
// 100 ms = 10 Hz

const unsigned long SENSOR_INTERVAL = 100;


// ThingSpeak:
// 20 seconds

const unsigned long THINGSPEAK_INTERVAL = 20000;


unsigned long lastSensorRead = 0;
unsigned long lastThingSpeakUpdate = 0;

// =====================================================
// USB SERIAL CSV DATA-LOGGING CONTROL
// Commands sent from the laptop:
//   ACT=LYING | ACT=SITTING | ACT=STANDING
//   ACT=WALKING | ACT=TRANSITION
//   START | STOP | STATUS
// =====================================================

const unsigned long MAX_LOGGING_DURATION = 600000UL; // 10 minutes
bool csvLogging = false;
String activityLabel = "UNSET";
unsigned long loggingStartMillis = 0;
unsigned long loggingSampleCount = 0;



// =====================================================
// WIFI CONNECTION
// =====================================================

void connectWiFi()
{
  Serial.println();
  Serial.println("========================================");
  Serial.println("           WIFI CONNECTION");
  Serial.println("========================================");

  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);


  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  int attempts = 0;


  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 30
  )
  {
    delay(500);

    Serial.print(".");

    attempts++;
  }


  Serial.println();


  if (
    WiFi.status() == WL_CONNECTED
  )
  {
    Serial.println(
      "WiFi connected successfully!"
    );


    Serial.print(
      "IP Address : "
    );

    Serial.println(
      WiFi.localIP()
    );


    Serial.print(
      "Gateway    : "
    );

    Serial.println(
      WiFi.gatewayIP()
    );


    Serial.print(
      "DNS        : "
    );

    Serial.println(
      WiFi.dnsIP()
    );


    Serial.print(
      "RSSI       : "
    );

    Serial.print(
      WiFi.RSSI()
    );

    Serial.println(
      " dBm"
    );
  }
  else
  {
    Serial.println(
      "ERROR: WiFi connection failed."
    );
  }


  Serial.println(
    "========================================"
  );
}


// =====================================================
// READ SENSORS
// =====================================================

void readSensors()
{

  // ===================================================
  // PIEZO
  //
  // A1 = LEFT PIEZO
  // A0 = RIGHT PIEZO
  // ===================================================

  plRaw =
    ads.readADC_SingleEnded(1);


  prRaw =
    ads.readADC_SingleEnded(0);


  // ===================================================
  // PIEZO VOLTAGE
  // ===================================================

  plVoltage =
    ads.computeVolts(
      plRaw
    );


  prVoltage =
    ads.computeVolts(
      prRaw
    );


  // ===================================================
  // ML = LEFT MPU
  // ADDRESS = 0x69
  // ===================================================

  sensors_event_t accelML;
  sensors_event_t gyroML;
  sensors_event_t tempML;


  mpuLeft.getEvent(
    &accelML,
    &gyroML,
    &tempML
  );


  // ===================================================
  // MR = RIGHT MPU
  // ADDRESS = 0x68
  // ===================================================

  sensors_event_t accelMR;
  sensors_event_t gyroMR;
  sensors_event_t tempMR;


  mpuRight.getEvent(
    &accelMR,
    &gyroMR,
    &tempMR
  );


  // ===================================================
  // STORE LEFT MPU VALUES
  // ===================================================

  mlX =
    accelML.acceleration.x;

  mlY =
    accelML.acceleration.y;

  mlZ =
    accelML.acceleration.z;


  // ===================================================
  // STORE RIGHT MPU VALUES
  // ===================================================

  mrX =
    accelMR.acceleration.x;

  mrY =
    accelMR.acceleration.y;

  mrZ =
    accelMR.acceleration.z;


  // ===================================================
  // ACCELERATION MAGNITUDE
  // ===================================================

  mlMagnitude =
    sqrt(
      mlX * mlX +
      mlY * mlY +
      mlZ * mlZ
    );


  mrMagnitude =
    sqrt(
      mrX * mrX +
      mrY * mrY +
      mrZ * mrZ
    );
}


// =====================================================
// SERIAL OUTPUT
// =====================================================

void printSensorData()
{

  Serial.println(
    "----------------------------------------"
  );


  // ===================================================
  // PL
  // ===================================================

  Serial.print(
    "PL: "
  );

  Serial.print(
    plRaw
  );

  Serial.print(
    " | "
  );

  Serial.print(
    plVoltage,
    3
  );

  Serial.println(
    " V"
  );


  // ===================================================
  // PR
  // ===================================================

  Serial.print(
    "PR: "
  );

  Serial.print(
    prRaw
  );

  Serial.print(
    " | "
  );

  Serial.print(
    prVoltage,
    3
  );

  Serial.println(
    " V"
  );


  // ===================================================
  // ML
  // ===================================================

  Serial.print(
    "ML | X: "
  );

  Serial.print(
    mlX,
    2
  );

  Serial.print(
    " | Y: "
  );

  Serial.print(
    mlY,
    2
  );

  Serial.print(
    " | Z: "
  );

  Serial.print(
    mlZ,
    2
  );

  Serial.print(
    " | MAG: "
  );

  Serial.print(
    mlMagnitude,
    2
  );

  Serial.println(
    " m/s2"
  );


  // ===================================================
  // MR
  // ===================================================

  Serial.print(
    "MR | X: "
  );

  Serial.print(
    mrX,
    2
  );

  Serial.print(
    " | Y: "
  );

  Serial.print(
    mrY,
    2
  );

  Serial.print(
    " | Z: "
  );

  Serial.print(
    mrZ,
    2
  );

  Serial.print(
    " | MAG: "
  );

  Serial.print(
    mrMagnitude,
    2
  );

  Serial.println(
    " m/s2"
  );
}


// =====================================================
// OLED DISPLAY
// =====================================================

void updateOLED()
{

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);


  // ===================================================
  // HEADER
  // ===================================================

  display.setCursor(
    0,
    0
  );

  display.println(
    "AMMA SENSOR MONITOR"
  );


  // ===================================================
  // PL
  // ===================================================

  display.setCursor(
    0,
    12
  );

  display.print(
    "PL:"
  );

  display.print(
    plRaw
  );


  // ===================================================
  // PR
  // ===================================================

  display.setCursor(
    65,
    12
  );

  display.print(
    "PR:"
  );

  display.print(
    prRaw
  );


  // ===================================================
  // ML MAGNITUDE
  // ===================================================

  display.setCursor(
    0,
    25
  );

  display.print(
    "ML MAG:"
  );

  display.print(
    mlMagnitude,
    2
  );


  // ===================================================
  // MR MAGNITUDE
  // ===================================================

  display.setCursor(
    0,
    37
  );

  display.print(
    "MR MAG:"
  );

  display.print(
    mrMagnitude,
    2
  );


  // ===================================================
  // WIFI STATUS
  // ===================================================

  display.setCursor(
    0,
    51
  );

  if (
    WiFi.status() == WL_CONNECTED
  )
  {
    display.print(
      "WiFi: OK"
    );
  }
  else
  {
    display.print(
      "WiFi: OFF"
    );
  }


  // ===================================================
  // UPDATE DISPLAY
  // ===================================================

  display.display();
}


// =====================================================
// SEND DATA TO THINGSPEAK
// =====================================================

void sendToThingSpeak()
{

  Serial.println();
  Serial.println(
    "========================================"
  );

  Serial.println(
    "Sending AMMA data to ThingSpeak..."
  );


  // ===================================================
  // CHECK WIFI
  // ===================================================

  if (
    WiFi.status() != WL_CONNECTED
  )
  {
    Serial.println(
      "ERROR: WiFi disconnected."
    );

    Serial.println(
      "Skipping ThingSpeak upload."
    );

    Serial.println(
      "========================================"
    );

    return;
  }


  // ===================================================
  // SECURE CLIENT
  // ===================================================

  WiFiClientSecure client;

  // For prototype/testing.
  // Certificate verification disabled.

  client.setInsecure();


  HTTPClient http;


  // ===================================================
  // THINGSPEAK UPDATE URL
  // ===================================================

  String url =
    "https://api.thingspeak.com/update";


  url +=
    "?api_key=";

  url +=
    WRITE_API_KEY;


  // ===================================================
  // FIELD 1
  // PL RAW
  // ===================================================

  url +=
    "&field1=";

  url +=
    String(
      plRaw
    );


  // ===================================================
  // FIELD 2
  // PR RAW
  // ===================================================

  url +=
    "&field2=";

  url +=
    String(
      prRaw
    );


  // ===================================================
  // FIELD 3
  // ML MAGNITUDE
  // ===================================================

  url +=
    "&field3=";

  url +=
    String(
      mlMagnitude,
      2
    );


  // ===================================================
  // FIELD 4
  // MR MAGNITUDE
  // ===================================================

  url +=
    "&field4=";

  url +=
    String(
      mrMagnitude,
      2
    );


  // ===================================================
  // FIELD 5
  // ML X
  // ===================================================

  url +=
    "&field5=";

  url +=
    String(
      mlX,
      2
    );


  // ===================================================
  // FIELD 6
  // ML Y
  // ===================================================

  url +=
    "&field6=";

  url +=
    String(
      mlY,
      2
    );


  // ===================================================
  // FIELD 7
  // MR X
  // ===================================================

  url +=
    "&field7=";

  url +=
    String(
      mrX,
      2
    );


  // ===================================================
  // FIELD 8
  // MR Y
  // ===================================================

  url +=
    "&field8=";

  url +=
    String(
      mrY,
      2
    );


  // ===================================================
  // PRINT URL
  // ===================================================

  Serial.println(
    "Uploading:"
  );

  Serial.println(
    "AMMA sensor data → ThingSpeak"
  );


  // ===================================================
  // HTTP REQUEST
  // ===================================================

  http.begin(
    client,
    url
  );


  int httpCode =
    http.GET();


  // ===================================================
  // RESPONSE
  // ===================================================

  Serial.print(
    "HTTP Response Code: "
  );

  Serial.println(
    httpCode
  );


  if (
    httpCode > 0
  )
  {

    String response =
      http.getString();


    Serial.print(
      "ThingSpeak Response: "
    );

    Serial.println(
      response
    );


    int entryID =
      response.toInt();


    if (
      entryID > 0
    )
    {

      Serial.println();
      Serial.println(
        "****************************************"
      );

      Serial.println(
        " AMMA DATA SENT SUCCESSFULLY"
      );

      Serial.print(
        " Entry ID: "
      );

      Serial.println(
        entryID
      );

      Serial.println(
        "****************************************"
      );
    }
    else
    {

      Serial.println(
        "ThingSpeak returned 0."
      );

      Serial.println(
        "Upload may have been rejected"
      );

      Serial.println(
        "because of the 15-second limit."
      );
    }
  }
  else
  {

    Serial.println(
      "ThingSpeak HTTP request failed."
    );

    Serial.print(
      "Error: "
    );

    Serial.println(
      http.errorToString(
        httpCode
      )
    );
  }


  http.end();


  Serial.println(
    "========================================"
  );
}


// =====================================================
// CSV DATA LOGGING OVER USB SERIAL
// Data records begin with DATA, so the laptop logger can
// ignore normal diagnostic and ThingSpeak messages.
// =====================================================

bool isValidActivity(const String& value)
{
  return value == "LYING" || value == "SITTING" ||
         value == "STANDING" || value == "WALKING" ||
         value == "TRANSITION";
}

void handleSerialCommand()
{
  if (!Serial.available()) return;

  String command = Serial.readStringUntil('\n');
  command.trim();
  command.toUpperCase();

  if (command.startsWith("ACT="))
  {
    String requestedActivity = command.substring(4);
    requestedActivity.trim();

    if (csvLogging)
    {
      Serial.println("#ERROR: Stop logging before changing activity.");
    }
    else if (isValidActivity(requestedActivity))
    {
      activityLabel = requestedActivity;
      Serial.print("#ACTIVITY=");
      Serial.println(activityLabel);
    }
    else
    {
      Serial.println("#ERROR: Use LYING, SITTING, STANDING, WALKING, or TRANSITION.");
    }
  }
  else if (command == "START")
  {
    if (csvLogging)
    {
      Serial.println("#ERROR: Logging is already active.");
    }
    else if (!isValidActivity(activityLabel))
    {
      Serial.println("#ERROR: Set activity first using ACT=LYING, etc.");
    }
    else
    {
      csvLogging = true;
      loggingStartMillis = millis();
      loggingSampleCount = 0;
      Serial.print("#LOG_STARTED,activity=");
      Serial.print(activityLabel);
      Serial.println(",duration_s=600");
    }
  }
  else if (command == "STOP")
  {
    if (csvLogging)
    {
      csvLogging = false;
      Serial.print("#LOG_STOPPED,samples=");
      Serial.println(loggingSampleCount);
    }
    else
    {
      Serial.println("#LOG_NOT_ACTIVE");
    }
  }
  else if (command == "STATUS")
  {
    Serial.print("#STATUS,activity=");
    Serial.print(activityLabel);
    Serial.print(",logging=");
    Serial.print(csvLogging ? "1" : "0");
    Serial.print(",samples=");
    Serial.println(loggingSampleCount);
  }
  else if (command == "HELP")
  {
    Serial.println("#COMMANDS: ACT=LYING|SITTING|STANDING|WALKING|TRANSITION, START, STOP, STATUS");
  }
  else if (command.length() > 0)
  {
    Serial.println("#ERROR: Unknown command. Send HELP.");
  }
}

void printCsvSample()
{
  if (!csvLogging) return;

  const unsigned long now = millis();
  const unsigned long elapsed = now - loggingStartMillis;

  // DATA prefix distinguishes records from diagnostic text.
  Serial.print("DATA,");
  Serial.print(now);
  Serial.print(',');
  Serial.print(elapsed);
  Serial.print(',');
  Serial.print(activityLabel);
  Serial.print(',');
  Serial.print(plRaw);
  Serial.print(',');
  Serial.print(prRaw);
  Serial.print(',');
  Serial.print(plVoltage, 4);
  Serial.print(',');
  Serial.print(prVoltage, 4);
  Serial.print(',');
  Serial.print(mlX, 4);
  Serial.print(',');
  Serial.print(mlY, 4);
  Serial.print(',');
  Serial.print(mlZ, 4);
  Serial.print(',');
  Serial.print(mlMagnitude, 4);
  Serial.print(',');
  Serial.print(mrX, 4);
  Serial.print(',');
  Serial.print(mrY, 4);
  Serial.print(',');
  Serial.print(mrZ, 4);
  Serial.print(',');
  Serial.println(mrMagnitude, 4);

  loggingSampleCount++;

  if (elapsed >= MAX_LOGGING_DURATION)
  {
    csvLogging = false;
    Serial.print("#LOG_AUTO_STOPPED,samples=");
    Serial.println(loggingSampleCount);
  }
}

// =====================================================
// SETUP
// =====================================================

void setup()
{

  Serial.begin(
    115200
  );


  delay(
    1000
  );


  // ===================================================
  // I2C
  // ===================================================

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );


  Wire.setClock(
    400000
  );


  // ===================================================
  // STARTUP MESSAGE
  // ===================================================

  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "       AMMA SENSOR + THINGSPEAK"
  );

  Serial.println(
    "========================================"
  );


  Serial.println();

  Serial.println(
    "FINAL SENSOR MAPPING:"
  );

  Serial.println(
    "PL = Left Piezo  -> ADS A1"
  );

  Serial.println(
    "PR = Right Piezo -> ADS A0"
  );

  Serial.println(
    "ML = Left MPU    -> 0x69"
  );

  Serial.println(
    "MR = Right MPU   -> 0x68"
  );

  Serial.println();


  // ===================================================
  // OLED
  // ===================================================

  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDR
    )
  )
  {

    Serial.println(
      "OLED ERROR"
    );

    while (1);
  }


  Serial.println(
    "OLED 0x3C: OK"
  );


  // ===================================================
  // ADS1115
  // ===================================================

  if (
    !ads.begin(
      ADS_ADDR,
      &Wire
    )
  )
  {

    Serial.println(
      "ADS1115 ERROR"
    );

    while (1);
  }


  // ±4.096 V range

  ads.setGain(
    GAIN_ONE
  );


  // Fast conversion

  ads.setDataRate(
    RATE_ADS1115_860SPS
  );


  Serial.println(
    "ADS1115 0x48: OK"
  );


  // ===================================================
  // ML = LEFT MPU
  // 0x69
  // ===================================================

  if (
    !mpuLeft.begin(
      ML_ADDR,
      &Wire
    )
  )
  {

    Serial.println(
      "ML ERROR"
    );

    while (1);
  }


  Serial.println(
    "ML 0x69: OK"
  );


  // ===================================================
  // MR = RIGHT MPU
  // 0x68
  // ===================================================

  if (
    !mpuRight.begin(
      MR_ADDR,
      &Wire
    )
  )
  {

    Serial.println(
      "MR ERROR"
    );

    while (1);
  }


  Serial.println(
    "MR 0x68: OK"
  );


  // ===================================================
  // MPU CONFIGURATION
  // ===================================================

  mpuLeft.setAccelerometerRange(
    MPU6050_RANGE_8_G
  );

  mpuRight.setAccelerometerRange(
    MPU6050_RANGE_8_G
  );


  mpuLeft.setGyroRange(
    MPU6050_RANGE_500_DEG
  );

  mpuRight.setGyroRange(
    MPU6050_RANGE_500_DEG
  );


  mpuLeft.setFilterBandwidth(
    MPU6050_BAND_21_HZ
  );

  mpuRight.setFilterBandwidth(
    MPU6050_BAND_21_HZ
  );


  // ===================================================
  // READY SCREEN
  // ===================================================

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  display.setTextSize(2);

  display.setCursor(
    25,
    5
  );

  display.println(
    "AMMA"
  );


  display.setTextSize(1);

  display.setCursor(
    18,
    30
  );

  display.println(
    "SENSOR + CLOUD"
  );


  display.setCursor(
    22,
    45
  );

  display.println(
    "Sensors Ready"
  );


  display.display();


  delay(
    1500
  );


  // ===================================================
  // WIFI
  // ===================================================

  connectWiFi();


  // ===================================================
  // THINGSPEAK STATUS
  // ===================================================

  if (
    WiFi.status() == WL_CONNECTED
  )
  {

    Serial.println(
      "ThingSpeak ready."
    );

    Serial.print(
      "Channel ID: "
    );

    Serial.println(
      CHANNEL_ID
    );
  }


  // ===================================================
  // INITIAL TIMERS
  // ===================================================

  lastSensorRead =
    millis();

  lastThingSpeakUpdate =
    millis();
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop()
{

  handleSerialCommand();

  unsigned long currentMillis =
    millis();


  // ===================================================
  // SENSOR ACQUISITION
  //
  // 100 ms = 10 Hz
  // ===================================================

  if (
    currentMillis -
    lastSensorRead >=
    SENSOR_INTERVAL
  )
  {

    lastSensorRead =
      currentMillis;


    // Read sensors

    readSensors();


    // During CSV logging, emit machine-readable DATA rows.
    // Outside logging, retain the original human-readable output.
    if (csvLogging)
    {
      printCsvSample();
    }
    else
    {
      printSensorData();
    }


    // OLED

    updateOLED();
  }


  // ===================================================
  // THINGSPEAK
  //
  // EVERY 20 SECONDS
  // ===================================================

  if (
    currentMillis -
    lastThingSpeakUpdate >=
    THINGSPEAK_INTERVAL
  )
  {

    lastThingSpeakUpdate =
      currentMillis;


    // Check WiFi

    if (
      WiFi.status() != WL_CONNECTED
    )
    {

      Serial.println();
      Serial.println(
        "WiFi disconnected."
      );

      connectWiFi();
    }


    // Upload

    if (
      WiFi.status() == WL_CONNECTED
    )
    {

      sendToThingSpeak();
    }
    else
    {

      Serial.println(
        "Upload skipped: WiFi unavailable."
      );
    }
  }
}