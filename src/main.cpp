#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

/* ================= CONFIG ================= */
#define MAX_RELAY 8
#define EEPROM_SIZE 32  // simpan semua state

int relayPins[MAX_RELAY] = {5,4,14,12,13,15,3,1}; // D1 D2 D5 D6 D7 D8 RX TX
const char* ssid = "87PROJECT-MATRIXV2";
const char* pass = "87PROJECT.AI";

/* ================= STATUS ================= */
bool relayState[MAX_RELAY];
int relayCount = 4; // default
bool runningMode = false;
bool blitzMode = false;

/* ================= TIMING ================= */
unsigned long runningSpeed = 120;
unsigned long lastRun = 0;

/* ================= BLITZ ================= */
unsigned long blitzOnTime = 80;
unsigned long blitzOffTime = 2000;
unsigned long blitzTimer = 0;
bool blitzState = false;
bool doubleFlash = false;
bool toggleGroup = false; // jalur 1-2 atau 3-4

/* ================= RUNNING ================= */
#define TOTAL_MODE 50
int runMode = 0;

/* ================= BASIC ================= */
void allOff() {
  for(int i=0;i<relayCount;i++){
    digitalWrite(relayPins[i], HIGH);
    relayState[i] = false;
  }
}

void allOn() {
  for(int i=0;i<relayCount;i++){
    digitalWrite(relayPins[i], LOW);
    relayState[i] = true;
  }
}

/* ============ RUNNING MODES ============ */
void modeRandomSingle(){ allOff(); digitalWrite(relayPins[random(relayCount)], LOW); }
void modeRandomDouble(){ allOff(); digitalWrite(relayPins[random(relayCount)], LOW); digitalWrite(relayPins[random(relayCount)], LOW); }
void modePingPong(){ static int i=0,d=1; allOff(); digitalWrite(relayPins[i],LOW); i+=d; if(i<=0||i>=relayCount-1) d=-d; }
void modeSnake(){ static int i=0; allOff(); digitalWrite(relayPins[i],LOW); if(i+1<relayCount) digitalWrite(relayPins[i+1],LOW); i=(i+1)%relayCount; }
void modeOddEven(){ static bool s=false; allOff(); for(int i=0;i<relayCount;i++) if(i%2==s) digitalWrite(relayPins[i],LOW); s=!s; }
void modeMirror(){ static int i=0; allOff(); digitalWrite(relayPins[i],LOW); digitalWrite(relayPins[relayCount-1-i],LOW); i=(i+1)%(relayCount/2+1); }
void modeCenterOut(){ static int i=0; int c=relayCount/2; allOff(); if(c-i>=0) digitalWrite(relayPins[c-i],LOW); if(c+i<relayCount) digitalWrite(relayPins[c+i],LOW); i++; if(i>c) i=0; }
void modeEdgeIn(){ static int i=0; allOff(); digitalWrite(relayPins[i],LOW); digitalWrite(relayPins[relayCount-1-i],LOW); i++; if(i>relayCount/2) i=0; }
void modeWave(){ static int s=0; allOff(); for(int i=0;i<relayCount;i++) if((i+s)%3==0) digitalWrite(relayPins[i],LOW); s++; }
void modeChaos(){ allOff(); for(int i=0;i<relayCount;i++) if(random(100)<40) digitalWrite(relayPins[i],LOW); }

/* VARIANT MODES (10–49) */
void modeBurst(int density){ allOff(); for(int i=0;i<relayCount;i++) if(random(100)<density) digitalWrite(relayPins[i],LOW); }
void modeGroup(int size){ static int p=0; allOff(); for(int i=0;i<size;i++) digitalWrite(relayPins[(p+i)%relayCount],LOW); p=(p+1)%relayCount; }
void modeMirrorChaos(){ allOff(); int i=random(relayCount/2); digitalWrite(relayPins[i],LOW); digitalWrite(relayPins[relayCount-1-i],LOW); }
void modeWavePhase(int phase){ allOff(); for(int i=0;i<relayCount;i++) if((i+phase)%4==0) digitalWrite(relayPins[i],LOW); }
void modeFlashStep(){ static bool s=false; s=!s; s?allOn():allOff(); }

/* ============ RUN ENGINE ============ */
void runEngine(){
  if(millis()-lastRun<runningSpeed) return;
  lastRun=millis();

  switch(runMode){
    case 0: modeRandomSingle(); break;
    case 1: modeRandomDouble(); break;
    case 2: modePingPong(); break;
    case 3: modeSnake(); break;
    case 4: modeOddEven(); break;
    case 5: modeMirror(); break;
    case 6: modeCenterOut(); break;
    case 7: modeEdgeIn(); break;
    case 8: modeWave(); break;
    case 9: modeChaos(); break;
    case 10 ... 19: modeBurst(20+(runMode-10)*5); break;
    case 20 ... 29: modeGroup(1+(runMode-20)%3); break;
    case 30 ... 34: modeMirrorChaos(); break;
    case 35 ... 39: modeWavePhase(runMode-35); break;
    default: modeFlashStep(); break;
  }

  static unsigned long modeTimer=0;
  if(millis()-modeTimer>6000){
    modeTimer=millis();
    runMode=(runMode+1)%TOTAL_MODE;
  }
}

