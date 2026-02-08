#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

/* ===== WIFI ===== */
const char* AP_SSID = "87PROJECT-MATRIXV2";
const char* AP_PASS = "87PROJECT.AI";

/* ===== RELAY ===== */
int relayPins[8] = {D1, D2, D3, D4, D5, D6, D7, D8};
bool relayState[8];
int relayCount = 4;   // default

/* ===== MODE ===== */
bool runningMode = false;
bool blitzMode   = false;

/* ===== SPEED ===== */
unsigned long runningSpeed = 120;

/* ===== TIMER ===== */
unsigned long autoTimer = 0;
int autoStep = 0;

/* ===== UTIL ===== */
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

/* ===== RUNNING RANDOM ===== */
void runRandomFast() {
  static unsigned long t = 0;
  if (millis() - t < runningSpeed) return;
  t = millis();

  allOff();
  int r = random(relayCount);
  relayState[r] = true;
  digitalWrite(relayPins[r], LOW);
}

/* ===== BLITZ PESAWAT ===== */
void handleBlitz() {
  static unsigned long t = 0;
  static bool flash = false;

  if (millis() - t < 2000) return;
  t = millis();

  flash = !flash;
  for (int i = 0; i < relayCount; i++)
    digitalWrite(relayPins[i], flash ? LOW : HIGH);
}

/* ===== RUNNING MASTER ===== */
void handleRunning() {
  if (!runningMode) return;

  if (millis() - autoTimer < 7000) {
    runRandomFast();
    return;
  }

  autoTimer = millis();
  autoStep = (autoStep + 1) % 3;

  allOff();
  blitzMode = false;

  if (autoStep == 1) blitzMode = true;
}

/* ===== WEB PAGE ===== */
String page() {
  String h = R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
body{margin:0;padding:20px;background:#0a0f1c;color:#fff;font-family:sans-serif}
h2{text-align:center;margin-bottom:10px}
button,select{width:100%;padding:14px;margin:6px 0;border:0;border-radius:10px;
background:#1f6cff;color:white;font-size:16px}
select{background:#1f2a48}
label{font-size:14px;opacity:.7}
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

<label>Running Speed</label>
<input type="range" min="40" max="500" value="120"
oninput="fetch('/runspeed?v='+this.value)">

<label>Jumlah Relay</label>
<select onchange="fetch('/count?v='+this.value).then(()=>location.reload())">
<option value="2">2 Channel</option>
<option value="3">3 Channel</option>
<option value="4" selected>4 Channel</option>
<option value="5">5 Channel</option>
<option value="6">6 Channel</option>
<option value="7">7 Channel</option>
<option value="8">8 Channel</option>
</select>

<div class="footer">87PROJECT</div>
</body></html>
)rawliteral";

  return h;
}

/* ===== SETUP ===== */
void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 8; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }

  WiFi.softAP(AP_SSID, AP_PASS);

  server.on("/", [](){ server.send(200, "text/html", page()); });

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
    int v = server.arg("v").toInt();
    if (v < 2) v = 2;
    if (v > 8) v = 8;
    relayCount = v;
    allOff();
    server.send(200,"text/plain","CNT");
  });

  server.begin();
}

/* ===== LOOP ===== */
void loop() {
  server.handleClient();

  if (runningMode) handleRunning();
  if (blitzMode) handleBlitz();
}
