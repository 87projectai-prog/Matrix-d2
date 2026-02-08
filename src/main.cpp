#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

// ================= WIFI =================
const char* ssid = "87PROJECT-MATRIXV2";
const char* password = "87PROJECT.AI";

// =============== RELAY ==================
int allPins[8] = {D1, D2, D3, D4, D5, D6, D7, D8};
bool relayState[8] = {false};
int numRelays = 4;

// =============== MODE ===================
bool runningMode = false;
bool blitzMode = false;
bool allOnMode = false;

// =============== SPEED ==================
int runningSpeed = 80;        // ms
int blitzInterval = 2000;     // ms
unsigned long lastBlitz = 0;
bool blitzState = false;

// ================= HTML =================
String page() {
  String btns = "";
  for (int i = 0; i < numRelays; i++)
    btns += "<button onclick=\"toggleRelay(" + String(i) + ")\">Relay " + String(i+1) + "</button>";

  return R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>87PROJECT RELAY</title>
<style>
body{background:#0a0a0a;color:#0ff;font-family:sans-serif;text-align:center}
button{padding:14px 22px;margin:6px;font-size:16px;
background:#111;border:2px solid #0ff;color:#0ff;border-radius:10px}
button:hover{background:#0ff;color:#000}
input[type=range]{width:80%}
footer{margin-top:20px;color:#555}
</style>
</head>
<body>

<h2>87PROJECT RELAY CONTROL</h2>

<select onchange="setRelayCount(this.value)">
<option>4</option><option>5</option><option>6</option><option>7</option><option>8</option>
</select>

<div id="relays">RELAY_BTNS</div>

<hr>

<button onclick="toggleRunning()">RUNNING</button>
<button onclick="toggleBlitz()">BLITZ PESAWAT</button>
<button onclick="toggleAll()">ALL ON / OFF</button>

<h4>RUNNING SPEED</h4>
<input type="range" min="30" max="300" value="80"
oninput="setRunning(this.value)">
<span id="rs">80</span> ms

<h4>BLITZ INTERVAL</h4>
<input type="range" min="500" max="5000" value="2000"
oninput="setBlitz(this.value)">
<span id="bs">2000</span> ms

<footer>87PROJECT</footer>

<script>
function toggleRelay(i){fetch('/relay?i='+i)}
function toggleRunning(){fetch('/running')}
function toggleBlitz(){fetch('/blitz')}
function toggleAll(){fetch('/allon')}
function setRelayCount(n){fetch('/count?n='+n).then(()=>location.reload())}

function setRunning(v){
  document.getElementById('rs').innerText = v;
  fetch('/speedRunning?v='+v);
}
function setBlitz(v){
  document.getElementById('bs').innerText = v;
  fetch('/speedBlitz?v='+v);
}
</script>
</body></html>
)rawliteral";
}

// ================= RUNNING =================
void handleRunning() {
  if (!runningMode) return;

  int r = random(numRelays);
  digitalWrite(allPins[r], LOW);    // ON
  delay(runningSpeed);
  digitalWrite(allPins[r], HIGH);   // OFF
}

// ================= BLITZ =================
void handleBlitz() {
  if (!blitzMode) return;

  unsigned long now = millis();
  if (now - lastBlitz >= blitzInterval) {
    lastBlitz = now;
    blitzState = !blitzState;
    for (int i = 0; i < numRelays; i++)
      digitalWrite(allPins[i], blitzState ? LOW : HIGH);
  }
}

// ================= SETUP =================
void setup() {
  randomSeed(micros());
  WiFi.softAP(ssid, password);

  for (int i = 0; i < 8; i++) {
    pinMode(allPins[i], OUTPUT);
    digitalWrite(allPins[i], HIGH); // OFF default
  }

  server.on("/", []() {
    String html = page();
    String buttons = "";
    for (int i = 0; i < numRelays; i++)
      buttons += "<button onclick=\"toggleRelay(" + String(i) + ")\">Relay " + String(i+1) + "</button>";
    html.replace("RELAY_BTNS", buttons);
    server.send(200, "text/html", html);
  });

  server.on("/relay", []() {
    int i = server.arg("i").toInt();
    relayState[i] = !relayState[i];
    digitalWrite(allPins[i], relayState[i] ? LOW : HIGH);
    server.send(200, "text/plain", "OK");
  });

  server.on("/running", []() {
    runningMode = !runningMode;
    blitzMode = false;
    server.send(200, "text/plain", "RUNNING");
  });

  server.on("/blitz", []() {
    blitzMode = !blitzMode;
    runningMode = false;
    lastBlitz = millis();
    server.send(200, "text/plain", "BLITZ");
  });

  server.on("/allon", []() {
    allOnMode = !allOnMode;
    runningMode = false;
    blitzMode = false;
    for (int i = 0; i < numRelays; i++)
      digitalWrite(allPins[i], allOnMode ? LOW : HIGH);
    server.send(200, "text/plain", "ALL");
  });

  server.on("/count", []() {
    numRelays = constrain(server.arg("n").toInt(), 4, 8);
    server.send(200, "text/plain", "OK");
  });

  server.on("/speedRunning", []() {
    runningSpeed = constrain(server.arg("v").toInt(), 30, 300);
    server.send(200, "text/plain", "OK");
  });

  server.on("/speedBlitz", []() {
    blitzInterval = constrain(server.arg("v").toInt(), 500, 5000);
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

// ================= LOOP =================
void loop() {
  server.handleClient();
  handleRunning();
  handleBlitz();
}