/* ============ BLITZ ENGINE REALISTIC ============ */
void handleBlitzFlash(){
  if(!blitzMode) return;
  unsigned long now = millis();

  if(now - blitzTimer >= (blitzState ? blitzOnTime : blitzOffTime)){
    blitzTimer = now;
    blitzState = !blitzState;

    if(blitzState){
      for(int i=0;i<relayCount;i++){
        if(toggleGroup){
          if(i<2) digitalWrite(relayPins[i], LOW); else digitalWrite(relayPins[i], HIGH);
        } else {
          if(i>=2 && i<4) digitalWrite(relayPins[i], LOW); else digitalWrite(relayPins[i], HIGH);
        }
      }
      doubleFlash = random(100) < 20; // 20% double flash
    } else {
      allOff();
      if(doubleFlash){
        delay(60);
        allOn();
        delay(70);
        allOff();
      }
      toggleGroup = !toggleGroup; // ganti jalur
    }
  }
}

/* ============ EEPROM SAVE / LOAD ============ */
void saveSettings(){
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(0, relayCount);
  EEPROM.put(1, runningSpeed);
  EEPROM.write(3, runMode);
  EEPROM.write(4, runningMode?1:0);
  EEPROM.write(5, blitzMode?1:0);
  for(int i=0;i<relayCount;i++) EEPROM.write(6+i, relayState[i]?1:0);
  EEPROM.commit();
}

void loadSettings(){
  EEPROM.begin(EEPROM_SIZE);
  relayCount=constrain(EEPROM.read(0),2,8);
  EEPROM.get(1,runningSpeed);
  runMode=EEPROM.read(3);
  runningMode = EEPROM.read(4)==1;
  blitzMode = EEPROM.read(5)==1;
  for(int i=0;i<relayCount;i++){
    relayState[i] = EEPROM.read(6+i)==1;
    digitalWrite(relayPins[i], relayState[i]?LOW:HIGH);
  }
}

/* ============ WEB UI ============ */
String page(){
  String h = "<html><head><meta name=viewport content='width=device-width,initial-scale=1'>";
  h += "<style>body{background:#0b0f1a;color:#fff;font-family:sans-serif;text-align:center}";
  h += "button{width:45%;padding:15px;margin:5px;font-size:18px;border-radius:12px}";
  h += "select,input{width:90%;padding:10px;margin:10px}</style></head><body>";
  h += "<h2>87PROJECT MATRIXV2</h2>";

  h += "<select onchange=\"fetch('/set?ch='+this.value)\">";
  for(int i=2;i<=8;i++) h+="<option "+String(i==relayCount?"selected":"")+">"+String(i)+"</option>";
  h += "</select>";

  for(int i=0;i<relayCount;i++)
    h += "<button onclick=\"fetch('/relay?id="+String(i)+"')\">Relay "+String(i+1)+"</button>";

  h += "<br><button onclick=\"fetch('/all?x=1')\">ALL ON</button>";
  h += "<button onclick=\"fetch('/all?x=0')\">ALL OFF</button>";

  h += "<br><button onclick=\"fetch('/run')\">MATRIX</button>";
  h += "<input type=range min=40 max=500 value="+String(runningSpeed)+" oninput=\"fetch('/speed?v='+this.value)\">";

  h += "<br><button onclick=\"fetch('/blitz')\">BLITZ</button>";
  h += "<br><button onclick=\"fetch('/save')\">SAVE SETTINGS</button>";
  h += "<footer><br>Created by 87PROJECT</footer></body></html>";
  return h;
}

/* ============ SETUP ============ */
void setup(){
  Serial.begin(115200);
  WiFi.softAP(ssid,pass);

  for(int i=0;i<MAX_RELAY;i++){
    pinMode(relayPins[i],OUTPUT);
    digitalWrite(relayPins[i],HIGH);
  }

  loadSettings();

  server.on("/",[]{ server.send(200,"text/html",page()); });
  server.on("/relay",[]{ int i=server.arg("id").toInt(); if(i<relayCount){ relayState[i]=!relayState[i]; digitalWrite(relayPins[i],relayState[i]?LOW:HIGH); } server.send(200,"text/plain","OK"); });
  server.on("/all",[]{ server.arg("x")=="1"?allOn():allOff(); server.send(200,"text/plain","OK"); });
  server.on("/run",[]{ runningMode=!runningMode; blitzMode=false; allOff(); server.send(200,"text/plain","OK"); });
  server.on("/blitz",[]{ blitzMode=!blitzMode; runningMode=false; allOff(); server.send(200,"text/plain","OK"); });
  server.on("/speed",[]{ runningSpeed=server.arg("v").toInt(); server.send(200,"text/plain","OK"); });
  server.on("/set",[]{ relayCount=constrain(server.arg("ch").toInt(),2,8); allOff(); server.send(200,"text/plain","OK"); });
  server.on("/save",[]{ saveSettings(); server.send(200,"text/plain","Settings Saved!"); });

  server.begin();
}

/* ============ LOOP ============ */
void loop(){
  server.handleClient();
  if(runningMode) runEngine();
  if(blitzMode && !runningMode) handleBlitzFlash();
}
