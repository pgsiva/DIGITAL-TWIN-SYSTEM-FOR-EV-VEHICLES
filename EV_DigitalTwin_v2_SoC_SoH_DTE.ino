/*
  ============================================================================
  Cloud Digital Twin for EV Battery SoC, SoH and DTE Estimation - Version 2
  ============================================================================
  MONITORING SYSTEM ONLY.
  No battery switching. No relay control. No MOSFET control. No charging control.

  Hardware:
    - ESP32 Dev Module
    - ADS1115 (I2C ADC)   -> 4 battery module voltages (A0..A3)
    - INA219  (I2C)       -> bus voltage, load voltage, current, power
    - MCP9808 (I2C)       -> battery temperature
    - WiFi + ThingSpeak   -> cloud upload

  I2C Wiring:
    ESP32 GPIO21 -> SDA (all sensors)
    ESP32 GPIO22 -> SCL (all sensors)
    ESP32 3.3V   -> VCC/VIN of ADS1115, INA219, MCP9808
    ESP32 GND    -> GND of all sensors

  ADS1115 Wiring:
    A0 -> Module 1 voltage divider (220k / 22k, factor = 11)
    A1 -> Module 2 voltage divider
    A2 -> Module 3 voltage divider
    A3 -> Module 4 voltage divider

  Battery Pack (Version 1 reference):
    4 modules x 12V nominal  (12.6V max, 9.0V min per module)
    Total pack: 48V nominal
  ============================================================================
*/

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_INA219.h>
#include <Adafruit_MCP9808.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ============================================================================
// WIFI / THINGSPEAK CONFIG
// ============================================================================
const char* ssid     = "xyz";
const char* password = "12345678";
String apiKey = "M76PKKNXBO0M6OI8";   // ThingSpeak WRITE API key

// ============================================================================
// SENSOR OBJECTS
// ============================================================================
Adafruit_ADS1115 ads;
Adafruit_INA219 ina219;
Adafruit_MCP9808 tempsensor;

bool adsOK  = false;
bool inaOK  = false;
bool mcpOK  = false;

// ============================================================================
// VOLTAGE DIVIDER
// ============================================================================
const float R_TOP    = 220000.0;   // 220k
const float R_BOTTOM = 22000.0;    // 22k
const float DIVIDER_FACTOR = (R_TOP + R_BOTTOM) / R_BOTTOM;  // = 11.0

// ============================================================================
// BATTERY PACK PARAMETERS (editable)
// ============================================================================
const float MODULE_MAX_VOLTAGE = 12.6;   // 100% SoC per module
const float MODULE_MIN_VOLTAGE = 9.0;    // 0% SoC per module
const int   NUM_MODULES = 4;
const float NOMINAL_PACK_VOLTAGE = 48.0; // 4 x 12V

// DTE / capacity model (editable to match real pack + vehicle)
float BATTERY_CAPACITY_AH        = 20.0;   // total pack capacity, Ah
float VEHICLE_EFFICIENCY_WH_PER_KM = 20.0; // Wh consumed per km travelled

// SoH model inputs (editable / replaceable by MATLAB digital twin later)
unsigned long cycleCount = 0;              // placeholder - update from your cycle logic
const float RATED_CYCLE_LIFE = 500.0;      // cycles until ~80% SoH (typical Li-ion)
const float TEMP_STRESS_LOW  = 0.0;        // below this, degradation accelerates
const float TEMP_STRESS_HIGH = 45.0;       // above this, degradation accelerates

// ============================================================================
// SoC LOOKUP TABLE (voltage -> % charge), per module, high to low
// ============================================================================
const int SOC_TABLE_SIZE = 11;
float socVoltageTable[SOC_TABLE_SIZE] = {12.6, 12.4, 12.2, 12.0, 11.8, 11.6, 11.4, 11.2, 11.0, 10.5, 9.0};
float socPercentTable[SOC_TABLE_SIZE] = {100,  90,   80,   70,   60,   50,   40,   30,   20,   10,   0};

// ============================================================================
// LIVE DATA STRUCTURE
// ============================================================================
struct BatteryData {
  float moduleVoltage[4] = {0, 0, 0, 0};
  float totalPackVoltage = 0;

  float busVoltage    = 0;
  float shuntVoltage_mV = 0;
  float loadVoltage   = 0;
  float current_mA    = 0;
  float power_mW      = 0;

  float temperatureC  = 0;

  float soc = 0;   // %
  float soh = 100; // %
  float dte = 0;   // km

  bool wifiConnected = false;
  int  lastHttpCode  = 0;
};

BatteryData battery;

// ============================================================================
// TIMING (no delay() except the mandatory ThingSpeak interval gate)
// ============================================================================
unsigned long lastUpload  = 0;
const unsigned long uploadInterval = 15000;   // ThingSpeak minimum = 15 sec

unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 5000;

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  Wire.begin(21, 22);

  wifiConnect();

  // ---- ADS1115 ----
  adsOK = ads.begin();
  if (!adsOK) {
    Serial.println("[ERROR] ADS1115 NOT FOUND - check wiring (0x48 default)");
  } else {
    ads.setGain(GAIN_ONE);   // +-4.096V range
    Serial.println("[OK] ADS1115 initialized");
  }

  // ---- INA219 ----
  inaOK = ina219.begin();
  if (!inaOK) {
    Serial.println("[ERROR] INA219 NOT FOUND - check wiring (0x40 default)");
  } else {
    Serial.println("[OK] INA219 initialized");
  }

  // ---- MCP9808 ----
  mcpOK = tempsensor.begin(0x18);
  if (!mcpOK) {
    Serial.println("[ERROR] MCP9808 NOT FOUND - check wiring (0x18 default)");
  } else {
    tempsensor.setResolution(3);
    Serial.println("[OK] MCP9808 initialized");
  }

  Serial.println("====================================================");
  Serial.println(" EV Battery Digital Twin v2 - Monitoring Started");
  Serial.println("====================================================");
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {

  wifiReconnect();

  readADS();
  readINA219();
  readTemperature();

  calculateSOC();
  calculateSOH();
  calculateDTE();

  printSerial();

  if (millis() - lastUpload >= uploadInterval) {
    lastUpload = millis();
    uploadThingSpeak();
  }

  delay(1000); // display refresh only, not used for ThingSpeak timing
}

// ============================================================================
// WIFI FUNCTIONS
// ============================================================================
void wifiConnect() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(400);
    Serial.print(".");
  }
  battery.wifiConnected = (WiFi.status() == WL_CONNECTED);
  Serial.println();
  if (battery.wifiConnected) {
    Serial.print("[OK] WiFi Connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[ERROR] WiFi connection failed, will retry in background");
  }
}

