Life-Link Emergency Beacon (v2.1)
🚨 Overview
Life-Link is a smart disaster-response node designed to turn trapped survivors into active beacons. Built on the THEJAS32 ARIES VEGA v3.0 (RISC-V) platform, the device uses environmental sensing and real-time clock data to provide rescuers with a "Digital Breadcrumb" and vital medical timelines.

🛠 Hardware Components
Processor: THEJAS32 ARIES VEGA v3.0 (RISC-V SoC)

Pressure Sensor: BMP280 (I2C) - Detects landslides and burial depth.

Real-Time Clock: DS3231 (I2C) - Provides high-precision incident timestamps.

Panic Button: Capacitive Touch Sensor (GPIO) - Manual trigger for survivors.

Audio Beacon: Active Piezo Buzzer - Emits high-frequency SOS pulses.

Communication: HC-05/HM-10 Bluetooth Module - Broadcasts survival data to smartphones.

🧠 Logic & Features
The code is optimized for reliability and power efficiency using a headless (OLED-free) architecture.

1. Anti-False Alarm System
Touch Debouncing: Requires a 3-second hold to trigger SOS, preventing accidental activation from bumps or debris.

Pressure Verification: Landslide detection is only confirmed if a pressure spike of >2.0 hPa is sustained for at least 2 seconds, filtering out wind gusts or atmospheric noise.

2. Medical Triage Data
Utilizes the DS3231 RTC to log the exact YY/MM/DD HH:MM:SS of the incident.

Automatically calculates Trapped Duration, allowing medical teams to assess the risk of "Crush Syndrome" before extraction.

3. Rescue Reset (Deactivation)
Integrated a "Rescue Complete" feature: holding the touch sensor for 5 seconds during an active alarm resets the system to monitoring mode.

📡 Data Output Format
In SOS Mode, the beacon broadcasts the following string via Bluetooth/Serial every 1 second:
[BEACON_ID: LIFE-LINK_01] | STATUS: TRAPPED | DURATION: 0h 15m 30s | RTC_NOW: 14:45
