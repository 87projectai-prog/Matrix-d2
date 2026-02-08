#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

/* ================= CONFIG ================= */
const char* apSSID = "87PROJECT-RELAY";
const char* apPASS = "87PROJECT.AI";

/* Relay ACTIVE LOW */
int relayPins[8] = {D1, D2, D3, D4, D5, D6, D7, D8};
int relayCount = 4;   // default

/* ================= STATE ================= */
bool relayState[8];
bool runningMode = false;
bool blitzMode   = false;

unsigned long runningSpeed = 120;   // slider running
unsigned long autoTimer = 0;
int autoStep = 0;

/* ================= UTILS ================= */
void allOff() {
  for (int i = 0; i < relayCount; i++) {
    relayState[i] = false;
    digitalWrite(relayPins[i], HIGH);
  }
}

void allOn() {
  for (int i = 0; i < relayCount; i++) {
    relayState[i] = true;
    digitalWrite(relayPins[i], LOW);
  }
}

/* ================= RUNNING RANDOM ================= */
void runRandomFast() {
  static unsigned long t = 0;
  if (millis() - t < runningSpeed) return;
  t = millis();

  allOff();
  int r = random(relayCount);
  relayState[r] = true;
  digitalWrite(relayPins[r], LOW);
}

/* ================= POLA CUSTOM ================= */
void runCustomPattern() {
  int patterns[][4] = {
    {0,1,2,3},
    {3,2,1,0},
    {1,2,0,3},
    {2,3,0,1}
  };

  for (int p = 0; p < 4; p++) {
    allOff();
    for (int i = 0; i < 4 && i < relayCount; i++) {
      digitalWrite(relayPins[patterns[p][i]], LOW);
      delay(runningSpeed);
      digitalWrite(relayPins[patterns[p][i]], HIGH);
    }
  }
}

/* ================= BLITZ PESAWAT ================= */
void handleBlitz() {
  static unsigned long t = 0;
  static bool flash = false;

  if (millis() - t < 2000) return;  // 2 detik
  t = millis();

  flash = !flash;
  for (int i = 0; i < relayCount; i++)
    digitalWrite(relayPins[i], flash ? LOW : HIGH);
}

/* ================= RUNNING MASTER ================= */
void handleRunning() {
  if (!runningMode) return;

  if (millis() - autoTimer < 7000) {
    runRandomFast();
    return;
  }

  autoTimer = millis();
  autoStep = (autoStep + 1) % 4;

  if (autoStep == 0) {
    blitzMode = false;
  }
  if (autoStep == 1) {
    runCustomPattern();
  }
  if (autoStep == 2) {
    blitzMode = true;
  }
  if (autoStep == 3) {
    blitzMode = false;
  }
}

/* ================= WEB PAGE ================= */
String page() {
  String h = R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{background:#0a0f1c;color:#fff;font-family:sans-serif;margin:0;padding:20px}
h2{text-align:center}
button{width:100%;padding:14px;margin:6px 0;border:0;border-radius:10px;
background:#1f6cff;color:white;font-size:16px}
.slider{margin:12px 0}
input[type=range]{width:100%}
.footer{text-align:center;margin-top:20px;opacity:.6}
</style>
</head>
<body>
<h2>87PROJECT RELAY</h2>
)rawliteral";

  for (int i = 0; i < relayCount; i++) {
    h += "<button onclick=\"fetch('/toggle?id=" + String(i) + "')\">Relay " + String(i+1) + "</button>";
  }

  h += R"rawliteral(
<button onclick="fetch('/run')">RUNNING</button>
<button onclick="fetch('/blitz')">BLITZ PESAWAT</button>
<button onclick="fetch('/allon')">ALL ON</button>
<button onclick="fetch('/alloff')">ALL OFF</button>

<div class="slider">
<label>Running Speed</label>
<input type="range" min="40" max="500" value="120"
oninput="fetch('/runspeed?v='+this.value)">
</div>

<div class="slider">
<label>Jumlah Relay</label>
<input type="range" min="4" max="8" step="4" value="4"
oninput="fetch('/count?v='+this.value)">
</div>

<div class="footer">87PROJECT</div>
</body></html>
)rawliteral";
  return h;
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 8; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }

  WiFi.softAP(apSSID, apPASS);

  server.on("/", [](){ server.send(200,"text/html",page()); });

  server.on("/toggle", [](){
    int id = server.arg("id").toInt();
    if (id < relayCount) {
      relayState[id] = !relayState[id];
      digitalWrite(relayPins[id], relayState[id] ? LOW : HIGH);
    }
    server.send(200,"text/plain","OK");
  });

  server.on("/run", [](){
    runningMode = !runningMode;
    blitzMode = false;
    allOff();
    server.send(200,"text/plain","RUN");
  });

  server.on("/blitz", [](){
    blitzMode = !blitzMode;
    runningMode = false;
    allOff();
    server.send(200,"text/plain","BLITZ");
  });

  server.on("/allon", [](){ allOn(); server.send(200,"text/plain","ON"); });
  server.on("/alloff", [](){ allOff(); server.send(200,"text/plain","OFF"); });

  server.on("/runspeed", [](){
    runningSpeed = constrain(server.arg("v").toInt(), 40, 500);
    server.send(200,"text/plain","SPD");
  });

  server.on("/count", [](){
    relayCount = server.arg("v").toInt();
    if (relayCount != 4 && relayCount != 8) relayCount = 4;
    allOff();
    server.send(200,"text/plain","CNT");
  });

  server.begin();
}

/* ================= LOOP ================= */
void loop() {
  server.handleClient();

  if (runningMode) handleRunning();
  if (blitzMode) handleBlitz();
}
