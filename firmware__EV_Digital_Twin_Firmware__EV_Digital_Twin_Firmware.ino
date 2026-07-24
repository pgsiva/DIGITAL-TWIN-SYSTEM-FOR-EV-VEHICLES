/*
  ===========================================================================
  EV Digital Twin - Battery Telemetry Firmware
  Board   : ESP32 Dev Module
  Sensors : ADS1115 (pack voltage via divider), INA219 (current/power),
            MCP9808 (pack temperature)
  Cloud   : ThingSpeak REST API
  ===========================================================================
  Credentials and per-deployment constants live in config.h (gitignored).
  Copy config.example.h -> config.h and fill in your own values before
  compiling. Never commit a real Wi-Fi password or ThingSpeak Write API Key.
  ===========================================================================
*/

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_INA219.h>
#include <Adafruit_MCP9808.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "config.h"   // WIFI_SSID, WIFI_PASSWORD, THINGSPEAK_API_KEY live here

// ------------------------- Sensor objects ---------------------------------
Adafruit_ADS1115 ads;
Adafruit_INA219 ina219;
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

// ------------------- Voltage divider (Version 1 - 12V pack) ---------------
// Battery+ --[Rtop]-- ADS1115 A0 --[Rbottom]-- Battery- / GND
const float Rtop            = 200000.0;   // 200k ohm
const float Rbottom         = 20000.0;    // 20k ohm
const float DIVIDER_FACTOR  = (Rtop + Rbottom) / Rbottom;

// --------------------------- Battery parameters ----------------------------
float batteryCapacityAh   = 7.0;    // rated capacity (Ah)
float measuredCapacityAh  = 6.5;    // capacity measured from last full test (Ah)
float efficiencyWhPerKm   = 20.0;   // energy consumption estimate (Wh/km)

const unsigned long SEND_INTERVAL_MS = 15000; // ThingSpeak free-tier minimum

// ============================================================================
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed, will retry in loop().");
  }
}

// ============================================================================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // ESP32 SDA = GPIO21, SCL = GPIO22

  connectWiFi();

  if (!ads.begin()) {
    Serial.println("ADS1115 not found - check wiring");
    while (1) delay(1000);
  }
  ads.setGain(GAIN_ONE); // +-4.096V range, suitable for the 12V prototype divider

  if (!ina219.begin()) {
    Serial.println("INA219 not found - check wiring");
    while (1) delay(1000);
  }

  if (!tempsensor.begin(0x18)) {
    Serial.println("MCP9808 not found - check wiring");
    while (1) delay(1000);
  }
  tempsensor.setResolution(3);

  Serial.println("All sensors initialised. Starting telemetry loop.");
}

// ============================================================================
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // ---------------------- Pack voltage (ADS1115) ---------------------------
  int16_t raw          = ads.readADC_SingleEnded(0);
  float   v_mid         = ads.computeVolts(raw);
  float   batteryVoltage = v_mid * DIVIDER_FACTOR;

  // ---------------------- Current / power (INA219) --------------------------
  float busVoltage    = ina219.getBusVoltage_V();
  float shuntVoltage_mV = ina219.getShuntVoltage_mV();
  float current_mA    = ina219.getCurrent_mA();
  float power_mW      = ina219.getPower_mW();
  float loadVoltage   = busVoltage + (shuntVoltage_mV / 1000.0);

  // ---------------------- Temperature (MCP9808) ------------------------------
  float temperatureC = tempsensor.readTempC();

  // ---------------------- Derived digital-twin metrics -----------------------
  // State of Charge - linear approximation between empty (11.0V) and full (12.6V)
  float soc = ((batteryVoltage - 11.0) / (12.6 - 11.0)) * 100.0;
  soc = constrain(soc, 0, 100);

  // State of Health - measured usable capacity vs rated capacity
  float soh = (measuredCapacityAh / batteryCapacityAh) * 100.0;

  // Distance To Empty - remaining energy / consumption rate
  float totalEnergyWh     = 12.0 * batteryCapacityAh;
  float remainingEnergyWh = totalEnergyWh * (soc / 100.0);
  float dteKm             = remainingEnergyWh / efficiencyWhPerKm;

  // ---------------------- Serial debug output --------------------------------
  Serial.println("\n========= DIGITAL TWIN TELEMETRY =========");
  Serial.printf("Battery Voltage : %.2f V\n", batteryVoltage);
  Serial.printf("Load Voltage    : %.2f V\n", loadVoltage);
  Serial.printf("Current         : %.2f mA\n", current_mA);
  Serial.printf("Power           : %.2f mW\n", power_mW);
  Serial.printf("Temperature     : %.2f C\n", temperatureC);
  Serial.printf("SOC             : %.1f %%\n", soc);
  Serial.printf("SOH             : %.1f %%\n", soh);
  Serial.printf("DTE             : %.1f km\n", dteKm);
  Serial.println("===========================================");

  // ---------------------- Push to ThingSpeak ----------------------------------
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String("https://api.thingspeak.com/update?api_key=") + THINGSPEAK_API_KEY +
                 "&field1=" + String(batteryVoltage) +
                 "&field2=" + String(current_mA) +
                 "&field3=" + String(power_mW) +
                 "&field4=" + String(temperatureC) +
                 "&field5=" + String(soc) +
                 "&field6=" + String(soh) +
                 "&field7=" + String(dteKm);

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.println("ThingSpeak response: " + http.getString());
    } else {
      Serial.println("HTTP error: " + String(httpCode));
    }
    http.end();
  }

  delay(SEND_INTERVAL_MS);
}
