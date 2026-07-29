# Project HYDRA

A real-time water/sewage level monitor built on the ESP8266. An ultrasonic sensor measures chamber depth every 250ms and an MQ-4 sensor tracks methane (CH₄), both shown on an onboard OLED and served to a live multi-node dashboard over Wi-Fi — buzzer alert included when the live node hits 80% fill (auto-lowers to 60% when rain is forecast) or methane exceeds 1000ppm.

**Live dashboard served at:** `http://<ESP-IP>/`

---

## Overview

Mount the ultrasonic sensor above your tank or chamber, add the MQ-4 methane sensor and an optional SH1106 OLED, flash the firmware, connect to Wi-Fi, and open the dashboard in any browser on your local network. No cloud, no subscriptions, no app — just your ESP8266 acting as its own web server. Only **one** node (`KR Puram Chamber`) is backed by real hardware; the other five nodes shown in the dashboard are simulated for layout/demo purposes (see the Dashboard section below).

---

## Features

- **Live water level** — ultrasonic reading every 250ms (median-of-3, non-blocking), polled by the dashboard roughly every 0.8s
- **Methane monitoring** — MQ-4 sensor sampled every 1s, converted to ppm via the datasheet log-log curve; served at `/gas`
- **Onboard OLED** — SH1106 128×64 display showing water fill %, a level bar, and live CH₄ ppm with a status label
- **Multi-node dashboard** — Overview, Nodes, and Map pages; only the first node (`krpuram`) reflects the real sensors, the rest are simulated
- **Trend chart** — 60-second rolling distance graph with hover tooltip and alert-threshold band
- **Dual buzzer alert** — rapid beep when the live node exceeds the water threshold *or* methane exceeds 1000ppm, each with hysteresis to prevent stutter
- **Weather-adaptive threshold** — pulls a Bengaluru forecast from OpenWeatherMap; if rain/thunderstorms are expected in the next ~12h, the alert threshold drops from 80% to 60%
- **Simulation mode** — dashboard auto-falls back to simulated data for the live node if the ESP is unreachable
- **Leaflet map** — node locations plotted on an OpenStreetMap/CARTO basemap
- **CSV export** — downloads a snapshot of all node readings
- **Dark / light mode** toggle
- **Zero dependencies** — single `.ino` file, HTML/CSS/JS embedded via `PROGMEM`

---

## Hardware

| Component | Detail |
|---|---|
| Microcontroller | ESP8266 (NodeMCU / Wemos D1 Mini) |
| Level sensor | HC-SR04 ultrasonic |
| Gas sensor | MQ-4 methane / natural-gas |
| Display | SH1106 128×64 I2C OLED (optional) |
| Buzzer | Active buzzer (any 3.3 / 5V) |
| Power | USB or 5V supply |

### Wiring

| ESP8266 Pin | Connected to |
|---|---|
| D6 (GPIO 12) | HC-SR04 TRIG |
| D7 (GPIO 13) | HC-SR04 ECHO |
| D5 (GPIO 14) | Buzzer + |
| A0 | MQ-4 AO (analog out) |
| D2 (GPIO 4) | OLED SDA |
| D1 (GPIO 5) | OLED SCL |
| GND | HC-SR04 GND, MQ-4 GND, OLED GND, Buzzer − |
| 5V / 3.3V | HC-SR04 VCC, MQ-4 VCC, OLED VCC |

> ⚠️ The MQ-4 AO can swing up to ~5V, but the ESP8266 A0 pin tolerates only ~1V (0–3.3V on some NodeMCU boards). Use a voltage divider / the board's onboard divider so AO stays within the ADC's safe range.

---

## Configuration

### Firmware settings

Edit the top of `water.ino` before flashing:

```cpp
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

#define TANK_EMPTY_CM  30   // distance (cm) when tank is empty
#define TANK_FULL_CM    2   // distance (cm) when tank is full
#define ALERT_PERCENT  80   // water buzzer turns ON at/above this fill %
#define ALERT_RELEASE  75   // ...and OFF once back below (hysteresis)

#define GAS_ALERT_PPM  1000 // methane buzzer ON at/above this ppm
#define GAS_RELEASE_PPM 900 // ...and OFF once back below (hysteresis)
#define GAS_WARN_PPM   100  // "elevated" band on the OLED
```

