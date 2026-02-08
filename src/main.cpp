#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "87PROJECT-MATRIXV2";
const char* password = "87PROJECT.AI";

// Semua pin relay (maks 8)
const int allPins[8] = {D1,D2,D5,D6,D7,D8,D3,D4};
int numRelays = 4;
bool relayState[8] = {false,false,false,false,false,false,false,false};

ESP8266WebServer server(80);
bool runningRelay = false;
bool blitzMode = false;
bool allOnMode = false;

// ===== Generate halaman web =====
String generateHTML() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>87PROJECT Relay Control</title>
<style>
body{background:#0f0f0f;color:#00ffdd;font-family:Arial,sans-serif;text-align:center;}
button{padding:15px 30px;margin:10px;border-radius:10px;border:none;cursor:pointer;font-weight:bold;transition:0.3s;}
.on{background:#00ffdd;color:#111;}
.off{background:#222;color:#00ffdd;}
#runningBtn{background:#ff00ff;color:#111;margin:10px;}
#blitzBtn{background:#ff0000;color:#fff;margin:10px;}
#allOnBtn{background:#00aa00;color:#fff;margin:10px;}
select{padding:8px;border-radius:8px;margin:20px;background:#111;color:#00ffdd;border:none;}
footer{margin-top:20px;text-shadow:0 0 10px #00ffdd;}
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
<br>
)rawliteral";

    for(int i=0;i<numRelays;i++){
        html += "<button id='btn"+String(i)+"' class='"+(relayState[i]?"on":"off")+"' onclick='toggleRelay("+String(i)+")'>Relay "+String(i+1)+"</button>";
    }

    html += R"rawliteral(
<br>
<button id="runningBtn" onclick="toggleRunning()">RUNNING</button>
<button id="blitzBtn" onclick="toggleBlitz()">BLITZ PESAWAT</button>
<button id="allOnBtn" onclick="toggleAllOn()">ALL ON</button>
<footer>87PROJECT</footer>
<script>
function toggleRelay(index){
  fetch('/toggle?index='+index)
  .then(resp=>resp.json())
  .then(data=>{
    let btn=document.getElementById('btn'+index);
    btn.className=data.state?'on':'off';
  });
}

function toggleRunning(){
  fetch('/running')
  .then(resp=>resp.json())
  .then(data=>{
    document.getElementById('runningBtn').innerText = data.running?'STOP RUNNING':'RUNNING';
    if(data.running) document.getElementById('blitzBtn').innerText='BLITZ PESAWAT';
  });
}

function toggleBlitz(){
  fetch('/blitz')
  .then(resp=>resp.json())
  .then(data=>{
    document.getElementById('blitzBtn').innerText = data.blitz?'STOP BLITZ':'BLITZ PESAWAT';
    if(data.blitz) document.getElementById('runningBtn').innerText='RUNNING';
  });
}

function toggleAllOn(){
  fetch('/allon')
  .then(resp=>resp.json())
  .then(data=>{
    document.getElementById('allOnBtn').innerText = data.allOn?'ALL OFF':'ALL ON';
  });
}

function changeRelayNum(){
  let val=document.getElementById('relayNum').value;
  fetch('/setRelayNum?num='+val).then(()=>location.reload());
}

setInterval(()=>{
  fetch('/status')
  .then(resp=>resp.json())
  .then(data=>{
    for(let i=0;i<data.states.length;i++){
      let btn=document.getElementById('btn'+i);
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

// ===== Handlers =====
void handleRoot(){ server.send(200,"text/html",generateHTML()); }

void handleToggle(){
    if(server.hasArg("index")){
        int idx=server.arg("index").toInt();
        if(idx>=0 && idx<numRelays){
            relayState[idx]=!relayState[idx];
            digitalWrite(allPins[idx],relayState[idx]?LOW:HIGH);
            server.send(200,"application/json","{\"state\":"+String(relayState[idx]?"true":"false")+"}");
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
    if(runningRelay) { blitzMode=false; allOnMode=false; }
    server.send(200,"application/json","{\"running\":"+String(runningRelay?"true":"false")+"}");
}

void handleBlitz(){
    blitzMode=!blitzMode;
    if(blitzMode) { runningRelay=false; allOnMode=false; }
    server.send(200,"application/json","{\"blitz\":"+String(blitzMode?"true":"false")+"}");
}

void handleAllOn(){
    allOnMode=!allOnMode;
    if(allOnMode) { runningRelay=false; blitzMode=false; }
    for(int i=0;i<numRelays;i++){
        digitalWrite(allPins[i],allOnMode?LOW:HIGH);
        relayState[i]=allOnMode;
    }
    server.send(200,"application/json","{\"allOn\":"+String(allOnMode?"true":"false")+"}");
}

void handleSetRelayNum(){
    if(server.hasArg("num")){
        int n=server.arg("num").toInt();
        if(n>=4 && n<=8) numRelays=n;
    }
    server.send(200,"text/plain","OK");
}

// ===== Setup =====
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
    server.on("/allon",handleAllOn);
    server.on("/setRelayNum",handleSetRelayNum);
    server.begin();
    randomSeed(analogRead(A0));
}

// ===== Loop =====
void loop(){
    server.handleClient();

    // ===== RUNNING acak modern =====
    if(runningRelay){
        int idx[8]; for(int i=0;i<numRelays;i++) idx[i]=i;
        for(int i=0;i<numRelays;i++){
            int r=i+random(numRelays-i);
            int t=idx[i]; idx[i]=idx[r]; idx[r]=t;
        }
        int activeCount=random(1,numRelays+1);
        for(int i=0;i<activeCount;i++){
            int r=idx[i];
            digitalWrite(allPins[r],LOW); relayState[r]=true;
        }
        delay(90);
        for(int i=0;i<activeCount;i++){
            int r=idx[i];
            digitalWrite(allPins[r],HIGH); relayState[r]=false;
        }
    }

    // ===== BLITZ PESAWAT flash =====
    if(blitzMode){
        for(int i=0;i<numRelays;i++){
            digitalWrite(allPins[i],LOW); relayState[i]=true;
            delay(50);
            digitalWrite(allPins[i],HIGH); relayState[i]=false;
            delay(50);
        }
    }

    // ===== ALL ON =====
    if(allOnMode){
        for(int i=0;i<numRelays;i++){
            digitalWrite(allPins[i],LOW); relayState[i]=true;
        }
    }
}
