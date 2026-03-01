/*
  ESP32 Web Server Trigger → BLE Mouse Prerecorded Action
  --------------------------------------------------------
  - MOVE x y is RELATIVE — moves cursor by (x, y) pixels from current position
  - Supports negative values: ADD MOVE -100 -50 moves left 100, up 50
  - Sequence uploadable via Serial Monitor (no reflashing needed)
  - Sequence + WiFi config saved to flash (survives reboot)

  Libraries needed (Arduino Library Manager):
    - ESP32-BLE-Mouse  (T-vK)  https://github.com/T-vK/ESP32-BLE-Mouse

  Board: ESP32 Dev Module — Partition Scheme: Huge APP
  Serial Monitor: 115200 baud, line ending = Newline
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <BleMouse.h>
#include <Preferences.h>

// ─── Boot Button ─────────────────────────────────────────────────────────────
#define BOOT_PIN        0     // GPIO0 = built-in BOOT button
#define DEBOUNCE_MS   200     // ignore bounces under 200ms

// ─── Action Types (must be declared before all functions) ──────────────────────
enum ActionType { MOVE, CLICK, RCLICK, DCLICK, PRESS, RELEASE, DELAY };
struct Action { ActionType type; int x; int y; int ms; };

// ─── BLE Mouse ───────────────────────────────────────────────────────────────
BleMouse bleMouse("ESP32 BLE Mouse", "ESP32", 100);

// ─── Web Server ──────────────────────────────────────────────────────────────
WebServer server(80);

// ─── Preferences ─────────────────────────────────────────────────────────────
Preferences prefs;

// ─── WiFi config (loaded from flash, fallback to defaults) ───────────────────
String wifiSSID     = "NamaSSIDWifi";
String wifiPassword = "PasswordWifi";
String wifiIP       = "192.168.1.200";
String wifiGateway  = "192.168.1.1";

void saveWiFiConfig() {
  prefs.begin("wificfg", false);
  prefs.putString("ssid",     wifiSSID);
  prefs.putString("pass",     wifiPassword);
  prefs.putString("ip",       wifiIP);
  prefs.putString("gateway",  wifiGateway);
  prefs.end();
  Serial.println("[WIFI] Config saved to flash.");
}

void loadWiFiConfig() {
  prefs.begin("wificfg", true);
  wifiSSID     = prefs.getString("ssid",    wifiSSID);
  wifiPassword = prefs.getString("pass",    wifiPassword);
  wifiIP       = prefs.getString("ip",      wifiIP);
  wifiGateway  = prefs.getString("gateway", wifiGateway);
  prefs.end();
  Serial.println("[WIFI] Config loaded:");
  Serial.printf("       SSID    : %s\n", wifiSSID.c_str());
  Serial.printf("       IP      : %s\n", wifiIP.c_str());
  Serial.printf("       Gateway : %s\n", wifiGateway.c_str());
}

void printWiFiConfig() {
  Serial.println("─────────────────────────────────────────");
  Serial.printf("  SSID     : %s\n", wifiSSID.c_str());
  Serial.printf("  Password : %s\n", wifiPassword.c_str());
  Serial.printf("  IP       : %s\n", wifiIP.c_str());
  Serial.printf("  Gateway  : %s\n", wifiGateway.c_str());
  Serial.println("─────────────────────────────────────────");
}

// Parse "192.168.1.200" → IPAddress
IPAddress parseIP(String s) {
  IPAddress ip;
  ip.fromString(s);
  return ip;
}


#define MAX_ACTIONS 100
Action actions[MAX_ACTIONS];
int    ACTION_COUNT = 0;

const Action DEFAULT_ACTIONS[] = {
  { MOVE,    200,   0,   0  },
  { DELAY,   0,     0,  200 },
  { CLICK,   0,     0,   0  },
  { DELAY,   0,     0,  300 },
  { MOVE,    0,   200,   0  },
  { DELAY,   0,     0,  150 },
  { DCLICK,  0,     0,   0  },
  { DELAY,   0,     0,  500 },
  { MOVE,   -200, -200,  0  },
  { DELAY,   0,     0,  200 },
  { PRESS,   0,     0,   0  },
  { DELAY,   0,     0,  100 },
  { MOVE,    300,   0,   0  },
  { DELAY,   0,     0,  100 },
  { RELEASE, 0,     0,   0  },
};
const int DEFAULT_COUNT = sizeof(DEFAULT_ACTIONS) / sizeof(DEFAULT_ACTIONS[0]);

void saveSequence() {
  prefs.begin("mouse", false);
  prefs.putInt("count", ACTION_COUNT);
  prefs.putBytes("actions", actions, ACTION_COUNT * sizeof(Action));
  prefs.end();
  Serial.printf("[SAVE] %d actions saved to flash.\n", ACTION_COUNT);
}

void loadSequence() {
  prefs.begin("mouse", true);
  int saved = prefs.getInt("count", 0);
  if (saved > 0 && saved <= MAX_ACTIONS) {
    ACTION_COUNT = saved;
    prefs.getBytes("actions", actions, ACTION_COUNT * sizeof(Action));
    Serial.printf("[LOAD] Loaded %d actions from flash.\n", ACTION_COUNT);
  } else {
    ACTION_COUNT = DEFAULT_COUNT;
    memcpy(actions, DEFAULT_ACTIONS, ACTION_COUNT * sizeof(Action));
    Serial.println("[LOAD] No saved sequence — using defaults.");
  }
  prefs.end();
}

const char* actionTypeName(ActionType t) {
  switch(t) {
    case MOVE:    return "MOVE";
    case CLICK:   return "CLICK";
    case RCLICK:  return "RCLICK";
    case DCLICK:  return "DCLICK";
    case PRESS:   return "PRESS";
    case RELEASE: return "RELEASE";
    case DELAY:   return "DELAY";
    default:      return "?";
  }
}

void printSequence() {
  Serial.println("─────────────────────────────────────────");
  Serial.printf("Sequence (%d steps) — MOVE is RELATIVE:\n", ACTION_COUNT);
  for (int i = 0; i < ACTION_COUNT; i++) {
    const Action& a = actions[i];
    switch (a.type) {
      case MOVE:  Serial.printf("  [%02d] MOVE    %d  %d\n", i, a.x, a.y); break;
      case DELAY: Serial.printf("  [%02d] DELAY   %dms\n",   i, a.ms);     break;
      default:    Serial.printf("  [%02d] %s\n",             i, actionTypeName(a.type)); break;
    }
  }
  Serial.println("─────────────────────────────────────────");
}

// ─── Mouse playback ──────────────────────────────────────────────────────────
bool sequenceRunning = false;

void moveRelative(int dx, int dy) {
  while (dx != 0 || dy != 0) {
    int sx = constrain(dx, -127, 127);
    int sy = constrain(dy, -127, 127);
    bleMouse.move(sx, sy);
    dx -= sx; dy -= sy;
    delay(4);
  }
}

void playSequence() {
  if (!bleMouse.isConnected()) { Serial.println("[BLE] No host connected."); return; }
  sequenceRunning = true;
  Serial.println("[SEQ] Playback start");
  for (int i = 0; i < ACTION_COUNT; i++) {
    const Action& a = actions[i];
    switch (a.type) {
      case MOVE:    Serial.printf("[SEQ] MOVE dx=%d dy=%d\n", a.x, a.y); moveRelative(a.x, a.y); break;
      case CLICK:   Serial.println("[SEQ] CLICK");   bleMouse.click(MOUSE_LEFT);   delay(50); break;
      case RCLICK:  Serial.println("[SEQ] RCLICK");  bleMouse.click(MOUSE_RIGHT);  delay(50); break;
      case DCLICK:  Serial.println("[SEQ] DCLICK");  bleMouse.click(MOUSE_LEFT);   delay(80);
                                                     bleMouse.click(MOUSE_LEFT);   delay(50); break;
      case PRESS:   Serial.println("[SEQ] PRESS");   bleMouse.press(MOUSE_LEFT);   break;
      case RELEASE: Serial.println("[SEQ] RELEASE"); bleMouse.release(MOUSE_LEFT); delay(50); break;
      case DELAY:   Serial.printf("[SEQ] DELAY %dms\n", a.ms); delay(a.ms); break;
    }
  }
  Serial.println("[SEQ] Playback done");
  sequenceRunning = false;
}

// ─── WiFi connect ────────────────────────────────────────────────────────────
void connectWiFi() {
  IPAddress localIP  = parseIP(wifiIP);
  IPAddress gateway  = parseIP(wifiGateway);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(8, 8, 8, 8);

  WiFi.disconnect(true);
  delay(200);
  WiFi.config(localIP, gateway, subnet, dns);

  Serial.printf("[WiFi] Connecting to %s ...", wifiSSID.c_str());
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500); Serial.print("."); attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected! Open: http://%s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WiFi] Failed to connect. Check SSID/password (type WIFISHOW).");
  }
}

// ─── Serial command parser ────────────────────────────────────────────────────
String serialBuffer = "";

void handleSerialCommand(String raw) {
  raw.trim();
  String cmd = raw;
  cmd.toUpperCase();

  // ── Help ──────────────────────────────────────────────────────────────────
  if (cmd == "HELP") {
    Serial.println("─── Sequence Commands ───────────────────");
    Serial.println("  LIST");
    Serial.println("  CLEAR");
    Serial.println("  ADD MOVE dx dy    (relative, negatives OK)");
    Serial.println("  ADD CLICK / RCLICK / DCLICK");
    Serial.println("  ADD PRESS / RELEASE");
    Serial.println("  ADD DELAY ms");
    Serial.println("  SAVE              save sequence to flash");
    Serial.println("  RUN               run sequence now");
    Serial.println("─── Direct Execute (instant, not saved) ─");
    Serial.println("  DO MOVE dx dy     e.g. DO MOVE 100 -50");
    Serial.println("  DO CLICK");
    Serial.println("  DO RCLICK");
    Serial.println("  DO DCLICK");
    Serial.println("  DO PRESS");
    Serial.println("  DO RELEASE");
    Serial.println("  DO DELAY ms       e.g. DO DELAY 500");
    Serial.println("─── WiFi Commands ───────────────────────");
    Serial.println("  WIFISHOW                  show current config");
    Serial.println("  SET SSID <name>           change WiFi SSID");
    Serial.println("  SET PASS <password>       change WiFi password");
    Serial.println("  SET IP <x.x.x.x>          change static IP");
    Serial.println("  SET GATEWAY <x.x.x.x>     change gateway IP");
    Serial.println("  WIFISAVE                  save WiFi config to flash");
    Serial.println("  WIFICONNECT               reconnect with new settings");
    Serial.println("─────────────────────────────────────────");
    return;
  }

  // ── Sequence commands ─────────────────────────────────────────────────────
  if (cmd == "LIST")  { printSequence(); return; }
  if (cmd == "SAVE")  { saveSequence();  return; }
  if (cmd == "CLEAR") { ACTION_COUNT = 0; Serial.println("[CMD] Cleared."); return; }
  if (cmd == "RUN")   { playSequence(); return; }

  // ── Direct execute commands (instant, not saved to sequence) ──────────────
  // DO MOVE dx dy   — move cursor immediately, e.g. DO MOVE 100 -50
  // DO CLICK        — left click immediately
  // DO RCLICK       — right click immediately
  // DO DCLICK       — double click immediately
  // DO PRESS        — hold left button
  // DO RELEASE      — release left button
  // DO DELAY ms     — pause (useful between chained DO commands)
  if (cmd.startsWith("DO ")) {
    if (!bleMouse.isConnected()) {
      Serial.println("[DO] BLE not connected.");
      return;
    }
    String rest = cmd.substring(3);
    rest.trim();

    if (rest.startsWith("MOVE")) {
      int dx = 0, dy = 0;
      String rawUpper = raw; rawUpper.toUpperCase();
      String nums = raw.substring(rawUpper.indexOf("DO MOVE") + 8);
      nums.trim();
      if (sscanf(nums.c_str(), "%d %d", &dx, &dy) == 2) {
        moveRelative(dx, dy);
        Serial.printf("[DO] MOVE dx=%d dy=%d done.\n", dx, dy);
      } else {
        Serial.println("[DO] Usage: DO MOVE dx dy   e.g. DO MOVE 100 -50");
      }
    }
    else if (rest == "CLICK")   { bleMouse.click(MOUSE_LEFT);   delay(50); Serial.println("[DO] CLICK done."); }
    else if (rest == "RCLICK")  { bleMouse.click(MOUSE_RIGHT);  delay(50); Serial.println("[DO] RCLICK done."); }
    else if (rest == "DCLICK")  { bleMouse.click(MOUSE_LEFT);   delay(80);
                                   bleMouse.click(MOUSE_LEFT);   delay(50); Serial.println("[DO] DCLICK done."); }
    else if (rest == "PRESS")   { bleMouse.press(MOUSE_LEFT);               Serial.println("[DO] PRESS (held)."); }
    else if (rest == "RELEASE") { bleMouse.release(MOUSE_LEFT); delay(50); Serial.println("[DO] RELEASE done."); }
    else if (rest.startsWith("DELAY")) {
      int ms = 0;
      if (sscanf(rest.c_str(), "DELAY %d", &ms) == 1) {
        delay(ms);
        Serial.printf("[DO] DELAY %dms done.\n", ms);
      } else {
        Serial.println("[DO] Usage: DO DELAY ms   e.g. DO DELAY 500");
      }
    }
    else { Serial.printf("[DO] Unknown: %s\n", rest.c_str()); }
    return;
  }

  // ── WiFi commands ─────────────────────────────────────────────────────────
  if (cmd == "WIFISHOW")    { printWiFiConfig(); return; }
  if (cmd == "WIFISAVE")    { saveWiFiConfig();  return; }
  if (cmd == "WIFICONNECT") {
    Serial.println("[WiFi] Reconnecting...");
    connectWiFi();
    server.begin();
    return;
  }

  // SET SSID / PASS / IP / GATEWAY — preserve original casing for values
  if (cmd.startsWith("SET ")) {
    // Work on original raw string but uppercase just the keyword part
    String upperRaw = raw;
    upperRaw.toUpperCase();

    if (upperRaw.startsWith("SET SSID ")) {
      wifiSSID = raw.substring(9);   // preserve original case
      wifiSSID.trim();
      Serial.printf("[WIFI] SSID set to: %s\n", wifiSSID.c_str());
      Serial.println("[WIFI] Type WIFISAVE to persist, WIFICONNECT to apply.");
      return;
    }
    if (upperRaw.startsWith("SET PASS ")) {
      wifiPassword = raw.substring(9);
      wifiPassword.trim();
      Serial.printf("[WIFI] Password set to: %s\n", wifiPassword.c_str());
      Serial.println("[WIFI] Type WIFISAVE to persist, WIFICONNECT to apply.");
      return;
    }
    if (upperRaw.startsWith("SET IP ")) {
      String newIP = raw.substring(7);
      newIP.trim();
      IPAddress test;
      if (test.fromString(newIP)) {
        wifiIP = newIP;
        Serial.printf("[WIFI] Static IP set to: %s\n", wifiIP.c_str());
        Serial.println("[WIFI] Type WIFISAVE to persist, WIFICONNECT to apply.");
      } else {
        Serial.printf("[WIFI] Invalid IP: %s\n", newIP.c_str());
      }
      return;
    }
    if (upperRaw.startsWith("SET GATEWAY ")) {
      String newGW = raw.substring(12);
      newGW.trim();
      IPAddress test;
      if (test.fromString(newGW)) {
        wifiGateway = newGW;
        Serial.printf("[WIFI] Gateway set to: %s\n", wifiGateway.c_str());
        Serial.println("[WIFI] Type WIFISAVE to persist, WIFICONNECT to apply.");
      } else {
        Serial.printf("[WIFI] Invalid IP: %s\n", newGW.c_str());
      }
      return;
    }
  }

  // ── ADD sequence steps ────────────────────────────────────────────────────
  if (cmd.startsWith("ADD ")) {
    if (ACTION_COUNT >= MAX_ACTIONS) {
      Serial.printf("[CMD] Max %d actions reached.\n", MAX_ACTIONS);
      return;
    }
    String rest = cmd.substring(4);
    rest.trim();

    if (rest.startsWith("MOVE")) {
      int dx = 0, dy = 0;
      String nums = raw.substring(raw.indexOf(' ', raw.indexOf(' ') + 1) + 1); // after "ADD MOVE"
      nums.trim();
      if (sscanf(nums.c_str(), "%d %d", &dx, &dy) == 2) {
        actions[ACTION_COUNT++] = {MOVE, dx, dy, 0};
        Serial.printf("[CMD] Added MOVE dx=%d dy=%d  (step %d)\n", dx, dy, ACTION_COUNT);
      } else {
        Serial.println("[CMD] Usage: ADD MOVE dx dy   e.g. ADD MOVE -100 50");
      }
    }
    else if (rest.startsWith("DELAY")) {
      int ms = 0;
      if (sscanf(rest.c_str(), "DELAY %d", &ms) == 1) {
        actions[ACTION_COUNT++] = {DELAY, 0, 0, ms};
        Serial.printf("[CMD] Added DELAY %dms  (step %d)\n", ms, ACTION_COUNT);
      } else {
        Serial.println("[CMD] Usage: ADD DELAY ms   e.g. ADD DELAY 200");
      }
    }
    else if (rest == "CLICK")   { actions[ACTION_COUNT++] = {CLICK,   0,0,0}; Serial.printf("[CMD] Added CLICK   (step %d)\n", ACTION_COUNT); }
    else if (rest == "RCLICK")  { actions[ACTION_COUNT++] = {RCLICK,  0,0,0}; Serial.printf("[CMD] Added RCLICK  (step %d)\n", ACTION_COUNT); }
    else if (rest == "DCLICK")  { actions[ACTION_COUNT++] = {DCLICK,  0,0,0}; Serial.printf("[CMD] Added DCLICK  (step %d)\n", ACTION_COUNT); }
    else if (rest == "PRESS")   { actions[ACTION_COUNT++] = {PRESS,   0,0,0}; Serial.printf("[CMD] Added PRESS   (step %d)\n", ACTION_COUNT); }
    else if (rest == "RELEASE") { actions[ACTION_COUNT++] = {RELEASE, 0,0,0}; Serial.printf("[CMD] Added RELEASE (step %d)\n", ACTION_COUNT); }
    else { Serial.printf("[CMD] Unknown: %s\n", rest.c_str()); }
    return;
  }

  Serial.printf("[CMD] Unknown: %s  (type HELP)\n", cmd.c_str());
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        handleSerialCommand(serialBuffer);
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
    }
  }
}

// ─── Web Page ────────────────────────────────────────────────────────────────
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 BLE Mouse</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      min-height: 100vh; display: flex; flex-direction: column;
      align-items: center; justify-content: center;
      background: #0f0f0f;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      color: #eee;
    }
    h1 { font-size: 1.4rem; margin-bottom: 0.4rem; letter-spacing: 0.05em; }
    .sub { font-size: 0.85rem; color: #666; margin-bottom: 2.5rem; }
    .card {
      background: #1a1a1a; border: 1px solid #2a2a2a;
      border-radius: 16px; padding: 2.5rem 3rem;
      text-align: center; box-shadow: 0 8px 40px rgba(0,0,0,0.5);
    }
    #triggerBtn {
      background: #3b82f6; color: #fff; border: none;
      border-radius: 10px; padding: 0.9rem 2.5rem;
      font-size: 1.1rem; font-weight: 600; cursor: pointer;
      transition: background 0.15s, transform 0.1s;
    }
    #triggerBtn:hover    { background: #2563eb; }
    #triggerBtn:active   { transform: scale(0.97); }
    #triggerBtn:disabled { background: #374151; cursor: not-allowed; }
    #status    { margin-top: 1.2rem; font-size: 0.85rem; color: #9ca3af; min-height: 1.2em; }
    #stepCount { margin-top: 0.6rem; font-size: 0.78rem; color: #555; }
    .dot { display:inline-block; width:8px; height:8px; border-radius:50%; margin-right:6px; vertical-align:middle; }
    .dot.connected    { background: #22c55e; }
    .dot.disconnected { background: #ef4444; }
    #bleStatus { margin-bottom: 1.6rem; font-size: 0.85rem; color: #9ca3af; }
  </style>
</head>
<body>
  <div class="card">
    <h1>BLE Mouse Controller</h1>
    <p class="sub">ESP32 Trigger</p>
    <div id="bleStatus"><span class="dot" id="bleDot"></span><span id="bleText">Checking BLE…</span></div>
    <button id="triggerBtn" onclick="trigger()">▶ Run Sequence</button>
    <div id="status"></div>
    <div id="stepCount"></div>
  </div>
  <script>
    async function trigger() {
      const btn = document.getElementById('triggerBtn');
      const st  = document.getElementById('status');
      btn.disabled = true; st.textContent = '⏳ Running…';
      try {
        const res = await fetch('/trigger');
        const txt = await res.text();
        st.textContent = res.ok ? '✅ ' + txt : '❌ ' + txt;
      } catch(e) { st.textContent = '❌ Request failed'; }
      btn.disabled = false;
    }
    async function checkBle() {
      try {
        const d = await (await fetch('/status')).json();
        document.getElementById('bleDot').className    = 'dot ' + (d.ble ? 'connected' : 'disconnected');
        document.getElementById('bleText').textContent  = d.ble ? 'BLE connected' : 'BLE not connected — pair "ESP32 BLE Mouse" first';
        document.getElementById('stepCount').textContent = d.steps + ' steps loaded';
      } catch(e) {}
    }
    checkBle();
    setInterval(checkBle, 3000);
  </script>
</body>
</html>
)rawliteral";

// ─── HTTP Handlers ───────────────────────────────────────────────────────────
void handleRoot()     { server.send(200, "text/html", INDEX_HTML); }
void handleNotFound() { server.send(404, "text/plain", "Not found"); }

void handleTrigger() {
  if (sequenceRunning)         { server.send(409, "text/plain", "Already running");   return; }
  if (!bleMouse.isConnected()) { server.send(503, "text/plain", "BLE not connected"); return; }
  server.send(200, "text/plain", "Sequence complete");
  playSequence();
}

void handleStatus() {
  String j = "{\"ble\":";
  j += bleMouse.isConnected() ? "true" : "false";
  j += ",\"steps\":"; j += ACTION_COUNT; j += "}";
  server.send(200, "application/json", j);
}

// ─── Setup / Loop ────────────────────────────────────────────────────────────
// ─── Button state ────────────────────────────────────────────────────────────
bool     lastBtnState   = HIGH;  // HIGH = not pressed (internal pull-up)
uint32_t lastDebounceMs = 0;

void checkBootButton() {
  bool state = digitalRead(BOOT_PIN);
  if (state == LOW && lastBtnState == HIGH) {          // just pressed
    uint32_t now = millis();
    if (now - lastDebounceMs > DEBOUNCE_MS) {
      lastDebounceMs = now;
      Serial.println("[BTN] BOOT button pressed — triggering sequence.");
      if (sequenceRunning) {
        Serial.println("[BTN] Already running, ignoring.");
      } else if (!bleMouse.isConnected()) {
        Serial.println("[BTN] BLE not connected.");
      } else {
        playSequence();
      }
    }
  }
  lastBtnState = state;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BOOT_PIN, INPUT_PULLUP);  // BOOT button

  Serial.println("[BLE] Starting BLE Mouse...");
  bleMouse.begin();

  loadWiFiConfig();
  loadSequence();
  printSequence();

  connectWiFi();

  server.on("/",        handleRoot);
  server.on("/trigger", handleTrigger);
  server.on("/status",  handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.printf("[WEB] Server → http://%s\n", WiFi.localIP().toString().c_str());
  Serial.println("[READY] Type HELP for all commands.");
}

void loop() {
  readSerial();
  checkBootButton();
  if (WiFi.status() != WL_CONNECTED) { connectWiFi(); server.begin(); }
  server.handleClient();
}

/*
  ─── All Serial Commands ─────────────────────────────────────────────────────
  Serial Monitor: 115200 baud, line ending = Newline

  SEQUENCE:
    LIST
    CLEAR
    ADD MOVE dx dy        e.g.  ADD MOVE -100 50
    ADD CLICK / RCLICK / DCLICK
    ADD PRESS / RELEASE
    ADD DELAY ms          e.g.  ADD DELAY 200
    SAVE
    RUN

  WIFI:
    WIFISHOW              print current config
    SET SSID <name>       e.g.  SET SSID MyNetwork
    SET PASS <password>   e.g.  SET PASS MyPassword123
    SET IP <x.x.x.x>      e.g.  SET IP 192.168.1.150
    SET GATEWAY <x.x.x.x> e.g.  SET GATEWAY 192.168.1.1
    WIFISAVE              persist config to flash
    WIFICONNECT           reconnect with new settings
*/
