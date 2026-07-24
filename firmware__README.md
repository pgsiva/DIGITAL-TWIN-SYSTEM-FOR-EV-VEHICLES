# Firmware - EV Digital Twin Battery Telemetry

ESP32 sketch that reads pack voltage, current, power and temperature, derives
SOC / SOH / DTE, and uploads the readings to ThingSpeak every 15 seconds.

## Bill of materials (Version 1 - 12V prototype)

| Component                | Qty | Notes                                    |
|---------------------------|-----|-------------------------------------------|
| ESP32 Dev Board            | 1   | Wi-Fi + I2C master                        |
| ADS1115 16-bit ADC          | 1   | Reads divided pack voltage on A0          |
| INA219 current/power sensor | 1   | In series between pack+ and load          |
| MCP9808 temperature sensor  | 1   | I2C, address 0x18                         |
| 200k&#8486; resistor          | 1   | Voltage divider top (R-top)               |
| 20k&#8486; resistor           | 1   | Voltage divider bottom (R-bottom)          |
| 100nF capacitor (optional)  | 1   | Noise filter across R-bottom               |
| 5.1V zener diode (optional) | 1   | ADC input protection, A0 to GND            |

See [`../docs/diagrams/circuit-wiring-v1.svg`](../docs/diagrams/circuit-wiring-v1.svg)
for the full wiring diagram.

## Bill of materials (Version 2 - 44.4V, 4-pack scale-up)

| Component            | Qty | Notes                                        |
|------------------------|-----|-------------------------------------------------|
| 11.1V (3S) sub-packs     | 4   | Series-connected to form the 44.4V bus         |
| D4184 MOSFET module       | 3   | One between each adjacent pack, fault isolation |
| 220k&#8486; resistor        | 4   | Voltage divider top, one per pack               |
| 22k&#8486; resistor         | 4   | Voltage divider bottom, one per pack             |
| ADS1115                | 1   | 4 channels (A0-A3), one tap per pack             |
| ACS758 current sensor    | 1   | Replaces INA219 above ~26V bus                   |

See [`../docs/diagrams/battery-pack-scaling-v2.svg`](../docs/diagrams/battery-pack-scaling-v2.svg).

> INA219's maximum bus voltage is ~26V, so it is only valid for the 12V
> single-pack prototype. The 44.4V version needs an isolated sensor such as
> the ACS758 or an INA226 behind proper isolation.

## Arduino libraries required

Install these via the Arduino Library Manager:

- `Adafruit ADS1X15`
- `Adafruit INA219`
- `Adafruit MCP9808 Library`
- ESP32 board support package (`esp32` by Espressif Systems)

## Setup

1. Copy `config.example.h` to `config.h` in this same folder.
2. Fill in your Wi-Fi SSID/password and your ThingSpeak **Write** API key.
3. Open `EV_Digital_Twin_Firmware.ino` in the Arduino IDE, select **ESP32 Dev
   Module** as the board, and upload.
4. Open the Serial Monitor at `115200` baud to confirm sensor readings.

`config.h` is listed in `.gitignore` so your real credentials are never
committed.

## ThingSpeak channel fields

| Field  | Value            |
|--------|------------------|
| field1 | Battery Voltage (V) |
| field2 | Current (mA)      |
| field3 | Power (mW)        |
| field4 | Temperature (C)  |
| field5 | SOC (%)           |
| field6 | SOH (%)           |
| field7 | DTE (km)          |
