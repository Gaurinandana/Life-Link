#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <RTClib.h>

TwoWire Wire(0); // Hardware I2C on ARIES

#define TOUCH_PIN 0
#define BUZZER_PIN 1
#define PRESSURE_THRESHOLD 2.0
#define BT_NAME "LIFE-LINK_01"

// Timing Thresholds
#define TRIGGER_HOLD_TIME 3000   
#define PRESSURE_CONFIRM_TIME 2000 
#define RESET_HOLD_TIME 5000     

Adafruit_BMP280 bmp;
RTC_DS3231 rtc;

float initialPressure;
bool sosMode = false;
DateTime startTime;
uint32_t lastUpdate = 0;
uint32_t touchStartTime = 0;
uint32_t pressureStartTime = 0;

void setup() {
  Serial.begin(9600); // Bluetooth and Serial Monitor output
  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin();
  
  if (!bmp.begin(0x76)) Serial.println("CRITICAL: BMP280 Not Found");
  if (!rtc.begin()) Serial.println("CRITICAL: RTC Not Found");

  initialPressure = bmp.readPressure() / 100.0F;

  Serial.println("--- LIFE-LINK BEACON INITIALIZED ---");
  Serial.println("STATUS: MONITORING | MODE: ARMED");
}

void loop() {
  float currentPressure = bmp.readPressure() / 100.0F;
  bool isTouched = (digitalRead(TOUCH_PIN) == HIGH);
  bool pressureAboveLimit = fabs(currentPressure - initialPressure) > PRESSURE_THRESHOLD;

  // 1. TOUCH SENSOR LOGIC
  if (isTouched) {
    if (touchStartTime == 0) touchStartTime = millis();
    uint32_t holdTime = millis() - touchStartTime;

    if (!sosMode && holdTime >= TRIGGER_HOLD_TIME) {
      activateSOS("MANUAL_PANIC_TRIGGER");
      touchStartTime = 0;
    } 
    else if (sosMode && holdTime >= RESET_HOLD_TIME) {
      deactivateSOS();
      touchStartTime = 0;
    }
  } else {
    touchStartTime = 0; 
  }

  // 2. PRESSURE SENSOR LOGIC
  if (!sosMode && pressureAboveLimit) {
    if (pressureStartTime == 0) pressureStartTime = millis();
    if (millis() - pressureStartTime >= PRESSURE_CONFIRM_TIME) {
      activateSOS("LANDSLIDE_DETECTION");
      pressureStartTime = 0;
    }
  } else {
    pressureStartTime = 0;
  }

  // 3. LOGGING & SIGNALLING
  if (sosMode) {
    if (millis() - lastUpdate >= 1000) {
      broadcastData();
      lastUpdate = millis();
    }
  } else {
    // Optional: Print status to monitor every 10 seconds to show it's alive
    if (millis() - lastUpdate >= 10000) {
      Serial.print("Monitoring... Current Pressure: ");
      Serial.print(currentPressure);
      Serial.println(" hPa");
      lastUpdate = millis();
    }
  }
}

void activateSOS(String reason) {
  sosMode = true;
  startTime = rtc.now();
  Serial.println("\n*******************************");
  Serial.print("!!! SOS ACTIVATED BY: "); Serial.println(reason);
  Serial.print("INCIDENT START TIME: "); 
  Serial.print(startTime.year()); Serial.print("/"); 
  Serial.print(startTime.month()); Serial.print("/"); 
  Serial.println(startTime.day());
  Serial.print("EXACT TIME: ");
  Serial.print(startTime.hour()); Serial.print(":"); 
  Serial.print(startTime.minute()); Serial.print(":"); 
  Serial.println(startTime.second());
  Serial.println("*******************************\n");
}

void deactivateSOS() {
  sosMode = false;
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("\n>>> RESCUE CONFIRMED. SYSTEM RESET TO MONITORING MODE. <<<\n");
  initialPressure = bmp.readPressure() / 100.0F; // Recalibrate
}

void broadcastData() {
  DateTime now = rtc.now();
  TimeSpan elapsed = now - startTime;

  // This goes to both your PC and the Rescuer's Phone via Bluetooth
  Serial.print("[BEACON_ID: "); Serial.print(BT_NAME); Serial.print("] ");
  Serial.print("| STATUS: TRAPPED | ");
  Serial.print("DURATION: ");
  Serial.print(elapsed.hours()); Serial.print("h ");
  Serial.print(elapsed.minutes()); Serial.print("m ");
  Serial.print(elapsed.seconds()); Serial.print("s | ");
  Serial.print("RTC_NOW: "); Serial.print(now.hour()); Serial.print(":"); Serial.println(now.minute());

  // Buzzer Pulse for location tracking
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
}
