#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

#define TRIG_PIN    12
#define ECHO_PIN    13
#define BUZZER_PIN  14

#define TANK_EMPTY_CM 30
#define TANK_FULL_CM  2
#define ALERT_PERCENT 80

ESP8266WebServer server(80);
int waterLevel = 0;

unsigned long lastSensorRead   = 0;
unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

#define SENSOR_INTERVAL 100
#define BUZZER_RAPID    100

float readDistanceCM();
int distanceToPercent(float dist);
void handleRoot();
void handleLevel();

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long d = pulseIn(ECHO_PIN, HIGH, 30000);
  if (d == 0) return -1;
  return d * 0.034 / 2.0;
}

int distanceToPercent(float dist) {
  if (dist < 0) return -1;
  int pct = map((int)dist, TANK_EMPTY_CM, TANK_FULL_CM, 0, 100);
  return constrain(pct, 0, 100);
}

const char PAGE[] PROGMEM = R"HYDRA(
<!DOCTYPE html>
<html lang="en" data-theme="light">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Project HYDRA | Sanitation Node</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap" rel="stylesheet">
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
<style>
:root {
  --bg: #f8fafc;
  --panel: #ffffff;
  --line: #e2e8f0;
  --text: #0f172a;
  --muted: #64748b;
  --dim: #94a3b8;
  --green: #10b981;
  --amber: #f59e0b;
  --red: #ef4444;
  --cyan: #0ea5e9;
  --blue: #3b82f6;
  --shadow: 0 4px 20px rgba(0, 0, 0, 0.04);
  --sewage: rgba(94, 105, 84, 0.85);
  --sewage-light: rgba(126, 138, 114, 0.5);
  --vessel-bg: #f1f5f9;
}
[data-theme="dark"] {
  --bg: #050607;
  --panel: #0b0e10;
  --line: #1c2428;
  --text: #f4f7f7;
  --muted: #829099;
  --dim: #4e5b62;
  --shadow: 0 8px 30px rgba(0, 0, 0, 0.4);
  --vessel-bg: #101417;
}
* { box-sizing: border-box; }
html, body { min-height: 100%; margin: 0; }
body {
  background: var(--bg);
  color: var(--text);
  font-family: 'Inter', sans-serif;
  -webkit-font-smoothing: antialiased;
  text-rendering: optimizeLegibility;
  overflow-x: hidden;
  overflow-y: auto;
  transition: background 0.3s ease, color 0.3s ease;
}
button { font: inherit; border: none; outline: none; }
.app { display: flex; min-height: 100vh; }
.side {
  position: fixed; inset: 0 auto 0 0; width: 240px;
  background: var(--panel);
  border-right: 1px solid var(--line);
  padding: 24px 20px;
  display: flex; flex-direction: column; gap: 20px;
  z-index: 3;
  transition: background 0.3s, border-color 0.3s;
}
.brand { display: flex; align-items: center; gap: 12px; padding: 4px 4px 14px; }
.mark {
  width: 44px; height: 44px; border-radius: 13px;
  display: grid; place-items: center;
  background: #3b82f6;
  color: white; font-weight: 800; font-size: 16px; letter-spacing: .04em;
  box-shadow: 0 6px 16px rgba(59, 130, 246, 0.3);
  flex-shrink: 0;
}
.brand h1 { font-size: 18px; margin: 0; letter-spacing: .05em; color: var(--text); }
.brand p { font-size: 11px; margin: 3px 0 0; color: var(--muted); }
.nav { display: flex; flex-direction: column; gap: 4px; }
.nav a {
  text-decoration: none; color: var(--muted);
  display: flex; align-items: center; gap: 12px;
  min-height: 40px; padding: 0 10px; border-radius: 10px;
  font-size: 14px; font-weight: 500; transition: .2s;
}
.nav a span:first-child {
  width: 36px; height: 36px; border-radius: 9px;
  display: grid; place-items: center; flex-shrink: 0;
  color: var(--muted); background: transparent;
}
.nav a.active, .nav a:hover { color: var(--text); background: var(--line); }
.nav a.active span:first-child { color: var(--text); }
.main { margin-left: 240px; width: calc(100% - 240px); min-height: 100vh; }
.top {
  position: sticky; top: 0; z-index: 2;
  display: flex; align-items: center; justify-content: space-between; gap: 16px;
  padding: 24px 32px 16px;
  background: var(--bg);
  opacity: 0.95; backdrop-filter: blur(12px);
  transition: background 0.3s;
}
.title h2 { margin: 0; font-size: 26px; letter-spacing: -0.02em; font-weight: 700; }
.title p { margin: 6px 0 0; color: var(--muted); font-size: 14px; }
.actions { display: flex; gap: 10px; align-items: center; flex-wrap: wrap; justify-content: flex-end; }
.pill, .status {
  border: 1px solid var(--line); background: var(--panel); color: var(--text);
  border-radius: 999px; padding: 8px 14px; font-size: 12px; font-weight: 500;
  display: flex; gap: 8px; align-items: center; white-space: nowrap;
  box-shadow: 0 2px 8px rgba(0,0,0,0.02);
  transition: background 0.3s, border-color 0.3s, color 0.3s;
}
.status i { width: 8px; height: 8px; border-radius: 50%; background: var(--green); }
.status.sim i { background: var(--amber); }
.btn { cursor: pointer; }
.btn:hover { background: var(--line); }
.content { padding: 0 32px 32px; }
.content.page { padding-top: 28px; }
.page[hidden] { display: none; }
.page-head { margin-bottom: 24px; }
.page-head h3 { margin: 0 0 6px; font-size: 20px; font-weight: 700; letter-spacing: -0.01em; }
.page-head p { margin: 0; color: var(--muted); font-size: 14px; }
.metrics { display: grid; grid-template-columns: repeat(4, minmax(150px, 1fr)); gap: 16px; margin-bottom: 24px; }
.metric {
  background: var(--panel); border: 1px solid var(--line); border-radius: 16px; padding: 20px;
  min-height: 112px; box-shadow: var(--shadow);
  transition: background 0.3s, border-color 0.3s;
}
.metric small { display: block; color: var(--muted); font-size: 13px; font-weight: 600; margin-bottom: 12px; }
.metric strong { font-size: 32px; font-weight: 800; line-height: 1; letter-spacing: -0.03em; }
.metric p { margin: 8px 0 0; color: var(--dim); font-size: 12px; font-weight: 500; }
.grid { display: grid; grid-template-columns: 1.2fr 0.8fr; gap: 20px; margin-bottom: 24px; align-items: start; }
.chart-card, .zone-card {
  background: var(--panel); border: 1px solid var(--line); border-radius: 16px; padding: 20px;
  box-shadow: var(--shadow);
  transition: background 0.3s, border-color 0.3s;
}
.card-head { display: flex; align-items: flex-start; justify-content: space-between; gap: 12px; margin-bottom: 16px; }
.card-head h3 { margin: 0; font-size: 16px; font-weight: 600; color: var(--text); }
.card-head p { margin: 4px 0 0; color: var(--muted); font-size: 13px; }
.legend { display: flex; gap: 12px; color: var(--muted); font-size: 12px; flex-wrap: wrap; }
.legend span { display: flex; gap: 6px; align-items: center; }
.legend i { width: 8px; height: 8px; border-radius: 50%; display: block; }
.canvas-wrap { height: 280px; position: relative; border-radius: 12px; overflow: hidden; background: var(--bg); border: 1px solid var(--line); cursor: crosshair; }
canvas { width: 100%; height: 100%; display: block; }
.zone-list { display: grid; gap: 12px; }
.zone-mini {
  color: inherit; text-decoration: none;
  display: grid; grid-template-columns: 1fr auto; gap: 10px;
  padding: 14px; border: 1px solid var(--line); border-radius: 12px;
  background: var(--bg); cursor: pointer; transition: .2s ease;
}
.zone-mini:hover, .zone-mini.active { border-color: var(--dim); box-shadow: 0 4px 12px rgba(0,0,0,0.05); }
.zone-mini h4 { margin: 0; font-size: 14px; font-weight: 600; }
.zone-mini p { margin: 4px 0 0; color: var(--muted); font-size: 12px; }
.mini-pct { font-weight: 700; font-size: 20px; line-height: 1; }
.mini-bar { grid-column: 1 / -1; height: 6px; border-radius: 99px; background: var(--line); overflow: hidden; margin-top: 4px; }
.mini-bar div { height: 100%; border-radius: 99px; transition: width .4s, background .4s; }
/* ── tank cards (compact row layout) ── */
.cards { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 12px; }
.cards--3col { grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 16px; }
.tank-card {
  background: var(--panel); border: 1px solid var(--line); border-radius: 16px; padding: 20px 22px;
  box-shadow: var(--shadow); transition: background 0.3s, border-color 0.3s;
  display: flex; flex-direction: column; gap: 14px;
}
/* top row: name + tag */
.tank-top { display: flex; align-items: center; justify-content: space-between; gap: 12px; }
.tank-id { display: flex; align-items: center; gap: 10px; min-width: 0; }
.icon { display: none; } /* removed — adds no info */
.tank-id h3 { margin: 0; font-size: 14px; font-weight: 700; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.tank-id p { margin: 2px 0 0; color: var(--muted); font-size: 12px; }
.tag { font-size: 11px; font-weight: 600; border-radius: 999px; padding: 4px 10px; border: 1px solid transparent; white-space: nowrap; }
.tag.ok { color: var(--green); background: rgba(16,185,129,0.08); border-color: rgba(16,185,129,0.2); }
.tag.warn { color: var(--amber); background: rgba(245,158,11,0.08); border-color: rgba(245,158,11,0.2); }
.tag.crit { color: var(--red); background: rgba(239,68,68,0.08); border-color: rgba(239,68,68,0.2); }
/* fill bar row */
.tank-fill-row { display: flex; align-items: center; gap: 10px; }
.tank-fill-pct { font-size: 20px; font-weight: 800; letter-spacing: -0.03em; min-width: 48px; line-height: 1; }
.tank-fill-bar-wrap { flex: 1; height: 8px; border-radius: 99px; background: var(--line); overflow: hidden; }
.tank-fill-bar { height: 100%; border-radius: 99px; transition: width .7s cubic-bezier(.2,.8,.2,1), background .4s; }
/* stats row */
.tank-stats { display: flex; gap: 0; border-top: 1px solid var(--line); padding-top: 10px; }
.tank-stat { flex: 1; }
.tank-stat span { display: block; color: var(--muted); font-size: 11px; font-weight: 500; margin-bottom: 3px; }
.tank-stat b { font-size: 14px; font-weight: 700; color: var(--text); }
.tank-stat + .tank-stat { border-left: 1px solid var(--line); padding-left: 14px; margin-left: 14px; }
/* keep kv/label/vessel/telemetry/wave defs but they won't be used */
.kv { display: none; }
.vessel { display: none; }
.telemetry { display: none; }
.label { display: none; }
.tank-body { display: none; }
.toast {
  position: fixed; right: 24px; bottom: 24px; z-index: 4;
  background: var(--text); border: 1px solid var(--text);
  color: var(--bg); border-radius: 12px; padding: 14px 18px; font-size: 13px; font-weight: 500;
  box-shadow: 0 12px 32px rgba(0,0,0,0.15);
  opacity: 0; transform: translateY(15px); pointer-events: none; transition: .3s cubic-bezier(0.2, 0.8, 0.2, 1);
}
.toast.show { opacity: 1; transform: translateY(0); }
#map { height: 500px; border-radius: 16px; border: 1px solid var(--line); z-index: 1; }
/* skeleton loading */
@keyframes shimmer { 0%,100%{opacity:.4} 50%{opacity:.9} }
.skeleton-val { animation: shimmer 1.4s ease-in-out infinite; }
.skeleton-line { animation: shimmer 1.4s ease-in-out 0.2s infinite; }

/* metric hierarchy */
.metric--primary {
  border-color: var(--sewage);
  background: linear-gradient(135deg, var(--panel) 0%, rgba(94,105,84,0.06) 100%);
}
.metric--primary strong { font-size: 42px; }

/* header action group */
.action-group { display: flex; gap: 6px; }
.action-group .btn { padding: 8px 10px; }
.export-btn { padding: 8px 14px !important; }

/* weather pill */
.weather-icon { font-style: normal; }

/* net flow in card */
.net-flow b { font-size: 15px; }

.metric-sub { margin: 8px 0 0; color: var(--dim); font-size: 12px; font-weight: 500; }


@media(max-width: 1200px) { .cards--3col { grid-template-columns: repeat(2, 1fr); } .grid { grid-template-columns: 1fr; } }
@media(max-width: 1120px) {
  .actions { flex-direction: column; align-items: stretch; width: 100%; margin-top: 10px; }
  .top { flex-direction: column; align-items: flex-start; }
}
@media(max-width: 840px) {
  body { overflow: auto; }
  .app { display: block; }
  .side { position: relative; width: auto; inset: auto; flex-direction: row; align-items: center; overflow: auto; padding: 16px 20px; border-right: none; border-bottom: 1px solid var(--line); }
  .brand { padding: 0; min-width: 150px; }
  .nav { flex-direction: row; gap: 12px; }
  .nav a { min-width: 44px; padding: 0 10px; }
  .nav a span+span { display: none; }
  .main { margin-left: 0; width: auto; min-height: auto; }
  .top { position: relative; align-items: flex-start; flex-direction: column; padding: 20px; }
  .actions { flex-direction: row; align-items: center; width: auto; }
  .content { padding: 0 20px 24px; }
  .metrics { grid-template-columns: repeat(2, minmax(0, 1fr)); }
  .cards { grid-template-columns: 1fr; }
}
@media(max-width: 520px) {
  .metrics { grid-template-columns: 1fr; }
  .tank-top { align-items: flex-start; flex-direction: column; }
  .canvas-wrap { height: 220px; }
  .tank-stats { flex-wrap: wrap; gap: 8px; }
  .tank-stat + .tank-stat { border-left: none; padding-left: 0; margin-left: 0; border-top: 1px solid var(--line); padding-top: 8px; }
}
</style>
</head>
<body>
<div class="app">
  <aside class="side">
    <div class="brand">
      <div class="mark">H</div>
      <div><h1>HYDRA</h1><p>Urban Sanitation</p></div>
    </div>
    <nav class="nav">
      <a class="active" href="#overview" data-page="overview">
        <span><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/></svg></span>
        <span>Overview</span>
      </a>
      <a href="#nodes" data-page="nodes">
        <span><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><ellipse cx="12" cy="5" rx="3" ry="2"/><ellipse cx="4" cy="19" rx="3" ry="2"/><ellipse cx="20" cy="19" rx="3" ry="2"/><path d="M12 7v4m0 0-6 6m6-6 6 6"/></svg></span>
        <span>Nodes</span>
      </a>
      <a href="#zones" data-page="zones">
        <span><svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2C8.686 2 6 4.686 6 8c0 4.5 6 12 6 12s6-7.5 6-12c0-3.314-2.686-6-6-6z"/><circle cx="12" cy="8" r="2"/></svg></span>
        <span>Map</span>
      </a>
    </nav>
  </aside>

  <main class="main">
    <header class="top">
      <div class="title">
        <h2>System Dashboard</h2>
        <p>Distance and sewer-level monitoring across active Bengaluru nodes.</p>
      </div>
      <div class="actions">
        <div class="pill weather-pill" id="weatherBadge">
          <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="weather-svg"><path d="M20 17.58A5 5 0 0 0 18 8h-1.26A8 8 0 1 0 4 16.25"/><line x1="8" y1="19" x2="8" y2="21"/><line x1="8" y1="13" x2="8" y2="15"/><line x1="16" y1="19" x2="16" y2="21"/><line x1="16" y1="13" x2="16" y2="15"/><line x1="12" y1="21" x2="12" y2="23"/><line x1="12" y1="15" x2="12" y2="17"/></svg>
          <span id="weatherText">Loading...</span>
        </div>
        <div class="pill threshold-pill" id="thresholdBadge">Threshold: 80%</div>
        <div class="action-group">
          <button class="pill btn" id="btnTheme" title="Toggle theme"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="5"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg></button>
          <button class="pill btn" id="btnRefresh" title="Refresh data"><svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8"/><path d="M21 3v5h-5"/><path d="M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16"/><path d="M8 16H3v5"/></svg></button>
          <button class="pill btn export-btn" id="btnExport">Export CSV</button>
        </div>
        <div class="status" id="modeBadge"><i></i><span>Connecting</span></div>
      </div>
    </header>

    <section class="content page" id="overviewPage">
      <div class="metrics">
        <div class="metric metric--primary"><small>KR Puram Node</small><strong id="mLive" class="skeleton-val">--%</strong><p id="mLiveSub" class="metric-sub skeleton-line">Connecting to sensor…</p></div>
        <div class="metric"><small>Surface Gap</small><strong id="mDistance" class="skeleton-val">-- cm</strong><p class="metric-sub">Ultrasonic distance</p></div>
        <div class="metric"><small>Critical Manholes</small><strong id="mCritical" class="skeleton-val">–</strong><p id="mCriticalSub" class="metric-sub skeleton-line">Checking…</p></div>
        <div class="metric"><small>Network Load</small><strong id="mAverage" class="skeleton-val">--%</strong><p class="metric-sub">6-node average</p></div>
      </div>

      <div class="grid">
        <section class="chart-card">
          <div class="card-head">
            <div><h3>Real-time Distance</h3><p>KR Puram incoming readings. Hover for precise values.</p></div>
            <div class="legend"><span><i style="background:var(--cyan)"></i>Distance cm</span><span><i style="background:var(--red)"></i>Backup limit</span></div>
          </div>
          <div class="canvas-wrap"><canvas id="trendCanvas"></canvas></div>
        </section>

        <aside class="zone-card">
          <div class="card-head">
            <div><h3>Node Index</h3><p>Tap a node to inspect.</p></div>
          </div>
          <div class="zone-list" id="zoneList"></div>
        </aside>
      </div>
    </section>

    <section class="content page" id="nodesPage" hidden>
      <div class="page-head">
        <h3>All Nodes</h3>
        <p>Live telemetry across all 6 monitored chambers.</p>
      </div>
      <section class="cards cards--3col" id="tankCards"></section>
    </section>

    <section class="content page" id="zonesPage" hidden>
      <div class="page-head">
        <h3>Geographic Map</h3>
        <p>Physical placement of monitored chambers across Bengaluru.</p>
      </div>
      <div id="map"></div>
    </section>
  </main>
</div>
<div class="toast" id="toast">Sensor endpoint unavailable.<br>Simulation mode is now active.</div>

<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
<script>
(function(){
  var EMPTY_CM=30, FULL_CM=2;
  var ALERT_PERCENT=80; // Dynamically updated by weather
  var POLL_MS=800, TIMEOUT_MS=850, TREND_MS=60000;
  var simulation=false, focused='krpuram', lastToast=0;
  var lastTick=Date.now();
  var series=[];
  
  // Weather API Setup (Replace with your actual OpenWeatherMap API Key)
  var weatherAPIKey = 'api-key';
  // Bengaluru Coordinates
  var blrLat = 12.9716, blrLon = 77.5946;
  
  var zones=[
    {id:'krpuram', name:'KR Puram Chamber', meta:'Ward 85 - East BLR', capacity:500000, level:0, temp:22, pressure:45, fill:1200, drain:980, live:true, lat:13.0068, lng:77.6966},
    {id:'whitefield', name:'Whitefield Node', meta:'ITPL Main Line', capacity:300000, level:52, temp:24, pressure:40, fill:950, drain:750, lat:12.9698, lng:77.7499},
    {id:'jayanagar', name:'Jayanagar Main', meta:'South Grid Basin', capacity:250000, level:61, temp:25, pressure:30, fill:800, drain:600, lat:12.9299, lng:77.5933},
    {id:'hebbal', name:'Hebbal Interceptor', meta:'North Ring Buffer', capacity:120000, level:79, temp:23, pressure:25, fill:600, drain:400, lat:13.0354, lng:77.5988},
    {id:'electronic', name:'E-City Catchment', meta:'Industrial Supply', capacity:220000, level:37, temp:26, pressure:34, fill:720, drain:510, lat:12.8399, lng:77.6770},
    {id:'yelahanka', name:'Yelahanka Basin', meta:'Aviation Rd Reserve', capacity:185000, level:84, temp:22, pressure:42, fill:680, drain:880, lat:13.1007, lng:77.5963}
  ];
  
  function byId(id){return document.getElementById(id)}
  function clamp(v,min,max){return Math.max(min,Math.min(max,v))}
  function round(v){return Math.round(v)}
  function fmt(n){return String(Math.round(n)).replace(/\B(?=(\d{3})+(?!\d))/g,',')}
  function distanceFromLevel(p){return EMPTY_CM-(clamp(p,0,100)/100)*(EMPTY_CM-FULL_CM)}
  function state(p){return p>=ALERT_PERCENT?'Critical Backup':p>=(ALERT_PERCENT-20)?'High Volume':'Normal Flow'}
  function stateClass(p){return p>=ALERT_PERCENT?'crit':p>=(ALERT_PERCENT-20)?'warn':'ok'}
  function color(p){return p>=ALERT_PERCENT?'#ef4444':p>=(ALERT_PERCENT-20)?'#f59e0b':'#10b981'}
  
  var SVG_RAIN = '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20 17.58A5 5 0 0 0 18 8h-1.26A8 8 0 1 0 4 16.25"/><line x1="8" y1="19" x2="8" y2="21"/><line x1="8" y1="13" x2="8" y2="15"/><line x1="16" y1="19" x2="16" y2="21"/><line x1="16" y1="13" x2="16" y2="15"/><line x1="12" y1="21" x2="12" y2="23"/><line x1="12" y1="15" x2="12" y2="17"/></svg>';
  var SVG_SUN  = '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="5"/><line x1="12" y1="1" x2="12" y2="3"/><line x1="12" y1="21" x2="12" y2="23"/><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"/><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"/><line x1="1" y1="12" x2="3" y2="12"/><line x1="21" y1="12" x2="23" y2="12"/><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"/><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"/></svg>';
  var SVG_WARN = '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>';
  function setWeatherBadge(icon, text) {
    byId('weatherBadge').innerHTML = icon + '<span>' + text + '</span>';
  }

  function fetchWeather() {
    if(weatherAPIKey === 'YOUR_OPENWEATHER_API_KEY_HERE') {
      setWeatherBadge(SVG_WARN, 'No API Key');
      return;
    }
    var url = 'https://api.openweathermap.org/data/2.5/forecast?lat=' + blrLat + '&lon=' + blrLon + '&appid=' + weatherAPIKey;
    
    fetch(url)
      .then(function(r) {
        if (!r.ok) throw new Error('HTTP ' + r.status);
        return r.json();
      })
      .then(function(data) {
        if (!data || !data.list) throw new Error('Bad API data');
        
        var rainIncoming = false;
        for(var i=0; i<4; i++){
          var condition = data.list[i].weather[0].main.toLowerCase();
          if(condition.includes('rain') || condition.includes('thunderstorm') || condition.includes('drizzle')) {
            rainIncoming = true;
            break;
          }
        }

        if(rainIncoming) {
          ALERT_PERCENT = 60;
          setWeatherBadge(SVG_RAIN, 'Rain expected (12h)');
          byId('thresholdBadge').innerHTML = 'Threshold: 60% — Monsoon';
          byId('thresholdBadge').style.borderColor = 'var(--amber)';
          byId('thresholdBadge').style.color = 'var(--amber)';
        } else {
          ALERT_PERCENT = 80;
          setWeatherBadge(SVG_SUN, 'Clear (12h)');
          byId('thresholdBadge').innerHTML = 'Threshold: 80%';
          byId('thresholdBadge').style.borderColor = 'var(--line)';
          byId('thresholdBadge').style.color = 'var(--text)';
        }
        renderAll(false); 
      })
      .catch(function(e) {
        console.log('Weather fetch failed:', e.message);
        if (e.message.includes('401')) {
          setWeatherBadge(SVG_WARN, 'Invalid API key');
        } else if (e.message.includes('Failed to fetch')) {
          setWeatherBadge(SVG_WARN, 'Offline');
        } else {
          setWeatherBadge(SVG_WARN, 'Weather unavailable');
        }
      });
  }
  function setMode(isSim){
    simulation=isSim;
    var badge=byId('modeBadge');
    badge.className='status'+(simulation?' sim':'');
    badge.querySelector('span').textContent=simulation?'Simulation Mode':'Live Sensor';
  }

  function showToast(){
    var now=Date.now();
    if(now-lastToast<7000)return;
    lastToast=now;
    var t=byId('toast');
    t.classList.add('show');
    setTimeout(function(){t.classList.remove('show')},3600);
  }

  function simulateStep(){
    var now=Date.now();
    var dt=Math.max(.2,(now-lastTick)/1000);
    lastTick=now;
    for(var i=1;i<zones.length;i++){
      var z=zones[i];
      var drift=Math.sin(now/8000+i)*0.18+Math.sin(now/13000+i*2)*0.08;
      var noise=(Math.random()-.5)*0.28;
      var bias=z.level>88?-0.18:z.level<18?0.18:0;
      z.level=clamp(z.level+(drift+noise+bias)*dt,6,96);
      z.fill=clamp(z.fill+(Math.random()-.5)*6,350,1300);
      z.drain=clamp(z.drain+(Math.random()-.5)*6,260,1050);
    }
  }

  function pushReading(){
    var live=zones[0];
    var now=Date.now();
    series.push({t:now,d:distanceFromLevel(live.level)});
    while(series.length&&now-series[0].t>TREND_MS+5000)series.shift();
  }

  function renderMetrics(){
    var live=zones[0],sum=0,crit=[];
    for(var i=0;i<zones.length;i++){
      sum+=zones[i].level;
      if(zones[i].level>=ALERT_PERCENT)crit.push(zones[i].name);
    }
    // remove skeleton once we have real data
    var skels = document.querySelectorAll('.skeleton-val,.skeleton-line');
    for(var j=0;j<skels.length;j++) skels[j].classList.remove('skeleton-val','skeleton-line');
    byId('mLive').textContent=round(live.level)+'%';
    byId('mLive').style.color=color(live.level);
    byId('mLiveSub').textContent=simulation?'Simulation active':state(live.level);
    byId('mDistance').textContent=distanceFromLevel(live.level).toFixed(1)+' cm';
    byId('mDistance').style.color=color(live.level);
    byId('mCritical').textContent=crit.length;
    byId('mCritical').style.color=crit.length?'#ef4444':'#10b981';
    byId('mCriticalSub').textContent=crit.length?crit.join(', '):'All nodes within limits';
    byId('mAverage').textContent=round(sum/zones.length)+'%';
  }

  function renderZoneList(){
    var html='';
    zones.forEach(function(z){
      var p=round(z.level),c=color(p);
      html+='<a class="zone-mini '+(z.id===focused?'active':'')+'" href="#area-'+z.id+'" data-zone="'+z.id+'">'
        +'<div><h4>'+z.name+'</h4><p>'+z.meta+'</p></div>'
        +'<div class="mini-pct" style="color:'+c+'">'+p+'%</div>'
        +'<div class="mini-bar"><div style="width:'+p+'%;background:'+c+'"></div></div>'
        +'</a>';
    });
    byId('zoneList').innerHTML=html;
    var nodes=document.querySelectorAll('.zone-mini');
    for(var i=0;i<nodes.length;i++){
      nodes[i].onclick=function(e){ e.preventDefault(); openArea(this.getAttribute('data-zone')); };
    }
  }

  function renderCards(){
    var ordered=zones.slice();
    ordered.sort(function(a,b){
      if(a.id===focused)return -1;
      if(b.id===focused)return 1;
      return b.level-a.level;
    });
    var html='';
    ordered.forEach(function(z){
      var p=round(z.level),dist=distanceFromLevel(z.level),st=state(z.level),sc=stateClass(z.level),c=color(p);
      var netFlow=round(z.fill-z.drain);
      var netColor=netFlow>0?'var(--amber)':'var(--green)';
      var netStr=(netFlow>0?'+':'')+fmt(netFlow)+' L/m';
      html+='<article class="tank-card" id="area-'+z.id+'">'
        +'<div class="tank-top"><div class="tank-id"><div><h3>'+z.name+'</h3><p>'+z.meta+'</p></div></div><div class="tag '+sc+'">'+st+'</div></div>'
        +'<div class="tank-fill-row">'
        +'<div class="tank-fill-pct" style="color:'+c+'">'+p+'%</div>'
        +'<div class="tank-fill-bar-wrap"><div class="tank-fill-bar" style="width:'+p+'%;background:'+c+'"></div></div>'
        +'</div>'
        +'<div class="tank-stats">'
        +'<div class="tank-stat"><span>Gap</span><b>'+dist.toFixed(1)+' cm</b></div>'
        +'<div class="tank-stat"><span>Inflow</span><b>'+round(z.fill)+' L/m</b></div>'
        +'<div class="tank-stat"><span>Outflow</span><b>'+round(z.drain)+' L/m</b></div>'
        +'<div class="tank-stat"><span>Net</span><b style="color:'+netColor+'">'+netStr+'</b></div>'
        +'</div>'
        +'</article>';
    });
    byId('tankCards').innerHTML=html;
  }

  function renderAll(addPoint){
    if(addPoint)pushReading();
    renderMetrics();
    renderZoneList();
    renderCards();
  }

  var map;
  function initMap(){
    if(map) return;
    map = L.map('map').setView([12.9716, 77.5946], 11);
    L.tileLayer('https://{s}.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}{r}.png', {
      maxZoom: 19, attribution: '&copy; OpenStreetMap contributors &copy; CARTO'
    }).addTo(map);
    
    zones.forEach(function(z) {
      var marker = L.marker([z.lat, z.lng]).addTo(map);
      marker.bindPopup("<b>"+z.name+"</b><br>System Load: " + round(z.level) + "%<br>Status: " + state(z.level));
    });
  }

  function showPage(page){
    byId('overviewPage').hidden = page !== 'overview';
    byId('nodesPage').hidden = page !== 'nodes';
    byId('zonesPage').hidden = page !== 'zones';
    var links=document.querySelectorAll('.nav a');
    for(var i=0;i<links.length;i++){
      links[i].className=links[i].getAttribute('data-page')===page?'active':'';
    }
    if(page==='zones') {
      initMap();
      setTimeout(function(){ map.invalidateSize(); }, 100);
    }
  }

  function openArea(id){
    focused=id;
    showPage('nodes');
    if(location.hash!=='#nodes')location.hash='nodes';
    renderAll(false);
    setTimeout(function(){
      var target=byId('area-'+focused);
      if(target)target.scrollIntoView({behavior:'smooth',block:'start'});
    }, 30);
  }

  function syncRoute(){
    var h = location.hash.replace('#','');
    showPage(h==='zones'?'zones':h==='nodes'?'nodes':'overview');
  }

  function fetchWithTimeout(url,ms){
    if(window.AbortController){
      var controller=new AbortController();
      var timer=setTimeout(function(){controller.abort()},ms);
      return fetch(url,{cache:'no-store',signal:controller.signal}).then(function(r){
        clearTimeout(timer);
        if(!r.ok)throw new Error('HTTP '+r.status);
        return r.text();
      });
    }
    return Promise.race([
      fetch(url,{cache:'no-store'}).then(function(r){if(!r.ok)throw new Error('HTTP '+r.status);return r.text()}),
      new Promise(function(_,reject){setTimeout(function(){reject(new Error('timeout'))},ms)})
    ]);
  }

  function fetchLevel(){
    fetchWithTimeout('/level',TIMEOUT_MS).then(function(text){
      var n=parseInt(text,10);
      if(isNaN(n))throw new Error('invalid level');
      zones[0].level=clamp(n,0,100);
      setMode(false); simulateStep(); renderAll(true);
    }).catch(function(){
      if(!simulation)showToast();
      setMode(true); simulateStep(); renderAll(true);
    });
  }

  var cvs = byId('trendCanvas');
  var mouseX = 0, hovering = false;
  cvs.addEventListener('mousemove', function(e){
    var rect = cvs.getBoundingClientRect();
    mouseX = e.clientX - rect.left;
    hovering = true;
  });
  cvs.addEventListener('mouseleave', function(){ hovering = false; });

  function drawTrend(){
    var ctx=cvs.getContext('2d');
    var rect=cvs.getBoundingClientRect();
    var ratio=window.devicePixelRatio||1;
    var w=Math.max(320,Math.floor(rect.width)),h=Math.max(180,Math.floor(rect.height));
    if(cvs.width!==w*ratio||cvs.height!==h*ratio){
      cvs.width=w*ratio;cvs.height=h*ratio;ctx.setTransform(ratio,0,0,ratio,0,0);
    }
    
    var isDark = document.documentElement.getAttribute('data-theme') === 'dark';
    ctx.clearRect(0,0,w,h);
    var padL=44,padR=16,padT=18,padB=30;
    var now=Date.now();
    var minD=FULL_CM,maxD=EMPTY_CM;
    
    ctx.fillStyle = isDark ? '#0b0e10' : '#ffffff'; ctx.fillRect(0,0,w,h);
    ctx.strokeStyle = isDark ? '#1c2428' : '#e2e8f0'; ctx.lineWidth=1;
    ctx.fillStyle = isDark ? '#64748b' : '#94a3b8'; ctx.font='11px Inter, sans-serif';
    
    for(var gy=0;gy<=4;gy++){
      var y=padT+gy*(h-padT-padB)/4;
      ctx.globalAlpha=gy===4?1:.75;
      ctx.beginPath();ctx.moveTo(padL,y);ctx.lineTo(w-padR,y);ctx.stroke();
      var val=maxD-gy*(maxD-minD)/4;
      ctx.fillText(val.toFixed(0)+' cm',8,y+4);
    }
    ctx.globalAlpha=1;
    var alertD=distanceFromLevel(ALERT_PERCENT);
    var ay=padT+(maxD-alertD)/(maxD-minD)*(h-padT-padB);
    
    ctx.fillStyle='rgba(239, 68, 68, 0.08)';
    ctx.fillRect(padL,padT,w-padL-padR,Math.max(0,ay-padT));
    ctx.strokeStyle='rgba(239, 68, 68, 0.6)';
    ctx.setLineDash([6,5]);ctx.beginPath();ctx.moveTo(padL,ay);ctx.lineTo(w-padR,ay);ctx.stroke();ctx.setLineDash([]);
    
    var closestPoint = null, minPxDist = Infinity, cx = 0, cy = 0;

    if(series.length>1){
      ctx.beginPath();
      for(var i=0;i<series.length;i++){
        var point=series[i];
        var x=w-padR-((now-point.t)/TREND_MS)*(w-padL-padR);
        var y=padT+(point.d-minD)/(maxD-minD)*(h-padT-padB);
        if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);
        
        if(hovering && x >= padL && x <= w-padR) {
          var dx = Math.abs(x - mouseX);
          if(dx < minPxDist) { minPxDist = dx; closestPoint = point; cx = x; cy = y; }
        }
      }
      var g=ctx.createLinearGradient(padL,0,w-padR,0);
      g.addColorStop(0,'rgba(14, 165, 233, 0.2)');
      g.addColorStop(1,'#0ea5e9');
      ctx.strokeStyle=g;ctx.lineWidth=3;ctx.lineCap='round';ctx.lineJoin='round';ctx.stroke();
      
      var last=series[series.length-1];
      var lx=w-padR-((now-last.t)/TREND_MS)*(w-padL-padR);
      var ly=padT+(last.d-minD)/(maxD-minD)*(h-padT-padB);
      ctx.fillStyle='#0ea5e9';ctx.beginPath();ctx.arc(lx,ly,4,0,Math.PI*2);ctx.fill();
    }
    
    if(hovering && closestPoint && minPxDist < 30) {
      ctx.strokeStyle = isDark ? '#4e5b62' : '#cbd5e1';
      ctx.lineWidth = 1; ctx.setLineDash([4,4]);
      ctx.beginPath(); ctx.moveTo(cx, padT); ctx.lineTo(cx, h-padB); ctx.stroke(); ctx.setLineDash([]);
      
      ctx.beginPath(); ctx.arc(cx, cy, 5, 0, Math.PI*2); 
      ctx.fillStyle = '#0ea5e9'; ctx.fill();
      ctx.strokeStyle = isDark ? '#000' : '#fff'; ctx.lineWidth = 2; ctx.stroke();

      var timeStr = new Date(closestPoint.t).toLocaleTimeString();
      var txt = closestPoint.d.toFixed(1) + ' cm @ ' + timeStr;
      
      var tw = ctx.measureText(txt).width + 16;
      var tx = Math.max(padL, Math.min(cx - tw/2, w - padR - tw));
      var ty = Math.max(padT + 15, cy - 20);

      ctx.fillStyle = isDark ? '#1c2428' : '#f8fafc';
      ctx.beginPath(); ctx.roundRect(tx, ty-14, tw, 22, 4); ctx.fill();
      ctx.strokeStyle = isDark ? '#4e5b62' : '#e2e8f0'; ctx.stroke();
      
      ctx.fillStyle = isDark ? '#f4f7f7' : '#0f172a';
      ctx.fillText(txt, tx + 8, ty + 2);
    }

    ctx.fillStyle = isDark ? '#64748b' : '#94a3b8';
    ctx.fillText('now',w-padR-22,h-10);
    ctx.fillText('-60s',padL,h-10);
    requestAnimationFrame(drawTrend);
  }

  byId('btnTheme').onclick = function() {
    var html = document.documentElement;
    var isLight = html.getAttribute('data-theme') === 'light';
    html.setAttribute('data-theme', isLight ? 'dark' : 'light');
  };

  byId('btnRefresh').onclick = function() {
    var btn = this; btn.style.opacity = '0.5';
    fetchLevel(); fetchWeather();
    setTimeout(function(){ btn.style.opacity = '1'; }, 500);
  };
  
  byId('btnExport').onclick = function() {
    var csv = 'Chamber Name,Level (%),Gap (cm),Status\n';
    zones.forEach(function(z) {
      csv += z.name + ',' + round(z.level) + ',' + distanceFromLevel(z.level).toFixed(1) + ',' + state(z.level) + '\n';
    });
    var blob = new Blob([csv], {type: 'text/csv'});
    var a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = 'hydra_sanitation_log.csv';
    document.body.appendChild(a); a.click(); document.body.removeChild(a);
  };

  // Initialization
  zones[0].level=0; setMode(false);
  for(var i=0;i<14;i++){simulateStep();pushReading()}
  renderAll(false); syncRoute();
  window.addEventListener('hashchange',syncRoute);
  
  fetchWeather();
  setInterval(fetchWeather, 900000); // Check weather every 15 minutes
  
  fetchLevel(); setInterval(fetchLevel,POLL_MS);
  requestAnimationFrame(drawTrend);
})();
</script>
</body>
</html>

)HYDRA";

void handleRoot() { server.send_P(200, "text/html", PAGE); }
void handleLevel() { server.send(200, "text/plain", String(waterLevel)); }

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("\nProject HYDRA starting...");
  WiFi.begin(ssid, password);
  for (int i = 0; i < 30; i++) {
    if (WiFi.status() == WL_CONNECTED) break;
    delay(500); Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("Open: http://");
    Serial.println(WiFi.localIP());
    server.on("/", handleRoot);
    server.on("/level", handleLevel);
    server.begin();
    Serial.println("HYDRA online.");
  } else {
    Serial.println("\nWiFi FAILED.");
  }
}

void loop() {
  unsigned long now = millis();
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    float dist = readDistanceCM();
    int level = distanceToPercent(dist);
    if (level >= 0) {
      waterLevel = level;
      Serial.print("KR Puram: ");
      Serial.print(level);
      Serial.println("%");
    }
  }
  if (waterLevel >= ALERT_PERCENT) {
    if (now - lastBuzzerToggle >= BUZZER_RAPID) {
      lastBuzzerToggle = now;
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
  }
  server.handleClient();
}
