#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_INA219.h>
#include <Adafruit_MCP9808.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ------------ WiFi Details ---------------
const char* ssid = "sam1";
const char* password = "12345678";

// ------------ ThingSpeak -----------------
const char* apiKey = "M76PKKNXBO0M6OI8";

// ------------ Sensor Objects -------------
Adafruit_ADS1115 ads;
Adafruit_INA219 ina219;
Adafruit_MCP9808 tempsensor;

// -------- Voltage Divider Values ---------
const float Rtop = 200000.0;
const float Rbottom = 20000.0;
const float DIVIDER_FACTOR = (Rtop + Rbottom) / Rbottom;

void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21,22);

  connectWiFi();

  Serial.println();
  Serial.println("===== EV Battery Sensor Test =====");

  if(!ads.begin())
  {
    Serial.println("ADS1115 NOT FOUND!");
    while(1);
  }

  ads.setGain(GAIN_ONE);
  Serial.println("ADS1115 OK");

  if(!ina219.begin())
  {
    Serial.println("INA219 NOT FOUND!");
    while(1);
  }

  Serial.println("INA219 OK");

  if(!tempsensor.begin(0x18))
  {
    Serial.println("MCP9808 NOT FOUND!");
    while(1);
  }

  tempsensor.setResolution(3);

  Serial.println("MCP9808 OK");
  Serial.println("==============================");
}

void loop()
{
  if(WiFi.status()!=WL_CONNECTED)
  {
    connectWiFi();
  }

  // -------- ADS1115 --------
  int16_t raw = ads.readADC_SingleEnded(0);
  float adcVoltage = ads.computeVolts(raw);
  float batteryVoltage = adcVoltage * DIVIDER_FACTOR;

  // -------- INA219 --------
  float busVoltage = ina219.getBusVoltage_V();
  float shuntVoltage = ina219.getShuntVoltage_mV();
  float current = ina219.getCurrent_mA();
  float power = ina219.getPower_mW();
  float loadVoltage = busVoltage + (shuntVoltage / 1000.0);

  // -------- MCP9808 --------
  float temperature = tempsensor.readTempC();

  // -------- Serial Monitor --------
  Serial.println();
  Serial.println("========== SENSOR READINGS ==========");

  Serial.print("Battery Voltage : ");
  Serial.print(batteryVoltage,2);
  Serial.println(" V");

  Serial.print("Bus Voltage     : ");
  Serial.print(busVoltage,2);
  Serial.println(" V");

  Serial.print("Load Voltage    : ");
  Serial.print(loadVoltage,2);
  Serial.println(" V");

  Serial.print("Current         : ");
  Serial.print(current,2);
  Serial.println(" mA");

  Serial.print("Power           : ");
  Serial.print(power,2);
  Serial.println(" mW");

  Serial.print("Temperature     : ");
  Serial.print(temperature,2);
  Serial.println(" C");

  Serial.println("=====================================");

  // -------- ThingSpeak Upload --------
  if(WiFi.status()==WL_CONNECTED)
  {
    HTTPClient http;

    String url =
      "https://api.thingspeak.com/update?api_key=" +
      String(apiKey) +
      "&field1=" + String(batteryVoltage,2) +
      "&field2=" + String(busVoltage,2) +
      "&field3=" + String(loadVoltage,2) +
      "&field4=" + String(current,2) +
      "&field5=" + String(power,2) +
      "&field6=" + String(temperature,2);

    http.begin(url);

    int httpCode = http.GET();

    if(httpCode > 0)
    {
      String response = http.getString();

      Serial.print("ThingSpeak Entry ID : ");
      Serial.println(response);
    }
    else
    {
      Serial.print("HTTP Error : ");
      Serial.println(httpCode);
    }

    http.end();
  }

  // ThingSpeak Free Account = 15 seconds minimum
  delay(15000);
}