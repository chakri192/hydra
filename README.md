# Project HYDRA

A real-time water/sewage level monitor built on the ESP8266. An ultrasonic sensor measures chamber depth every 100ms and serves a live multi-node dashboard over Wi-Fi — buzzer alert included when the live node hits 80% (auto-lowers to 60% when rain is forecast).

**Live dashboard served at:** `http://<ESP-IP>/`

---

## Overview

Mount the sensor above your tank or chamber, flash the firmware, connect to Wi-Fi, and open the dashboard in any browser on your local network. No cloud, no subscriptions, no app — just your ESP8266 acting as its own web server. Only **one** node (`KR Puram Chamber`) is backed by real hardware; the other five nodes shown in the dashboard are simulated for layout/demo purposes (see the Dashboard section below).

---

## Features

- **Live water level** — ultrasonic reading every 100ms, polled by the dashboard roughly every 0.8s
- **Multi-node dashboard** — Overview, Nodes, and Map pages; only the first node (`krpuram`) reflects the real sensor, the rest are simulated
- **Trend chart** — 60-second rolling distance graph with hover tooltip and alert-threshold band
- **Buzzer alert** — rapid beep when the live node exceeds the alert threshold
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
| Sensor | HC-SR04 ultrasonic |
| Buzzer | Active buzzer (any 3.3 / 5V) |
| Power | USB or 5V supply |

### Wiring

| ESP8266 Pin | Connected to |
|---|---|
| D6 (GPIO 12) | HC-SR04 TRIG |
| D7 (GPIO 13) | HC-SR04 ECHO |
| D5 (GPIO 14) | Buzzer + |
| GND | HC-SR04 GND, Buzzer − |
| 5V / 3.3V | HC-SR04 VCC |

---

## Configuration

### Firmware settings

Edit the top of `water.ino` before flashing:

```cpp
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

#define TANK_EMPTY_CM  30   // distance (cm) when tank is empty
#define TANK_FULL_CM    2   // distance (cm) when tank is full
#define ALERT_PERCENT  80   // buzzer triggers above this fill % (firmware side)
```

To get `TANK_EMPTY_CM` and `TANK_FULL_CM`: mount the sensor, open Serial Monitor at 115200 baud, and note the raw distance values at empty and full.

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

The ESP exposes two endpoints:

| Route | Response |
|---|---|
| `GET /` | Full HTML dashboard (served from PROGMEM) |
| `GET /level` | Plain text integer — current water level as `0`–`100` |

Example polling from any script:

```bash
curl http://192.168.x.x/level
# → 73
```

---

## Dashboard

### Pages

The dashboard is organized into three pages, navigable from the left sidebar (or via URL hash: `#overview`, `#nodes`, `#zones`):

- **Overview** — primary live-node metric card, surface-gap distance, critical-node count, network average, real-time trend chart, and a compact node index
- **Nodes** — full card grid for all six configured nodes (fill %, gap in cm, inflow/outflow, net flow, status tag)
- **Map** — Leaflet map plotting all node coordinates

### Behavior notes

- **Only the first node (`krpuram` / "KR Puram Chamber") is real** — its level comes from `GET /level`. The other five nodes (Whitefield, Jayanagar, Hebbal, Electronic City, Yelahanka) are client-side simulated values that drift over time, included to make the multi-node layout demonstrable without extra hardware.
- **Trend chart** — 60-second rolling distance graph for the live node only, with hover tooltip and a shaded/dashed band marking the current alert threshold.
- **Status badges** — each node is tagged `Normal Flow` / `High Volume` / `Critical Backup` based on the current alert threshold.
- **CSV export** — downloads `hydra_sanitation_log.csv` with name, fill %, gap (cm), and status for all nodes.
- **Dark / light mode** — toggle in the top bar.
- **Simulation mode** — amber "Simulation Mode" badge shown when `/level` is unreachable or times out (~850ms); the live node then falls back to the same drift simulation used for the other five.
- **Weather pill** — shows current conditions near Bengaluru and adjusts the alert threshold (80% normal / 60% if rain is forecast within ~12h); refreshes every 15 minutes.

---

## How It Works

1. On boot, the ESP connects to Wi-Fi and starts an HTTP server on port 80
2. Every 100ms, it pulses the HC-SR04 TRIG pin and measures the ECHO pulse duration
3. Distance is converted to a fill percentage using the configured `TANK_EMPTY_CM` / `TANK_FULL_CM` range
4. `GET /level` returns the latest percentage as plain text
5. The dashboard polls `/level` roughly every 0.8s and updates the live node; the other five nodes update from a local drift simulation
6. If the live node's fill ≥ `ALERT_PERCENT` (firmware-side), the buzzer toggles at 100ms intervals (rapid beep)
7. In the browser, the dashboard separately tracks its own `ALERT_PERCENT` for display/threshold purposes, which the weather widget can shift between 80% and 60%
8. The full dashboard HTML/CSS/JS is stored in flash via `PROGMEM` to avoid RAM pressure

---

## Resource Usage

| Resource | Usage |
|---|---|
| Flash | ~350KB (mostly the embedded dashboard) |
| RAM | ~30KB at runtime |
| Sensor poll | Every 100ms (non-blocking) |
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
| Buzzer always on | `ALERT_PERCENT` (firmware `#define`) too low or `TANK_FULL_CM` misconfigured |
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
