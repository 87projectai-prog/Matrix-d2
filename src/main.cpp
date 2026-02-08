#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "87PROJECT-MATRIX";
const char* password = "87PROJECT.AI";

const int relayPins[4] = {D1, D2, D5, D6};
bool relayState[4] = {false, false, false, false};

ESP8266WebServer server(80);
bool runningRelay = false;

String generateHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>87PROJECT Relay Control</title>
<style>
body { margin:0; padding:0; background:#0f0f0f; color:#00ffdd; font-family:'Arial',sans-serif; display:flex; flex-direction:column; align-items:center; min-height:100vh; }
h1 { margin:20px; text-shadow:0 0 20px #00ffdd; }
.relay-container { display:flex; gap:20px; flex-wrap:wrap; justify-content:center; margin-bottom:20px;}
.relay { background:#111; padding:25px 35px; border-radius:15px; box-shadow:0 0 20px #00ffdd; text-align:center; transition:0.3s; }
.relay:hover { transform:scale(1.05); }
.relay button { padding:12px 28px; font-size:16px; border:none; border-radius:12px; cursor:pointer; transition:0.3s; color:#111; font-weight:bold; margin-top:10px; }
.on { background:#00ffdd; } .off { background:#222; color:#00ffdd; }
#runningBtn { background:#ff00ff; color:#111; }
footer { margin-top:auto; padding:10px; text-shadow:0 0 10px #00ffdd; }
</style>
</head>
<body>
<h1>87PROJECT Relay Control</h1>
<div class="relay-container">
)rawliteral";

    for (int i = 0; i < 4; i++) {
        html += "<div class='relay'><p>Relay " + String(i + 1) + "</p>";
        html += "<button id='btn" + String(i) + "' class='" + (relayState[i] ? "on" : "off") + "' onclick='toggleRelay(" + String(i) + ")'>";
        html += relayState[i] ? "ON" : "OFF";
        html += "</button></div>";
    }

    html += R"rawliteral(
</div>
<button id="runningBtn" onclick="toggleRunning()">RUNNING RELAY</button>
<footer>87PROJECT</footer>
<script>
function toggleRelay(index){
    fetch('/toggle?index='+index)
    .then(response=>response.json())
    .then(data=>{
        let btn=document.getElementById('btn'+index);
        btn.innerText=data.state?'ON':'OFF';
        btn.className=data.state?'on':'off';
    });
}

function toggleRunning(){
    fetch('/running')
    .then(response=>response.json())
    .then(data=>{
        let btn=document.getElementById('runningBtn');
        btn.innerText = data.running ? 'STOP RUNNING' : 'RUNNING RELAY';
    });
}

setInterval(()=>{
    fetch('/status')
    .then(response=>response.json())
    .then(data=>{
        for(let i=0;i<4;i++){
            let btn=document.getElementById('btn'+i);
            btn.innerText=data.states[i]?'ON':'OFF';
            btn.className=data.states[i]?'on':'off';
        }
    });
},500);
</script>
</body>
</html>
)rawliteral";

    return html;
}

void handleRoot() { server.send(200, "text/html", generateHTML()); }

void handleToggle() {
    if (server.hasArg("index")) {
        int idx = server.arg("index").toInt();
        if (idx >= 0 && idx < 4) {
            relayState[idx] = !relayState[idx];
            digitalWrite(relayPins[idx], relayState[idx] ? LOW : HIGH);
            server.send(200, "application/json", "{\"state\":" + String(relayState[idx] ? "true" : "false") + "}");
            return;
        }
    }
    server.send(400, "text/plain", "Invalid index");
}

void handleStatus() {
    String json = "{\"states\":[";
    for (int i = 0; i < 4; i++) {
        json += relayState[i] ? "true" : "false";
        if (i < 3) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
}

void handleRunning() {
    runningRelay = !runningRelay;
    server.send(200, "application/json", "{\"running\":" + String(runningRelay ? "true" : "false") + "}");
}

void setup() {
    Serial.begin(115200);
    for (int i = 0; i < 4; i++) { pinMode(relayPins[i], OUTPUT); digitalWrite(relayPins[i], HIGH); }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500); Serial.print(".");
        if (millis() - startTime > 15000) {
            WiFi.mode(WIFI_AP_STA);
            WiFi.softAP(ssid, password);
            break;
        }
    }

    server.on("/", handleRoot);
    server.on("/toggle", handleToggle);
    server.on("/status", handleStatus);
    server.on("/running", handleRunning);
    server.begin();
    randomSeed(analogRead(A0));
}

void loop() {
    server.handleClient();

    if (runningRelay) {
        int idx[4] = {0,1,2,3};
        int count = random(1,5); // nyala 1-4 relay acak
        // acak urutan
        for(int i=0;i<4;i++){
            int r=i+random(4-i); int t=idx[i]; idx[i]=idx[r]; idx[r]=t;
        }

        // nyalakan relay acak bergantian
        for(int i=0;i<count;i++){
            int r=idx[i];
            digitalWrite(relayPins[r], LOW); relayState[r]=true;
            delay(120); // animasi cepat
            digitalWrite(relayPins[r], HIGH); relayState[r]=false;
        }
    }
}
