#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#include <RTClib.h>
TwoWire Wire(0);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define TOUCH_PIN 0
#define BUZZER_PIN 1
#define PRESSURE_THRESHOLD 2.0
#define BT_NAME "LIFE-LINK_01"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BMP280 bmp;
RTC_DS3231 rtc;

float initialPressure;
bool sosMode = false;
DateTime startTime;
uint32_t lastUpdate = 0;

void setup() {
  Serial.begin(9600);   // Using only Serial (USB UART)

  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    Serial.println("OLED Fail");

  if (!bmp.begin(0x76))
    Serial.println("BMP Fail");

  if (!rtc.begin())
    Serial.println("RTC Fail");

  initialPressure = bmp.readPressure() / 100.0F;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.print("SYSTEM ARMED");
  display.display();
}

void loop() {
  float currentPressure = bmp.readPressure() / 100.0F;
  bool isTouched = (digitalRead(TOUCH_PIN) == HIGH);

  if (!sosMode && (isTouched || fabs(currentPressure - initialPressure) > PRESSURE_THRESHOLD)) {
    activateSOS();
  }

  if (sosMode) {
    if (millis() - lastUpdate >= 1000) {
      runSOSSequence();
      lastUpdate = millis();
    }
  } else {
    updateStandbyUI(currentPressure);
  }
}

void activateSOS() {
  sosMode = true;
  startTime = rtc.now();

  Serial.print("\n>>> ALERT ACTIVATED: ");
  Serial.println(BT_NAME);
}

void runSOSSequence() {
  DateTime now = rtc.now();
  TimeSpan elapsed = now - startTime;

  // Send data via Serial (Bluetooth must share this line)
  Serial.print("ID:");
  Serial.print(BT_NAME);
  Serial.print("|ELAPSED:");
  Serial.println(elapsed.totalseconds());

  // Buzzer Pulse
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  // OLED Update
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("SIGNAL: ");
  display.print(BT_NAME);

  display.setTextSize(2);
  display.setCursor(10, 20);
  display.print("EMERGENCY");

  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("Time: ");
  display.print(elapsed.hours()); display.print("h ");
  display.print(elapsed.minutes()); display.print("m ");
  display.print(elapsed.seconds()); display.print("s");

  display.display();
}

void updateStandbyUI(float p) {
  if (millis() - lastUpdate >= 5000) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("MONITORING...");
    display.setCursor(0, 25);
    display.print("P: ");
    display.print(p, 1);
    display.print(" hPa");
    display.display();

    lastUpdate = millis();
  }
}
