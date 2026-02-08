#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "87PROJECT-RELAY";
const char* password = "87PROJECT.AI";

// Semua pin relay yang tersedia
const int allPins[8] = {D1,D2,D5,D6,D7,D8,D3,D4};
int numRelays = 4; // default 4 relay aktif
bool relayState[8] = {false,false,false,false,false,false,false,false};

ESP8266WebServer server(80);
bool runningRelay = false;

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
#runningBtn { background:#ff00ff; color:#111; margin-bottom:20px;}
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
    server.send(200,"application/json","{\"running\":"+String(runningRelay?"true":"false")+"}");
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
    server.on("/setRelayNum",handleSetRelayNum);
    server.begin();
    randomSeed(analogRead(A0));
}

void loop(){
    server.handleClient();

    if(runningRelay){
        // Efek 8TR kiri→kanan
        for(int i=0;i<numRelays;i++){
            if(!runningRelay) break;
            digitalWrite(allPins[i],LOW); relayState[i]=true;
            delay(100);
            digitalWrite(allPins[i],HIGH); relayState[i]=false;
        }
        // Efek 8TR kanan→kiri
        for(int i=numRelays-1;i>=0;i--){
            if(!runningRelay) break;
            digitalWrite(allPins[i],LOW); relayState[i]=true;
            delay(100);
            digitalWrite(allPins[i],HIGH); relayState[i]=false;
        }
        // Efek acak 1–numRelays relay
        int idx[8]; for(int i=0;i<numRelays;i++) idx[i]=i;
        int count=random(1,numRelays+1);
        for(int i=0;i<numRelays;i++){ int r=i+random(numRelays-i); int t=idx[i]; idx[i]=idx[r]; idx[r]=t; }
        for(int i=0;i<count;i++){ int r=idx[i]; digitalWrite(allPins[r],LOW); relayState[r]=true; }
        delay(120);
        for(int i=0;i<count;i++){ int r=idx[i]; digitalWrite(allPins[r],HIGH); relayState[r]=false; }
    }
}
