<div align="center">

<img src="docs/water.svg" width="840" alt="" />

# Project HYDRA

**Real-time water level and methane monitoring for sewage chambers, on the ESP8266.**

Ultrasonic depth sensing, MQ-4 gas detection, and a multi-node dashboard served directly from the microcontroller. No cloud service, message broker, or subscription.

<p>
  <img alt="Board" src="https://img.shields.io/badge/ESP8266-NodeMCU-1c1c1e?style=flat-square&logo=espressif&logoColor=E7352C" />
  <img alt="Language" src="https://img.shields.io/badge/Arduino-C%2B%2B-1c1c1e?style=flat-square&logo=arduino&logoColor=00979D" />
  <img alt="Sensors" src="https://img.shields.io/badge/HC--SR04-%2B%20MQ--4-1c1c1e?style=flat-square" />
  <img alt="Cloud" src="https://img.shields.io/badge/cloud-none-1c1c1e?style=flat-square" />
</p>

</div>

---

## Overview

An ultrasonic sensor mounted in the roof of a chamber measures the distance to the water surface four times per second and converts it to a fill percentage, displayed on an onboard OLED. An MQ-4 sensor monitors methane concentration in the headspace above it. A buzzer sounds when either measurement crosses its threshold, and the water threshold is lowered automatically when rain is forecast — so a chamber that is merely full is flagged before runoff arrives rather than after.

The dashboard is not a separate application. The ESP8266 serves the complete interface — markup, styles, scripts, and icons — from `PROGMEM`, making any browser on the same network a client.

## Before deployment

**The dashboard displays six nodes; one is instrumented.** `KR Puram Chamber` reflects the sensors attached to your board. The remaining five are simulated so that the multi-node overview and map have content to display.

**An OpenWeatherMap API key is currently committed in `hydra.html`.** It requires rotation and relocation out of tracked source. Wi-Fi credentials are handled correctly through a gitignored `secrets.h`; the weather key is not yet.

## Hardware

| Component | Specification |
|---|---|
| Microcontroller | ESP8266 — NodeMCU or Wemos D1 Mini |
| Level sensor | HC-SR04 ultrasonic |
| Gas sensor | MQ-4 methane / natural gas |
| Display | SH1106 128×64 I²C OLED (optional) |
| Buzzer | Active, 3.3–5 V |
| Power | USB or a 5 V supply |

### Wiring

| ESP8266 pin | Connection |
|---|---|
| D6 (GPIO 12) | HC-SR04 `TRIG` |
| D7 (GPIO 13) | HC-SR04 `ECHO` |
| D5 (GPIO 14) | Buzzer `+` |
| A0 | MQ-4 `AO` |
| D1 · D2 | OLED `SCL` · `SDA` |

> **The MQ-4 analog output can reach approximately 5 V, while the ESP8266 `A0` input is rated to 3.3 V.** Fit a voltage divider on this line, or use a module with one integrated. This connection is the most common cause of board damage in this configuration.

The MQ-4 also requires a burn-in period before readings stabilise — several hours on first power-up, and a few minutes subsequently. Early readings are elevated; this is a property of the sensor rather than the firmware.

## Installation

1. Arduino IDE with the ESP8266 board package installed.
2. Libraries: `Adafruit_SH110X`, `Adafruit_GFX`, `ArduinoJson`.
3. Credentials — copy the template rather than editing tracked files:

```sh
cp secrets.h.example secrets.h
```

```c
const char* ssid     = "your-network";
const char* password = "your-password";
```

`secrets.h` is gitignored.

4. Select the board, upload, and read the assigned IP address from the Serial Monitor at 115200 baud.
5. Open `http://<address>/` from any device on the same network.

## Measurement

**The sensor measures air, not water.** It reports the distance to the surface, so the reading decreases as the chamber fills. `map(d, 30, 2, 0, 100)` inverts this: 30 cm of air corresponds to empty, 2 cm to full. Both bounds are configurable through `TANK_EMPTY_CM` and `TANK_FULL_CM`, and these two values constitute the calibration for a given chamber.

