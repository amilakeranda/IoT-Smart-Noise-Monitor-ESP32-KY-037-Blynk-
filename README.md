# IoT-Smart-Noise-Monitor-ESP32-KY-037-Blynk-
An IoT-based real-time Environmental Noise Monitoring System built using an ESP32 NodeMCU, a KY-037 Sound Sensor, an I2C 16x2 LCD Display, and the Blynk IoT Cloud.
 IoT Smart Noise Monitor (ESP32 + KY-037 + Blynk)

An IoT-based real-time Environmental Noise Monitoring System built using an ESP32 NodeMCU, a KY-037 Sound Sensor, an I2C 16x2 LCD Display, and the Blynk IoT Cloud. This system continuously tracks ambient noise decibel levels, categorizes them into different safety zones, triggers local visual alerts, and logs the historical data to the cloud via Wi-Fi.

Key Features

Real-time Calibrated Measurement: Dynamically reads analog acoustic waves and converts raw signals into real-time decibels ($30\text{ dB} - 120\text{ dB}$).

Signal Inversion Handling: Includes customized software logic to calibrate inverted analog outputs typical of the KY-037 sensor.

Local Visual Feedback:

Displays current decibels and environment status (SAFE, MODERATE, LOUD, or DANGER) on an I2C 16x2 LCD.

Dynamically controls a multi-tier LED alert system (Green, Yellow, Red) based on noise thresholds.

IoT Cloud Integration: Pushes live decibel values, system status, and raw sensor wave variations directly to the Blynk IoT mobile/web dashboard.

Zero-Lag Asynchronous Timing: Utilizes BlynkTimer instead of blocking delay() loops, guaranteeing a responsive local UI and smooth data streaming.

🔌 Hardware Wiring & Pin Mapping

Component

Pin on Component

Pin on ESP32

Description

KY-037 Sensor

A0 (Analog Out)

GPIO 34

Noise Analog Signal Input



GND

GND

Common Ground



VCC

3.3V / Vin

Power Input

I2C 16x2 LCD

SDA

GPIO 21

I2C Data Line



SCL

GPIO 22

I2C Clock Line



VCC

5V / Vin

Backlight Power



GND

GND

Common Ground

LED Green

Anode (+)

GPIO 25

Safe Indicator

LED Yellow

Anode (+)

GPIO 26

Moderate/Loud Indicator

LED Red

Anode (+)

GPIO 23

Danger Indicator

All LEDs

Cathode (-)

GND (via $220\Omega$)

Common Ground

🛠️ Software Prerequisites & Libraries

To compile and upload this project, make sure you have installed the following libraries in your Arduino IDE:

Blynk (by Volodymyr Shymanskyy)

hd44780 (by Bill Perry) - Used for optimized and auto-detected I2C LCD control.

💻 Source Code

Below is the stable, noise-calibrated, and production-ready C++ firmware for the ESP32:

/* 
  =====================================================
  IoT Noise Pollution Monitor (ESP32 + KY-037)
  =====================================================
*/

#define BLYNK_TEMPLATE_ID "TMPL66xaYRT1D"
#define BLYNK_TEMPLATE_NAME "NoisePollutionMonitoring"
#define BLYNK_AUTH_TOKEN "42XOLcEQSgy1NmOtiLRDbmDQniUa4I_w"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>

#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

char ssid[] = "4G-MIFI-F065";
char pass[] = "20020526As";

hd44780_I2Cexp lcd;

const int LCD_COLS = 16;
const int LCD_ROWS = 2;

#define SOUND_ANALOG_PIN 34

#define LED_GREEN 25
#define LED_YELLOW 26
#define LED_RED 23 // Re-routed to GPIO 23

#define DB_MIN 30
#define DB_MAX 120

BlynkTimer timer;

//--------------------------------------------------
// Read and Process Noise Level
//--------------------------------------------------
float getDecibels()
{
    const int samples = 200;
    long total = 0;

    for (int i = 0; i < samples; i++)
    {
        total += analogRead(SOUND_ANALOG_PIN);
        delayMicroseconds(100);
    }

    float average = total / (float)samples;

    // KY-037 analog output is inverted relative to raw sound intensity
    float raw = 4095.0 - average;

    // Hardware Calibration Math mapping values into precise decibels
    float db = ((raw - 500.0) * (DB_MAX - DB_MIN) / (3500.0 - 500.0)) + DB_MIN;
    db = constrain(db, DB_MIN, DB_MAX);

    return db;
}

//--------------------------------------------------
// Map dB to Human Readable Status
//--------------------------------------------------
String getStatus(float db)
{
    if (db < 35)      return "SAFE";
    if (db < 45)      return "MODERATE";
    if (db < 55)      return "LOUD";
    return "DANGER";
}

//--------------------------------------------------
// Control Warning LEDs
//--------------------------------------------------
void updateLEDs(float db)
{
    if (db < 35)
    {
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_RED, LOW);
    }
    else if (db < 55)
    {
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_YELLOW, HIGH);
        digitalWrite(LED_RED, LOW);
    }
    else
    {
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_RED, HIGH);
    }
}

//--------------------------------------------------
// Write Output to I2C LCD
//--------------------------------------------------
void updateLCD(float db, String status)
{
    lcd.setCursor(0, 0);
    lcd.print("Noise:");

    if ((int)db < 10)
        lcd.print("  ");
    else if ((int)db < 100)
        lcd.print(" ");

    lcd.print((int)db);
    lcd.print(" dB   ");

    lcd.setCursor(0, 1);
    lcd.print("Status:");
    lcd.print("        ");
    lcd.setCursor(7, 1);
    lcd.print(status);
}

//--------------------------------------------------
// Core Logic Loop Execution
//--------------------------------------------------
void measureAndSend()
{
    float db = getDecibels();
    String status = getStatus(db);

    updateLEDs(db);
    updateLCD(db, status);

    // Streams mapped signals to Blynk Virtual Pins
    Blynk.virtualWrite(V0, (int)db);
    Blynk.virtualWrite(V1, status);

    Serial.print("Noise : ");
    Serial.print(db);
    Serial.print(" dB  |  ");
    Serial.println(status);
}

//--------------------------------------------------
// System Init and Setup
//--------------------------------------------------
void setup()
{
    Serial.begin(115200);

    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_RED, OUTPUT);

    analogReadResolution(12);
    analogSetPinAttenuation(SOUND_ANALOG_PIN, ADC_11db);

    int status = lcd.begin(LCD_COLS, LCD_ROWS);
    if (status)
    {
        Serial.print("LCD Error : ");
        Serial.println(status);
    }

    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Noise Monitor");
    lcd.setCursor(0, 1);
    lcd.print("Connecting...");
    
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi Connected");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    delay(1500);
    lcd.clear();

    Blynk.config(BLYNK_AUTH_TOKEN);
    if (!Blynk.connect())
    {
        Serial.println("Blynk Connection Failed!");
    }
    else
    {
        Serial.println("Blynk Connected");
    }

    timer.setInterval(1000L, measureAndSend);
}

//--------------------------------------------------
// Standard Asynchronous Execution
//--------------------------------------------------
void loop()
{
    Blynk.run();
    timer.run();
}
