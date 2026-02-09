#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

/* ================= CONFIG ================= */
#define MAX_RELAY 8
int relayPins[MAX_RELAY] = {5,4,14,12,13,15,3,1}; // D1,D2,D5,D6,D7,D8,RX,TX
int relayCount = 4; // default

const char* ssid = "87PROJECT-MATRIXV2";
const char* pass = "87PROJECT.AI";

/* ================ STATUS ================== */
bool relayState[MAX_RELAY];
bool runningMode = false;
bool blitzMode = false;

/* ============== TIMING =================== */
unsigned long runningSpeed = 120;
unsigned long lastRun = 0;

unsigned long blitzOnTime  = 90;    // flash cepat
unsigned long blitzOffTime = 2200;  // jeda lama
unsigned long blitzTimer   = 0;
bool blitzFlash = false;

/* ============== RUNNING ================== */
int runMode = 0;
#define TOTAL_MODE 30

/* ======================================== */
void allOff(){
  for(int i=0;i<relayCount;i++){
    digitalWrite(relayPins[i], HIGH);
    relayState[i] = false;
  }
}

void allOn(){
  for(int i=0;i<relayCount;i++){
    digitalWrite(relayPins[i], LOW);
    relayState[i] = true;
  }
}

/* ============== RUN MODES ================= */

// 1
void modeRandomSingle(){
  allOff();
  digitalWrite(relayPins[random(relayCount)], LOW);
}

// 2
void modeRandomDouble(){
  allOff();
  digitalWrite(relayPins[random(relayCount)], LOW);
  digitalWrite(relayPins[random(relayCount)], LOW);
}

// 3
void modePingPong(){
  static int i=0,d=1;
  allOff();
  digitalWrite(relayPins[i], LOW);
  i+=d;
  if(i==relayCount-1||i==0) d=-d;
}

// 4
void modeSnake(){
  static int i=0;
  allOff();
  digitalWrite(relayPins[i], LOW);
  if(i+1<relayCount) digitalWrite(relayPins[i+1], LOW);
  i=(i+1)%relayCount;
}

// 5
void modeOddEven(){
  static bool s=false;
  allOff();
  for(int i=0;i<relayCount;i++)
    if(i%2==s) digitalWrite(relayPins[i], LOW);
  s=!s;
}

// 6
void modeMirror(){
  static int i=0;
  allOff();
  digitalWrite(relayPins[i], LOW);
  digitalWrite(relayPins[relayCount-1-i], LOW);
  i=(i+1)%(relayCount/2+1);
}

// 7
void modeCenterOut(){
  static int i=0;
  int c=relayCount/2;
  allOff();
  if(c-i>=0) digitalWrite(relayPins[c-i], LOW);
  if(c+i<relayCount) digitalWrite(relayPins[c+i], LOW);
  i++; if(i>c) i=0;
}

// 8
void modeEdgeIn(){
  static int i=0;
  allOff();
  digitalWrite(relayPins[i], LOW);
  digitalWrite(relayPins[relayCount-1-i], LOW);
  i++; if(i>relayCount/2) i=0;
}

// 9
void modeWave(){
  static int s=0;
  allOff();
  for(int i=0;i<relayCount;i++)
    if((i+s)%3==0) digitalWrite(relayPins[i], LOW);
  s++;
}

// 10–30 (acak & chaos aman)
void modeChaos(){
  allOff();
  for(int i=0;i<relayCount;i++)
    if(random(100)<40) digitalWrite(relayPins[i], LOW);
}

/* ============== ENGINE =================== */
void runEngine(){
  if(millis()-lastRun < runningSpeed) return;
  lastRun = millis();

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
    default: modeChaos(); break;
  }

  static unsigned long modeTimer=0;
  if(millis()-modeTimer>6000){
    modeTimer=millis();
    runMode=(runMode+1)%TOTAL_MODE;
  }
}

/* ============== BLITZ FLASH =============== */
void blitzEngine(){
  unsigned long now = millis();

  if(!blitzFlash && now-blitzTimer > blitzOffTime){
    blitzTimer = now;
    blitzFlash = true;
    allOn();
  }

  if(blitzFlash && now-blitzTimer > blitzOnTime){
    allOff();
    blitzFlash = false;
    blitzTimer = now;
  }
}

/* ============== WEB ====================== */
String page(){
  String h="<html><head><meta name=viewport content='width=device-width,initial-scale=1'>";
  h+="<style>body{background:#0b0f1a;color:#fff;font-family:sans-serif;text-align:center}";
  h+="button{width:45%;padding:15px;margin:5px;font-size:18px;border-radius:12px}";
  h+="select,input{width:90%;padding:10px;margin:10px}</style></head><body>";
  h+="<h2>87PROJECT RELAY</h2>";

  h+="<select onchange=\"fetch('/set?ch='+this.value)\">";
  for(int i=2;i<=8;i++)
    h+="<option "+String(i==relayCount?"selected":"")+">"+String(i)+"</option>";
  h+="</select>";

  for(int i=0;i<relayCount;i++){
    h+="<button onclick=\"fetch('/relay?id="+String(i)+"')\">Relay "+String(i+1)+"</button>";
  }

  h+="<br><button onclick=\"fetch('/all?x=1')\">ALL ON</button>";
  h+="<button onclick=\"fetch('/all?x=0')\">ALL OFF</button>";

  h+="<br><button onclick=\"fetch('/run')\">RUNNING</button>";
  h+="<input type=range min=40 max=500 value="+String(runningSpeed)+" onchange=\"fetch('/speed?v='+this.value)\">";

  h+="<br><button onclick=\"fetch('/blitz')\">BLITZ FLASH</button>";

  h+="<footer><br>87PROJECT</footer></body></html>";
  return h;
}

/* ============== SETUP ==================== */
void setup(){
  Serial.begin(115200);
  WiFi.softAP(ssid,pass);

  for(int i=0;i<MAX_RELAY;i++){
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }

  server.on("/",[]{ server.send(200,"text/html",page()); });

  server.on("/relay",[]{
    int id=server.arg("id").toInt();
    if(id<relayCount){
      relayState[id]=!relayState[id];
      digitalWrite(relayPins[id], relayState[id]?LOW:HIGH);
    }
    server.send(200,"text/plain","OK");
  });

  server.on("/all",[]{
    server.arg("x")=="1"?allOn():allOff();
    server.send(200,"text/plain","OK");
  });

  server.on("/run",[]{
    runningMode=!runningMode;
    blitzMode=false;
    allOff();
    server.send(200,"text/plain","OK");
  });

  server.on("/blitz",[]{
    blitzMode=!blitzMode;
    runningMode=false;
    allOff();
    server.send(200,"text/plain","OK");
  });

  server.on("/speed",[]{
    runningSpeed=server.arg("v").toInt();
    server.send(200,"text/plain","OK");
  });

  server.on("/set",[]{
    relayCount=constrain(server.arg("ch").toInt(),2,8);
    allOff();
    server.send(200,"text/plain","OK");
  });

  server.begin();
}

/* ============== LOOP ===================== */
void loop(){
  server.handleClient();

  if(runningMode) runEngine();
  if(blitzMode) blitzEngine();
}
