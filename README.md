<div align="center">

<img src="docs/water.svg" width="840" alt="" />

# Project HYDRA

**A sewage chamber that tells you before it overflows.**

Ultrasonic depth, methane in the headspace, and a dashboard the microcontroller serves itself. No cloud, no broker, no subscription.

<p>
  <img alt="Board" src="https://img.shields.io/badge/ESP8266-NodeMCU-1c1c1e?style=flat-square&logo=espressif&logoColor=E7352C" />
  <img alt="Language" src="https://img.shields.io/badge/Arduino-C%2B%2B-1c1c1e?style=flat-square&logo=arduino&logoColor=00979D" />
  <img alt="Sensors" src="https://img.shields.io/badge/HC--SR04-%2B%20MQ--4-1c1c1e?style=flat-square" />
  <img alt="Cloud" src="https://img.shields.io/badge/cloud-none-1c1c1e?style=flat-square" />
</p>

</div>

---

Mount an ultrasonic sensor in the roof of a tank. It pings the water surface four times a second, converts echo time to a fill percentage, and shows it on an OLED. An MQ-4 alongside watches for methane. A buzzer sounds when either crosses its line — and the water threshold drops on its own when rain is forecast, so a chamber that's merely full gets flagged *before* the runoff arrives.

The dashboard isn't a separate app. The ESP8266 serves the whole thing — HTML, CSS, JS, icons — out of `PROGMEM`, so anything with a browser on the same network is a client.

## Read this before you flash it

**The dashboard shows six nodes; one is real.** `KR Puram Chamber` reflects your actual sensors. The other five are simulated so the multi-node layout and map have something to display.

**There's an OpenWeatherMap key committed in `hydra.html`.** It needs rotating and moving out of tracked source. Wi-Fi credentials are handled properly via a gitignored `secrets.h`; the weather key isn't, yet.

## Hardware

| Part | Detail |
|---|---|
| Microcontroller | ESP8266 — NodeMCU or Wemos D1 Mini |
| Level | HC-SR04 ultrasonic |
| Gas | MQ-4 methane |
| Display | SH1106 128×64 I²C OLED — optional |
| Buzzer | Any active 3.3–5 V |

| ESP8266 | Connects to |
|---|---|
| D6 · D7 | HC-SR04 `TRIG` · `ECHO` |
| D5 | Buzzer `+` |
| A0 | MQ-4 `AO` |
| D1 · D2 | OLED `SCL` · `SDA` |

> **The MQ-4's analog output can swing to ~5 V; the ESP8266's A0 tops out near 3.3 V.** Put a divider on that line, or use a module with one built in. This is the connection that quietly kills boards.

The MQ-4 also needs a burn-in before readings settle — hours on first power-up. Early numbers drift high; that's the sensor, not the code.

## Flashing it

Arduino IDE with the ESP8266 board package. Libraries: `Adafruit_SH110X`, `Adafruit_GFX`, `ArduinoJson`.

```bash
cp secrets.h.example secrets.h     # then fill in ssid / password
```

`secrets.h` is gitignored. Upload, read the IP from Serial at 115200, and open `http://<that-ip>/`.

## How it decides

**It measures air, not water.** The sensor reports distance to the surface, so the reading gets *smaller* as the tank fills. `map(d, 30, 2, 0, 100)` inverts it. Both ends are configurable via `TANK_EMPTY_CM` and `TANK_FULL_CM` — measure your own chamber, because those two numbers are the calibration.

**Three pings, middle one wins.** Ultrasonic sensors throw the occasional wild value — a stray echo, a splash, some foam. A median of three readings 4 ms apart discards the outlier without the lag a running average adds.

**Both alerts have a deadband.** Water on at 80%, off below 75%. Methane on at 1000 ppm, off below 900. Without the gap, a chamber resting exactly at threshold stutters the buzzer on every ripple.

**Methane comes off the datasheet curve** — `ppm = 1012.7 · (Rs/Ro)^-2.786`, with several Rs samples averaged so one noisy ADC read can't spike it.

**The threshold follows the weather.** The dashboard scans the next four 3-hour forecast blocks for rain, drizzle, or thunderstorms; if any hit, the alert level drops from 80% to 60%. A chamber at 65% is fine on a clear day and a problem an hour before a downpour.

## Endpoints

| | |
|---|---|
| `GET /` | The whole dashboard, from `PROGMEM` |
| `GET /level` | Fill percentage and raw distance |
| `GET /gas` | Methane estimate in ppm |

The page polls `/level` every ~0.8 s and `/gas` every second. If the ESP is unreachable it falls back to simulated data for the live node — convenient for demos, worth remembering when you're debugging and the numbers look suspiciously healthy.

## Tuning

All `#define`s at the top of `water.ino`:

| Constant | Default | Meaning |
|---|---|---|
| `TANK_EMPTY_CM` · `TANK_FULL_CM` | `30` · `2` | Readings meaning 0% and 100%. **Measure yours** |
| `ALERT_PERCENT` · `ALERT_RELEASE` | `80` · `75` | Water buzzer on / off |
| `GAS_ALERT_PPM` · `GAS_RELEASE_PPM` | `1000` · `900` | Methane on / off |
| `SENSOR_INTERVAL` · `GAS_INTERVAL` | `250` · `1000` ms | Sampling rates |
| `MQ4_CURVE_A` · `MQ4_CURVE_B` | `1012.7` · `-2.786` | The fitted ppm curve |

## On the OLED

Fill percentage, a level bar, live CH₄ with a status word, and a Wi-Fi indicator reading `----` when the connection drops. Refreshed every 300 ms — slower than the sensor, because redrawing an I²C display is the most expensive thing in the loop. The board keeps sensing and alerting with the display disconnected; it's genuinely optional.

## Known limits

One real node. The weather key is client-side, so it's visible to anyone loading the dashboard — proxying the forecast through the ESP is the fix. No history: the trend chart is a 60-second window that resets on reload. No authentication on any endpoint, which is fine on a home network and not fine port-forwarded. And the MQ-4 is uncalibrated in absolute terms — good for "methane is rising", not for a number you'd report.
