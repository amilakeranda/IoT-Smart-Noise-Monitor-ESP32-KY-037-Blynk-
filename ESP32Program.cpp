/*
=====================================================
 IoT Noise Pollution Monitor (ESP32 + KY-037)
 LCD + Blynk + LEDs
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

char ssid[] = "WiFi-Name";
char pass[] = "password";

hd44780_I2Cexp lcd;

const int LCD_COLS = 16;
const int LCD_ROWS = 2;

#define SOUND_ANALOG_PIN 34

#define LED_GREEN 25
#define LED_YELLOW 26
#define LED_RED 27

#define DB_MIN 30
#define DB_MAX 120

BlynkTimer timer;

//--------------------------------------------------
// Read Noise Level
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

    // KY-037 output is inverted

    float raw = 4095.0 - average;

    // Calibration

    float db =
        ((raw - 500.0) * (DB_MAX - DB_MIN) /
         (3500.0 - 500.0))
        + DB_MIN;

    db = constrain(db, DB_MIN, DB_MAX);

    return db;
}

//--------------------------------------------------

String getStatus(float db)
{
    if (db < 35)
        return "SAFE";

    if (db < 45)
        return "MODERATE";

    if (db < 55)
        return "LOUD";

    return "DANGER";
}

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

void measureAndSend()
{
    float db = getDecibels();

    String status = getStatus(db);

    updateLEDs(db);

    updateLCD(db, status);

    Blynk.virtualWrite(V0, (int)db);

    Blynk.virtualWrite(V1, status);

    Serial.print("Noise : ");
    Serial.print(db);
    Serial.print(" dB  |  ");
    Serial.println(status);
}

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

    Serial.println();
    Serial.println("WiFi Connected");

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

void loop()
{
    Blynk.run();

    timer.run();
}