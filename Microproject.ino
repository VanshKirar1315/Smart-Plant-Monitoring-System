// ***** BLYNK TEMPLATE DETAILS *****
#define BLYNK_TEMPLATE_ID "TMPL3xdNVPJA9"
#define BLYNK_TEMPLATE_NAME "Smart Plant Monitoring System"
#define BLYNK_AUTH_TOKEN "LQwmSqih2Bxmrnm7njEu0pSFjT9Pe2d7"

// ***** LIBRARIES *****
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

// ***** LCD INITIALIZATION *****
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ***** WIFI CREDENTIALS *****
char ssid[] = "SPARK 7 Pro";    
char pass[] = "00001237";       

// ***** DHT SENSOR SETUP *****
#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ***** BLYNK TIMER *****
BlynkTimer timer;

// ***** COMPONENT PIN DEFINITIONS *****
#define SOIL_SENSOR A0
#define RELAY_PIN D1       // Changed from D3 → D1 (more stable GPIO)
#define BUTTON_PIN D7
#define PIR_PIN D5         

#define VPIN_BUTTON V12
#define VPIN_PIR_LED V5

int relayState = LOW;  // 0 = OFF, 1 = ON

// ***** SETUP FUNCTION *****
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();

  pinMode(SOIL_SENSOR, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PIR_PIN, INPUT);

  digitalWrite(RELAY_PIN, HIGH);  // Active LOW → OFF initially

  dht.begin();

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  // Connect WiFi first
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected");
  delay(1000);
  lcd.clear();

  // Then start Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass, "blynk.cloud", 80);

  // Timers
  timer.setInterval(2000L, readDHT);
  timer.setInterval(1500L, readSoil);
  timer.setInterval(500L, checkButton);
  timer.setInterval(1000L, PIRsensor);
}

// ***** READ DHT SENSOR *****
void readDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);

  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(t);
  lcd.print("C H:");
  lcd.print(h);
  lcd.print("% ");
}

// ***** READ SOIL SENSOR *****
void readSoil() {
  int value = analogRead(SOIL_SENSOR);
  value = map(value, 0, 1024, 100, 0); // 100% = wet, 0% = dry

  Blynk.virtualWrite(V3, value);

  lcd.setCursor(0, 1);
  lcd.print("Soil:");
  lcd.print(value);
  lcd.print("% ");
}

// ***** BUTTON + RELAY CONTROL *****
BLYNK_WRITE(VPIN_BUTTON) {
  relayState = param.asInt();
  digitalWrite(RELAY_PIN, !relayState); // Active LOW
  updateRelayOnLCD();
}

void checkButton() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState == LOW && lastButtonState == HIGH) {
    relayState = !relayState;
    digitalWrite(RELAY_PIN, !relayState);
    Blynk.virtualWrite(VPIN_BUTTON, relayState);
    updateRelayOnLCD();
    delay(300);
  }
  lastButtonState = currentButtonState;
}

void updateRelayOnLCD() {
  lcd.setCursor(11, 1);
  if (relayState == HIGH) {
    lcd.print("W:ON ");
  } else {
    lcd.print("W:OFF");
  }
}

// ***** PIR SENSOR FUNCTION *****
void PIRsensor() {
  bool value = digitalRead(PIR_PIN);
  WidgetLED LED(VPIN_PIR_LED);

  if (value) {
    Blynk.logEvent("pirmotion", "⚠️ Motion Detected!");
    LED.on();

    Serial.println("Motion Detected!");
    lcd.setCursor(0, 0);
    lcd.print("Motion: YES  ");
  } else {
    LED.off();
    Serial.println("No Motion");
    lcd.setCursor(0, 0);
    lcd.print("Motion: NO   ");
  }
}

// ***** MAIN LOOP *****
void loop() {
  Blynk.run();
  timer.run();
}
