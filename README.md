# Project HYDRA

A real-time water level monitor for overhead tanks, built on the ESP8266. An ultrasonic sensor measures tank depth every 100ms and serves a live dashboard over Wi-Fi — buzzer alert included when the tank hits 80%.

**Live dashboard served at:** `http://<ESP-IP>/`

---

## Overview

Mount the sensor above your tank, flash the firmware, connect to Wi-Fi, and open the dashboard in any browser on your local network. No cloud, no subscriptions, no app — just your ESP8266 acting as its own web server.

---

## Features

- **Live water level** — ultrasonic reading every 100ms, polled by the dashboard every second
- **Wi-Fi dashboard** — dark/light mode, trend chart (60s rolling), per-chamber cards, CSV export
- **Buzzer alert** — rapid beep when tank exceeds 80% full
- **Simulation mode** — dashboard auto-falls back to simulated data if the ESP is unreachable
- **Leaflet map** — zone/location view built into the dashboard
- **Weather widget** — pulls local conditions via open-meteo (no API key needed)
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

Edit the top of `water.ino` before flashing:

```cpp
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

#define TANK_EMPTY_CM  30   // distance (cm) when tank is empty
#define TANK_FULL_CM    2   // distance (cm) when tank is full
#define ALERT_PERCENT  80   // buzzer triggers above this fill %
```

To get `TANK_EMPTY_CM` and `TANK_FULL_CM`: mount the sensor, open Serial Monitor at 115200 baud, and note the raw distance values at empty and full.

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

- **Trend chart** — 60-second rolling distance graph with hover tooltip
- **Tank cards** — fill percentage, gap in cm, status badge (OK / ALERT / FULL)
- **CSV export** — downloads a snapshot of all chamber readings
- **Dark / light mode** — toggle in the top bar
- **Simulation mode** — amber badge shown when ESP is unreachable; chart runs on synthetic data so you can preview the UI without hardware

---

## How It Works

1. On boot, the ESP connects to Wi-Fi and starts an HTTP server on port 80
2. Every 100ms, it pulses the HC-SR04 TRIG pin and measures the ECHO pulse duration
3. Distance is converted to a fill percentage using the configured `TANK_EMPTY_CM` / `TANK_FULL_CM` range
4. `GET /level` returns the latest percentage as plain text
5. The dashboard polls `/level` every second and updates the UI
6. If fill ≥ 80%, the buzzer toggles at 100ms intervals (rapid beep)
7. The full dashboard HTML/CSS/JS (~700 lines) is stored in flash via `PROGMEM` to avoid RAM pressure

---

## Resource Usage

| Resource | Usage |
|---|---|
| Flash | ~350KB (mostly the embedded dashboard) |
| RAM | ~30KB at runtime |
| Sensor poll | Every 100ms (non-blocking) |
| Wi-Fi | Idle between HTTP requests |

---

## Troubleshooting

| Problem | Fix |
|---|---|
| Serial shows `WiFi FAILED` | Double-check SSID and password in the sketch |
| Distance reads `-1` | Check TRIG/ECHO wiring; sensor may be out of range (max ~4m) |
| Dashboard shows "Simulation" | ESP not reachable — check IP, same network, firewall |
| Buzzer always on | `ALERT_PERCENT` too low or `TANK_FULL_CM` misconfigured |
| Level jumps erratically | Add a small capacitor across the HC-SR04 VCC/GND; reduce electrical noise |

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
