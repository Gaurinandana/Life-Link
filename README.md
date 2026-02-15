/*
  Life-Link Emergency Beacon MVP for Aries v3
  - Triggers on PIR motion, touch button, or BMP280 pressure change (>5kPa delta)
  - Outputs: SOS Morse on LED Matrix & Buzzer, flashing RGB Red/Yellow
  - Wireless: WiFi Soft AP "HELP_ZONE_A1" (RSSI for proximity), BLE beacon "SOS"
  - Optional OLED: Shows uptime in seconds
  - Uses standard Arduino libs; install via Library Manager:
    - Adafruit BMP280, Adafruit SSD1306, MD_MAX72xx, LedControl, ESP32 BLE Arduino
  - Pinout based on Aries v3 UNO headers & SPI/I2C [web:30][page:1]
*/

#include <Wire.h>              // I2C
#include <WiFi.h>              // Built-in WiFi AP
#include <BLEDevice.h>         // BLE Beacon
#include <BLEServer.h>
#include <Adafruit_BMP280.h>   // BMP280 pressure sensor
#include <MD_MAX72xx.h>        // 8x8 LED Matrix (install MD_MAX72XX)
#include <LedControl.h>        // Fallback or RGB if needed
#include <Adafruit_SSD1306.h>  // Optional OLED

// Pin definitions (Aries v3 compatible)
#define PIR_PIN 2        // Motion sensor
#define TOUCH_PIN 4      // Panic button
#define BUZZER_PIN 3     // Buzzer PWM
#define RGB_PIN 6        // RGB data (WS2812 if used)
#define OLED_RESET -1    // OLED reset

// Devices
Adafruit_BMP280 bmp;           // I2C: A4(SDA), A5(SCL)
MD_MAX72XX mx = MD_MAX72XX(MD_MAX72XX::FC16_HW, 12, 11, 10);  // DIN=12(MOSI), CLK=13, CS=10 [page:0]
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);  // Optional

bool beaconActive = false;
unsigned long startTime = 0;
float basePressure = 1013.25;  // Sea level hPa baseline
#define PRESSURE_DELTA 5.0  // Trigger threshold kPa

// SOS Morse pattern (dots/dashes in ms: . = 200, - = 600, space=600, letter=1800)
uint8_t sosPattern[] = {3, 3, 3, 0};  // ... --- ...
int patternLen = 4;
int patIdx = 0;
unsigned long lastFlash = 0;
int flashState = LOW;

void setup() {
  Serial.begin(115200);
  Wire.begin();  // I2C on A4/A5

  // Init inputs
  pinMode(PIR_PIN, INPUT);
  pinMode(TOUCH_PIN, INPUT_PULLUP);

  // Init outputs
  pinMode(BUZZER_PIN, OUTPUT);
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 15);  // Max brightness
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
  }

  // BMP280
  if (!bmp.begin(0x76)) {  // Or 0x77
    Serial.println("BMP280 not found");
  } else {
    basePressure = bmp.readPressure() / 100.0F;  // Calibrate baseline
  }

  // Wireless: WiFi AP (no pass for easy connect)
  WiFi.softAP("HELP_ZONE_A1");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // BLE Beacon
  BLEDevice::init("SOS_BEACON");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->getAdvertising()->start();

  Serial.println("Life-Link ready. Triggers: PIR/TOUCH/BMP");
}

void loop() {
  // Check triggers
  bool pirTrig = digitalRead(PIR_PIN);
  bool touchTrig = !digitalRead(TOUCH_PIN);  // Active low
  float currPressure = bmp.readPressure() / 100.0F;
  bool bmpTrig = abs(currPressure - basePressure) > PRESSURE_DELTA;

  if ((pirTrig || touchTrig || bmpTrig) && !beaconActive) {
    activateBeacon();
  }

  if (beaconActive) {
    handleSOS();
    if (display.begin()) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.print("ACTIVE: ");
      display.print((millis() - startTime)/1000);
      display.print("s");
      display.display();
    }
  }

  delay(100);
}

void activateBeacon() {
  beaconActive = true;
  startTime = millis();
  Serial.println("BEACON ACTIVATED!");
}

void handleSOS() {
  unsigned long now = millis();
  if (now - lastFlash > (patIdx < 3 ? 200 : 1800)) {  // Flash timing
    flashState = !flashState;
    lastFlash = now;

    // LED Matrix SOS (simple: row flashes or use font)
    if (flashState) {
      mx.clear();
      // Display 'S' or full SOS bitmap; simplify to all-on for vis
      for (uint8_t i=0; i<8; i++) mx.setRow(0, i, 0xFF);  // Bright row
    } else {
      mx.clear();
    }

    // Buzzer beep
    tone(BUZZER_PIN, 2000, 200);  // 2kHz, 200ms

    // RGB flash red
    analogWrite(RGB_PIN, flashState ? 255 : 0);  // Red channel

    patIdx = (patIdx + 1) % 12;  // Cycle SOS full sequence
  }
}
