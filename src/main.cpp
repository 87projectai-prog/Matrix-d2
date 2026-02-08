#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "87PROJECT-MATRIXV2";
const char* password = "87PROJECT.AI";

// Semua pin relay yang tersedia
const int allPins[8] = {D1,D2,D5,D6,D7,D8,D3,D4};
int numRelays = 4; // default 4 relay aktif
bool relayState[8] = {false,false,false,false,false,false,false,false};

ESP8266WebServer server(80);
bool runningRelay = false;
bool blitzMode = false;

// Generate halaman web
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
#runningBtn { background:#ff00ff; color:#111; margin-bottom:10px;}
#blitzBtn { background:#ff0000; color:#fff; margin-bottom:20px;}
footer { margin-top:auto; padding:10px; text-shadow:0 0 10px #00ffdd; }
select { padding:8px; border-radius:8px; margin-bottom:20px; background:#111; color:#00ffdd; border:none;}
</style>
</head>
<body>
<h1>87PROJECT Relay Control</h1>
<label>Jumlah Relay:</label>
<select id="relayNum" onchange="changeRelayNum()">
  <option value="4")rawliteral" + String(numRelays==4?" selected":"") + R"rawliteral(>4</option>
  <option value="5")rawliteral" + String(numRelays==5?" selected":"") + R"rawliteral(>5</option>
  <option value="6")rawliteral" + String(numRelays==6?" selected":"") + R"rawliteral(>6</option>
  <option value="7")rawliteral" + String(numRelays==7?" selected":"") + R"rawliteral(>7</option>
  <option value="8")rawliteral" + String(numRelays==8?" selected":"") + R"rawliteral(>8</option>
</select>
<div class="relay-container">
)rawliteral";

    for(int i=0;i<numRelays;i++){
        html += "<div class='relay'><p>Relay "+String(i+1)+"</p>";
        html += "<button id='btn"+String(i)+"' class='" + (relayState[i]?"on":"off") + "' onclick='toggleRelay("+String(i)+")'>";
        html += relayState[i]?"ON":"OFF";
        html += "</button></div>";
    }

    html += R"rawliteral(
</div>
<button id="runningBtn" onclick="toggleRunning()">RUNNING RELAY</button>
<button id="blitzBtn" onclick="toggleBlitz()">BLITZ PESAWAT</button>
<footer>87PROJECT</footer>
<script>
function toggleRelay(index){
    fetch('/toggle?index='+index)
    .then(resp=>resp.json())
    .then(data=>{
        let btn=document.getElementById('btn'+index);
        btn.innerText=data.state?'ON':'OFF';
        btn.className=data.state?'on':'off';
    });
}

function toggleRunning(){
    fetch('/running')
    .then(resp=>resp.json())
    .then(data=>{
        document.getElementById('runningBtn').innerText = data.running?'STOP RUNNING':'RUNNING RELAY';
        // matikan blitz jika running aktif
        if(data.running) document.getElementById('blitzBtn').innerText='BLITZ PESAWAT';
    });
}

function toggleBlitz(){
    fetch('/blitz')
    .then(resp=>resp.json())
    .then(data=>{
        document.getElementById('blitzBtn').innerText = data.blitz ? 'STOP BLITZ' : 'BLITZ PESAWAT';
        // matikan running jika blitz aktif
        if(data.blitz) document.getElementById('runningBtn').innerText='RUNNING RELAY';
    });
}

function changeRelayNum(){
    let val=document.getElementById('relayNum').value;
    fetch('/setRelayNum?num='+val)
    .then(()=>location.reload());
}

setInterval(()=>{
    fetch('/status')
    .then(resp=>resp.json())
    .then(data=>{
        for(let i=0;i<data.states.length;i++){
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

// Handler
void handleRoot(){ server.send(200,"text/html",generateHTML()); }

void handleToggle(){
    if(server.hasArg("index")){
        int idx=server.arg("index").toInt();
        if(idx>=0 && idx<numRelays){
            relayState[idx]=!relayState[idx];
            digitalWrite(allPins[idx],relayState[idx]?LOW:HIGH);
            server.send(200,"application/json","{\"state\":" + String(relayState[idx]?"true":"false")+"}");
            return;
        }
    }
    server.send(400,"text/plain","Invalid index");
}

void handleStatus(){
    String json="{\"states\":[";
    for(int i=0;i<numRelays;i++){
        json += relayState[i]?"true":"false";
        if(i<numRelays-1) json+=",";
    }
    json += "]}";
    server.send(200,"application/json",json);
}

void handleRunning(){
    runningRelay=!runningRelay;
    if(runningRelay) blitzMode=false; // matikan blitz kalau running aktif
    server.send(200,"application/json","{\"running\":"+String(runningRelay?"true":"false")+"}");
}

void handleBlitz(){
    blitzMode=!blitzMode;
    if(blitzMode) runningRelay=false; // matikan running kalau blitz aktif
    server.send(200,"application/json","{\"blitz\":"+String(blitzMode?"true":"false")+"}");
}

void handleSetRelayNum(){
    if(server.hasArg("num")){
        int n=server.arg("num").toInt();
        if(n>=4 && n<=8) numRelays=n;
    }
    server.send(200,"text/plain","OK");
}

void setup(){
    Serial.begin(115200);
    for(int i=0;i<8;i++){ pinMode(allPins[i],OUTPUT); digitalWrite(allPins[i],HIGH); }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid,password);
    unsigned long start=millis();
    while(WiFi.status()!=WL_CONNECTED){
        delay(500); Serial.print(".");
        if(millis()-start>15000){ WiFi.mode(WIFI_AP_STA); WiFi.softAP(ssid,password); break;}
    }

    server.on("/",handleRoot);
    server.on("/toggle",handleToggle);
    server.on("/status",handleStatus);
    server.on("/running",handleRunning);
    server.on("/blitz",handleBlitz);
    server.on("/setRelayNum",handleSetRelayNum);
    server.begin();
    randomSeed(analogRead(A0));
}

void loop(){
    server.handleClient();

    // ===== RUNNING relay (pola tetap + acak) =====
    if(runningRelay){
        // pola tetap
        int fixedPatterns[][8] = {{0,1,2,3,4,5,6,7},{7,6,5,4,3,2,1,0}};
        int numFixed=2;
        for(int p=0;p<numFixed;p++){
            if(!runningRelay) break;
            for(int i=0;i<numRelays;i++){
                int r = fixedPatterns[p][i];
                digitalWrite(allPins[r],LOW); relayState[r]=true;
                delay(100);
                digitalWrite(allPins[r],HIGH); relayState[r]=false;
            }
        }

        // beberapa pola acak predefined
        int randomPatterns[][8] = {
            {1,2,0,3,4,5,6,7},
            {2,3,0,1,4,5,6,7},
            {3,2,1,0,4,5,6,7},
            {0,3,1,2,4,5,6,7}
        };
        int sel = random(0,4);
        for(int i=0;i<numRelays;i++){
            int r = randomPatterns[sel][i];
            digitalWrite(allPins[r],LOW); relayState[r]=true;
            delay(90);
            digitalWrite(allPins[r],HIGH); relayState[r]=false;
        }

        // pola acak tambahan
        int idx[8]; for(int i=0;i<numRelays;i++) idx[i]=i;
        int count=random(1,numRelays+1);
        for(int i=0;i<numRelays;i++){
            int r=i+random(numRelays-i);
            int t=idx[i]; idx[i]=idx[r]; idx[r]=t;
        }
        for(int i=0;i<count;i++){
            int r=idx[i];
            digitalWrite(allPins[r],LOW); relayState[r]=true;
        }
        delay(120);
        for(int i=0;i<count;i++){
            int r=idx[i];
            digitalWrite(allPins[r],HIGH); relayState[r]=false;
        }
    }

    // ===== BLITZ PESAWAT =====
    if(blitzMode){
        int idx[8]; for(int i=0;i<numRelays;i++) idx[i]=i;
        for(int i=0;i<numRelays;i++){
            int r=i+random(numRelays-i);
            int t=idx[i]; idx[i]=idx[r]; idx[r]=t;
        }
        for(int i=0;i<numRelays;i++){
            int r=idx[i];
            digitalWrite(allPins[r],LOW); relayState[r]=true;
            delay(50); // sangat cepat
            digitalWrite(allPins[r],HIGH); relayState[r]=false;
        }
    }
}