void wifiReconnect() {
  if (millis() - lastWifiCheck < wifiCheckInterval) return;
  lastWifiCheck = millis();

  if (WiFi.status() != WL_CONNECTED) {
    battery.wifiConnected = false;
    Serial.println("[WARN] WiFi disconnected - attempting reconnect...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  } else {
    battery.wifiConnected = true;
  }
}

// ============================================================================
// SENSOR READ FUNCTIONS
// ============================================================================
void readADS() {
  if (!adsOK) return;

  for (int i = 0; i < NUM_MODULES; i++) {
    int16_t raw = ads.readADC_SingleEnded(i);
    float v_adc = ads.computeVolts(raw);
    float v_module = v_adc * DIVIDER_FACTOR;

    // ADC out-of-range check
    if (v_module < 0 || v_module > 15.0) {
      Serial.printf("[WARN] Module %d voltage out of expected range: %.2f V\n", i + 1, v_module);
    }
    battery.moduleVoltage[i] = v_module;
  }

  battery.totalPackVoltage = battery.moduleVoltage[0] + battery.moduleVoltage[1] +
                              battery.moduleVoltage[2] + battery.moduleVoltage[3];
}

void readINA219() {
  if (!inaOK) return;

  battery.busVoltage      = ina219.getBusVoltage_V();
  battery.shuntVoltage_mV = ina219.getShuntVoltage_mV();
  battery.current_mA      = ina219.getCurrent_mA();
  battery.power_mW        = ina219.getPower_mW();
  battery.loadVoltage      = battery.busVoltage + (battery.shuntVoltage_mV / 1000.0);
}

void readTemperature() {
  if (!mcpOK) return;
  battery.temperatureC = tempsensor.readTempC();
}

// ============================================================================
// SOC CALCULATION (voltage lookup + linear interpolation, averaged over modules)
// ============================================================================
float interpolateSOC(float voltage) {
  if (voltage >= socVoltageTable[0]) return 100.0;
  if (voltage <= socVoltageTable[SOC_TABLE_SIZE - 1]) return 0.0;

  for (int i = 0; i < SOC_TABLE_SIZE - 1; i++) {
    float vHigh = socVoltageTable[i];
    float vLow  = socVoltageTable[i + 1];

    if (voltage <= vHigh && voltage >= vLow) {
      float socHigh = socPercentTable[i];
      float socLow  = socPercentTable[i + 1];
      // linear interpolation between the two nearest table points
      float ratio = (voltage - vLow) / (vHigh - vLow);
      return socLow + ratio * (socHigh - socLow);
    }
  }
  return 0.0;
}

void calculateSOC() {
  float total = 0;
  for (int i = 0; i < NUM_MODULES; i++) {
    total += interpolateSOC(battery.moduleVoltage[i]);
  }
  battery.soc = total / NUM_MODULES;
}

// ============================================================================
// SOH ESTIMATION (modular - replace body with MATLAB Digital Twin model later)
// Inputs: voltage spread across modules, current, temperature, cycle count
// ============================================================================
void calculateSOH() {
  float soh = 100.0;

  // 1) Cycle-life based degradation
  float cycleDegradation = (cycleCount / RATED_CYCLE_LIFE) * 20.0; // ~20% loss at rated life
  soh -= cycleDegradation;

  // 2) Temperature stress degradation
  if (battery.temperatureC > TEMP_STRESS_HIGH || battery.temperatureC < TEMP_STRESS_LOW) {
    soh -= 5.0; // flat penalty while operating outside safe thermal band
  }

  // 3) Module voltage imbalance degradation (higher spread = more aged/weaker cells)
  float vMax = battery.moduleVoltage[0], vMin = battery.moduleVoltage[0];
  for (int i = 1; i < NUM_MODULES; i++) {
    if (battery.moduleVoltage[i] > vMax) vMax = battery.moduleVoltage[i];
    if (battery.moduleVoltage[i] < vMin) vMin = battery.moduleVoltage[i];
  }
  float imbalance = vMax - vMin;
  soh -= imbalance * 10.0; // penalize spread between weakest and strongest module

  if (soh < 0) soh = 0;
  if (soh > 100) soh = 100;

  battery.soh = soh;
}

// ============================================================================
// DTE CALCULATION (Distance To Empty)
// Remaining energy (Wh) = Capacity(Ah) x Nominal Voltage x (SoC/100) x (SoH/100)
// DTE (km) = Remaining energy (Wh) / Vehicle efficiency (Wh/km)
// ============================================================================
void calculateDTE() {
  float remainingEnergyWh = BATTERY_CAPACITY_AH * NOMINAL_PACK_VOLTAGE *
                             (battery.soc / 100.0) * (battery.soh / 100.0);

  if (VEHICLE_EFFICIENCY_WH_PER_KM <= 0) {
    battery.dte = 0;
    return;
  }

  battery.dte = remainingEnergyWh / VEHICLE_EFFICIENCY_WH_PER_KM;
}

// ============================================================================
// SERIAL PRINT
// ============================================================================
void printSerial() {
  Serial.println();
  Serial.println("================== BATTERY DIGITAL TWIN ==================");
  Serial.printf("Module 1 Voltage   : %.2f V\n", battery.moduleVoltage[0]);
  Serial.printf("Module 2 Voltage   : %.2f V\n", battery.moduleVoltage[1]);
  Serial.printf("Module 3 Voltage   : %.2f V\n", battery.moduleVoltage[2]);
  Serial.printf("Module 4 Voltage   : %.2f V\n", battery.moduleVoltage[3]);
  Serial.printf("Total Pack Voltage : %.2f V\n", battery.totalPackVoltage);
  Serial.println("------------------------------------------------------------");
  Serial.printf("Bus Voltage        : %.2f V\n", battery.busVoltage);
  Serial.printf("Load Voltage       : %.2f V\n", battery.loadVoltage);
  Serial.printf("Current            : %.2f mA\n", battery.current_mA);
  Serial.printf("Power              : %.2f mW\n", battery.power_mW);
  Serial.printf("Temperature        : %.2f C\n", battery.temperatureC);
  Serial.println("------------------------------------------------------------");
  Serial.printf("SoC (State of Charge) : %.1f %%\n", battery.soc);
  Serial.printf("SoH (State of Health)  : %.1f %%\n", battery.soh);
  Serial.printf("DTE (Distance To Empty): %.1f km\n", battery.dte);
  Serial.println("------------------------------------------------------------");
  Serial.printf("WiFi Status        : %s\n", battery.wifiConnected ? "CONNECTED" : "DISCONNECTED");
  Serial.printf("Last Upload Code   : %d\n", battery.lastHttpCode);
  Serial.printf("Timestamp (millis) : %lu ms\n", millis());
  Serial.println("=============================================================");
}

// ============================================================================
// THINGSPEAK UPLOAD
// ============================================================================
void uploadThingSpeak() {
  if (!battery.wifiConnected || WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] Upload skipped - WiFi not connected");
    return;
  }

  String url = "https://api.thingspeak.com/update?api_key=" + apiKey +
               "&field1=" + String(battery.moduleVoltage[0]) +
               "&field2=" + String(battery.moduleVoltage[1]) +
               "&field3=" + String(battery.moduleVoltage[2]) +
               "&field4=" + String(battery.moduleVoltage[3]) +
               "&field5=" + String(battery.current_mA) +
               "&field6=" + String(battery.temperatureC) +
               "&field7=" + String(battery.soc) +
               "&field8=" + String(battery.dte);

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  http.end();

  battery.lastHttpCode = httpCode;

  if (httpCode == 200) {
    Serial.println("[OK] ThingSpeak upload successful");
  } else {
    Serial.printf("[ERROR] ThingSpeak upload failed, code: %d\n", httpCode);
  }
}
