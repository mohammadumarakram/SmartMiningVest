#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// WiFi & Telegram Setup
const char* ssid = "project";
const char* password = "project007";
#define BOT_TOKEN "7633819878:AAHW_WNw09s2sAxu0vdXZPq-1bS1Fen6aK8"
#define CHAT_ID "6311067019"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// DHT11 Setup
#define DHTPIN D7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// MPU6050 Setup
Adafruit_MPU6050 mpu;

// MQ135 Setup
#define GAS_PIN A0
int gasThreshold = 200; // Adjust based on calibration

// LED and Button
#define LED_PIN D6
#define BUTTON_PIN D5

// Thresholds
float tempThreshold = 40.0;     // °C
float humidityThreshold = 70.0; // %

unsigned long lastButtonPress = 0;
bool fallDetected = false;

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,HIGH);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  dht.begin();
  Wire.begin();
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1);
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  client.setInsecure(); // for https
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  bot.sendMessage(CHAT_ID, "🟢 System Started!", "");
}

#define GAS_PIN A0
float Ro = 10.0; // Calibrate this value based on clean air

float getMQ135PPM() {
  int adcValue = analogRead(GAS_PIN);
  //float voltage = (adcValue / 1023.0) * 5.0;
  // float Rs = ((5.0 * 10.0) / voltage) - 10.0; // using 10k load resistor

  // float ratio = Rs/Ro;

  // // CO Curve (from datasheet approximation)
  // float ppm = 605.18 * pow(ratio, -3.937);
  return adcValue;
}

void loop() {
  checkGas();
  checkTempHumidity();
  checkFall();
  checkButton();
  delay(1000);
}

// Function to blink LED pattern
void blinkLED(int times, int delayTime) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(delayTime);
    digitalWrite(LED_PIN, HIGH);
    delay(delayTime);
  }
}

// 1. Gas Detection
void checkGas() {
  float ppm = getMQ135PPM();
  Serial.print("CO PPM: ");
  Serial.println(ppm);

  if (ppm > gasThreshold) {
    blinkLED(1, 500);
    bot.sendMessage(CHAT_ID, "⚠️ *Carbon Monoxide Detected!*\nPlease take immediate action.", "Markdown");
  }
}

// 2. Temperature & Humidity
void checkTempHumidity() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temp: "); Serial.println(t);
  Serial.print("Humidity: "); Serial.println(h);

  if (t > tempThreshold) {
    blinkLED(2, 300);
    bot.sendMessage(CHAT_ID, "🌡️ *High Temperature Detected!*\nTemp: " + String(t) + "°C", "Markdown");
  }

  if (h > humidityThreshold) {
    blinkLED(3, 300);
    bot.sendMessage(CHAT_ID, "💧 *High Humidity Detected!*\nHumidity: " + String(h) + "%", "Markdown");
  }
}

// 3. Fall Detection (using sudden acceleration)
void checkFall() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float accTotal = sqrt(a.acceleration.x * a.acceleration.x +
                        a.acceleration.y * a.acceleration.y +
                        a.acceleration.z * a.acceleration.z);
  Serial.println("MPU-" + String(accTotal));

  if (accTotal > 16.0) 
  {  // Adjust sensitivity if needed
    if (!fallDetected) 
    {
      fallDetected = true;
      bot.sendMessage(CHAT_ID, "🆘 *Fall Detected!* Please check immediately.", "Markdown");
    }
  } 
  else 
  {
    fallDetected = false;
  }
}

// 4. Emergency Button
void checkButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (millis() - lastButtonPress > 2000) {
      lastButtonPress = millis();
      bot.sendMessage(CHAT_ID, "🚨 *Emergency Distress Signal Activated!*", "Markdown");
    }
  }
}