To get `TANK_EMPTY_CM` and `TANK_FULL_CM`: mount the sensor, open Serial Monitor at 115200 baud, and note the raw distance values at empty and full.

The MQ-4 self-calibrates its clean-air baseline (`Ro`) at boot and estimates ppm from a datasheet-fitted log-log curve (`MQ4_CURVE_A` / `MQ4_CURVE_B`). For best accuracy the sensor needs a long burn-in; let it warm up before trusting absolute readings.

Note: the dashboard's alert threshold (used for the trend-chart band, status tags, and weather-driven 80%/60% switch) is configured separately inside the embedded JavaScript (`ALERT_PERCENT` near the top of the `<script>` block), not from the firmware `#define`. Keep both in sync if you change one.

### Weather widget API key

The dashboard's weather pill calls OpenWeatherMap directly from the browser. Find this line inside the `<script>` block in `water.ino` and set your own key:

```js
var weatherAPIKey = 'api-key';
```

Get a free key at [openweathermap.org](https://openweathermap.org/api). Without a valid key, the badge shows "No API Key" / "Invalid API key" and the alert threshold simply stays at the default 80%.

The widget is hardcoded to Bengaluru coordinates (`blrLat`, `blrLon` in the script) — edit these if you're monitoring a chamber elsewhere.

---

## Installation

### 1. Install Arduino IDE + ESP8266 board support

Add this URL under **File → Preferences → Additional Boards Manager URLs**:

```
http://arduino.esp8266.com/stable/package_esp8266com_index.json
```

Then: **Tools → Board → Boards Manager → search "esp8266" → Install**

### 2. Install required libraries

In **Tools → Manage Libraries**, install:

- `ESP8266WiFi` — bundled with the ESP8266 board package
- `ESP8266WebServer` — bundled with the ESP8266 board package
- `ESP8266mDNS` — bundled with the ESP8266 board package
- `Adafruit GFX Library` — for the OLED
- `Adafruit SH110X` — SH1106 OLED driver

### 3. Flash

```
1. Open water.ino in Arduino IDE
2. Set board: Tools → Board → NodeMCU 1.0 (or your variant)
3. Set upload speed: 115200
4. Select your COM/USB port
5. Click Upload
```

### 4. Open the dashboard

After flashing, open Serial Monitor (115200 baud). Copy the printed IP and open it in any browser on the same network:

```
http://192.168.x.x/
```

---

## API

The ESP exposes three endpoints:

| Route | Response |
|---|---|
| `GET /` | Full HTML dashboard (served from PROGMEM, `Cache-Control: max-age=3600`) |
| `GET /level` | Plain text integer — current water level as `0`–`100` |
| `GET /gas` | Plain text integer — current methane reading in ppm |

Example polling from any script:

```bash
curl http://192.168.x.x/level
# → 73
curl http://192.168.x.x/gas
# → 112
```

---

## Dashboard

### Pages

The dashboard is organized into three pages, navigable from the left sidebar (or via URL hash: `#overview`, `#nodes`, `#zones`):

- **Overview** — primary live-node metric card, surface-gap distance, critical-node count, network average, real-time trend chart, and a compact node index
- **Nodes** — full card grid for all six configured nodes (fill %, gap in cm, inflow/outflow, net flow, status tag)
- **Map** — Leaflet map plotting all node coordinates

### Behavior notes

- **Only the first node (`krpuram` / "KR Puram Chamber") is real** — its level comes from `GET /level` and its methane from `GET /gas`. The other five nodes (Whitefield, Jayanagar, Hebbal, Electronic City, Yelahanka) are client-side simulated values that drift over time, included to make the multi-node layout demonstrable without extra hardware.
- **Trend chart** — 60-second rolling distance graph for the live node only, with hover tooltip and a shaded/dashed band marking the current alert threshold.
- **Status badges** — each node is tagged `Normal Flow` / `High Volume` / `Critical Backup` based on the current alert threshold.
- **CSV export** — downloads `hydra_sanitation_log.csv` with name, fill %, gap (cm), and status for all nodes.
- **Dark / light mode** — toggle in the top bar.
- **Simulation mode** — amber "Simulation Mode" badge shown when `/level` is unreachable or times out (~850ms); the live node then falls back to the same drift simulation used for the other five.
- **Weather pill** — shows current conditions near Bengaluru and adjusts the alert threshold (80% normal / 60% if rain is forecast within ~12h); refreshes every 15 minutes.

---

## How It Works

1. On boot, the ESP connects to Wi-Fi, self-calibrates the MQ-4 clean-air baseline, and starts an HTTP server on port 80
2. Every 250ms it takes a median-of-3 HC-SR04 reading (5ms echo timeout so a missing echo can't stall the server); every 1s it averages 5 MQ-4 samples into a ppm estimate
3. Distance is converted to a fill percentage using the configured `TANK_EMPTY_CM` / `TANK_FULL_CM` range
4. `GET /level` and `GET /gas` return the latest values as plain text; the server is also serviced right after each sensor read to stay responsive
5. The SH1106 OLED redraws (~every 300ms) with the fill %, a level bar, and CH₄ ppm plus a status label
6. The dashboard polls `/level` roughly every 0.8s and updates the live node; the other five nodes update from a local drift simulation
7. The buzzer latches with hysteresis: water fill triggers ON at `ALERT_PERCENT` / OFF below `ALERT_RELEASE`; methane triggers ON at `GAS_ALERT_PPM` (1000ppm) / OFF below `GAS_RELEASE_PPM` (900ppm)
8. In the browser, the dashboard separately tracks its own `ALERT_PERCENT` for display/threshold purposes, which the weather widget can shift between 80% and 60%
9. The full dashboard HTML/CSS/JS is stored in flash via `PROGMEM` to avoid RAM pressure

---

## Resource Usage

| Resource | Usage |
|---|---|
| Flash | ~350KB (mostly the embedded dashboard) |
| RAM | ~30KB at runtime |
| Water poll | Every 250ms (non-blocking) |
| Gas poll | Every 1s |
| OLED refresh | Every ~300ms |
| Dashboard poll | `/level` every ~0.8s |
| Wi-Fi | Idle between HTTP requests |

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Serial shows `WiFi FAILED` | Double-check SSID and password in the sketch |
| Distance reads `-1` | Check TRIG/ECHO wiring; sensor may be out of range (max ~4m) |
| Dashboard shows "Simulation Mode" | ESP not reachable — check IP, same network, firewall, or request timed out (>850ms) |
| Weather badge shows "No API Key" / "Invalid API key" | Set a valid OpenWeatherMap key in `weatherAPIKey` inside the script |
| Weather badge shows "Offline" | Browser couldn't reach OpenWeatherMap — check internet access on the client device (not the ESP) |
| Buzzer always on | `ALERT_PERCENT` too low / `TANK_FULL_CM` misconfigured, or methane above `GAS_ALERT_PPM` — check the MQ-4 |
| Methane reads absurdly high/low | Let the MQ-4 burn in; verify `Ro` calibrated in clean air at boot; check the AO voltage divider into A0 |
| OLED blank | Confirm SDA→D2, SCL→D1, address `0x3C`; firmware runs fine without a display if `display.begin()` fails |
| Level jumps erratically | Add a small capacitor across the HC-SR04 VCC/GND; reduce electrical noise |
| Other five nodes look "alive" with no extra sensors | Expected — they're client-side simulated, not real hardware (see Dashboard section above) |

---

## Environment

ESP8266 · Arduino IDE · Tested on NodeMCU v2 and Wemos D1 Mini

---

## Author

Created by [chakri192](https://github.com/chakri192)

## Contributors

| Contributor | Role |
|---|---|
| [chakri192](https://github.com/chakri192) | Author |
| [aider](https://github.com/Aider-AI/aider) | AI pair programmer |

### AI tooling

README and code contributions assisted by [aider](https://github.com/Aider-AI/aider) using local LLMs via [Ollama](https://ollama.com):

| Model | Used for |
|---|---|
| `qwen2.5-coder:7b` | Code suggestions, refactoring |
| `llama3.1:8b` | Prose, documentation, commit messages |

---

## License

MIT