**Three pings, median selected.** Ultrasonic sensors return occasional outliers caused by stray echoes, surface disturbance, or foam. Taking the median of three readings 4 ms apart discards the outlier without introducing the lag of a running average.

**Both alerts implement hysteresis.** The water alarm activates at 80% and clears below 75%; the methane alarm activates at 1000 ppm and clears below 900. Without this separation, a chamber resting at the threshold causes the buzzer to oscillate on every surface ripple.

**Methane concentration is derived from the datasheet curve.** The sensor resistance ratio maps to ppm through `ppm = 1012.7 · (Rs/Ro)^-2.786`, fitted to the CH₄ response curve, with several samples averaged so that a single noisy ADC reading cannot produce a spike.

**The water threshold responds to forecast conditions.** The dashboard retrieves a local forecast and examines the next four three-hour intervals for rain, drizzle, or thunderstorms. If any are present, the alert threshold is reduced from 80% to 60% and the status badge changes accordingly.

## HTTP interface

| Endpoint | Response |
|---|---|
| `GET /` | The complete dashboard, from `PROGMEM` |
| `GET /level` | Current fill percentage and raw distance |
| `GET /gas` | Current methane estimate in ppm |

The page polls `/level` approximately every 0.8 s and `/gas` every second. If the board is unreachable, the dashboard substitutes simulated data for the live node rather than displaying an error — convenient for demonstration, and worth noting when debugging.

## Configuration

All values are `#define` constants at the top of `water.ino`.

| Constant | Default | Description |
|---|---|---|
| `TANK_EMPTY_CM` · `TANK_FULL_CM` | `30` · `2` | Distances corresponding to 0% and 100%. **Measure per chamber** |
| `ALERT_PERCENT` · `ALERT_RELEASE` | `80` · `75` | Water alarm activation and clearance |
| `GAS_ALERT_PPM` · `GAS_RELEASE_PPM` | `1000` · `900` | Methane alarm activation and clearance |
| `GAS_WARN_PPM` | `100` | Elevated indication on the display; no alarm |
| `SENSOR_INTERVAL` · `GAS_INTERVAL` | `250` · `1000` ms | Sampling intervals |
| `MQ4_RL_KOHM` · `MQ4_CLEAN_AIR` | `10.0` · `4.4` | Load resistance and clean-air Rs/Ro from the datasheet |
| `MQ4_CURVE_A` · `MQ4_CURVE_B` | `1012.7` · `-2.786` | Fitted ppm curve coefficients |

## Display

The OLED shows fill percentage, a level bar, live methane concentration with a status label, and a Wi-Fi indicator that reads `----` when the connection is lost. It refreshes every 300 ms — slower than the sensor sampling rate, because redrawing an I²C display is the most expensive operation in the main loop.

The board continues to sample and alert with the display disconnected. It is genuinely optional.

## Project structure

```
hydra/
├── water.ino            Firmware — sensing, display, web server, alerts
├── hydra.html           Dashboard, embedded into the firmware
├── secrets.h.example    Wi-Fi credential template
├── hydra_icon.svg
└── oled_mockup.svg
```

## Limitations

- **One instrumented node.** The remaining five are simulated for layout purposes.
- **The weather API key is client-side.** The dashboard executes in the browser, so any key it uses is visible to anyone loading the page. Proxying the forecast request through the ESP8266 is the appropriate fix.
- **No historical storage.** Readings are not persisted; the trend chart is a 60-second rolling window in the page and resets on reload.
- **No authentication.** Acceptable on a private network, unsuitable for a port-forwarded deployment.
- **The MQ-4 is uncalibrated in absolute terms.** Readings are meaningful as a trend, not as a reportable measurement.

## Contributors

| | |
|---|---|
| [chakri192](https://github.com/chakri192) | Author |
| [aider](https://github.com/Aider-AI/aider) | AI pair programmer |
