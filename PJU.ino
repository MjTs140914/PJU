/*
  Smart Relay Scheduler (ESP32 + DS3231 RTC)
  ------------------------------------------
  Fitur:
  - Kontrol relay berbasis jadwal ON/OFF dengan RTC DS3231
  - Kontrol manual via web (Hidupkan/Matikan relay)
  - Tema web interface: Terang 🌞 / Gelap 🌙
  - Sinkronisasi waktu RTC dari HP/Browser
  - Penyimpanan pengaturan (jadwal, WiFi SSID & password, tema, last ON/OFF)
    menggunakan Preferences (NVS) di ESP32
  - mDNS untuk akses mudah via http://alamat_anda.local ganti oleh anda
  - Riwayat waktu terakhir relay ON/OFF tersimpan

  Hardware:
  - Board   : ESP32
  - RTC     : DS3231 (I2C SDA=21, SCL=22)
  - Relay   : Pin 16 (aktif LOW)

  Library Dependencies:
  - Wire.h
  - RTClib.h (Adafruit)
  - WiFi.h
  - AsyncTCP.h
  - ESPAsyncWebServer.h
  - Preferences.h
  - ESPmDNS.h

  Author   : MjTs-140914™
  Tahun    : 2025
  License  : Open-source / bebas digunakan dengan mencantumkan kredit
*/


#include <Wire.h>
#include "RTClib.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>

RTC_DS3231 rtc;
Preferences preferences;

String ap_ssid = "SSID_Anda";
String ap_password = "Sandi_Anda"; // Minimal sandi 8 angka

const int relayPin = 16; // relay aktif LOW
AsyncWebServer server(80);

bool relayState = false;         
bool manualOverride = false;
bool scheduleEnabled = true;     
bool darkTheme = false;          // tema default terang
unsigned long manualTime = 0;
const unsigned long manualDuration = 30000; // 30 detik

int onHour = 18;
int onMinute = 0;
int offHour = 6;
int offMinute = 0;

String lastOn = "--:--:-- --/--/----";
String lastOff = "--:--:-- --/--/----";

// ==== Helper ====
String formatTime(int h, int m){
  char buf[6];
  sprintf(buf,"%02d:%02d",h,m);
  return String(buf);
}
String formatDateTime(DateTime t){
  char buf[25];
  sprintf(buf,"%02d:%02d:%02d %02d/%02d/%04d",t.hour(),t.minute(),t.second(),t.day(),t.month(),t.year());
  return String(buf);
}

void saveSchedule(){
  preferences.begin("relay-schedule", false);
  preferences.putUInt("onHour", onHour);
  preferences.putUInt("onMinute", onMinute);
  preferences.putUInt("offHour", offHour);
  preferences.putUInt("offMinute", offMinute);
  preferences.end();
}
void saveToggle(){
  preferences.begin("relay-schedule", false);
  preferences.putBool("schedule", scheduleEnabled);
  preferences.end();
}
void saveWiFiConfig(){
  preferences.begin("relay-schedule", false);
  preferences.putString("ap_ssid", ap_ssid);
  preferences.putString("ap_password", ap_password);
  preferences.end();
}
void saveLastTimes(){
  preferences.begin("relay-schedule", false);
  preferences.putString("lastOn", lastOn);
  preferences.putString("lastOff", lastOff);
  preferences.end();
}
void saveTheme(){
  preferences.begin("relay-schedule", false);
  preferences.putBool("darkTheme", darkTheme);
  preferences.end();
}

void loadSettings(){
  preferences.begin("relay-schedule", true);
  onHour = preferences.getUInt("onHour", 18);
  onMinute = preferences.getUInt("onMinute", 0);
  offHour = preferences.getUInt("offHour", 6);
  offMinute = preferences.getUInt("offMinute", 0);
  scheduleEnabled = preferences.getBool("schedule", true);
  ap_ssid = preferences.getString("ap_ssid", ap_ssid);
  ap_password = preferences.getString("ap_password", ap_password);
  lastOn = preferences.getString("lastOn", lastOn);
  lastOff = preferences.getString("lastOff", lastOff);
  darkTheme = preferences.getBool("darkTheme", false);
  preferences.end();
}

// ==== Relay ====
void setRelay(bool turnOn){
  if(turnOn){
    digitalWrite(relayPin, LOW);
    if(!relayState){
      DateTime now = rtc.now();
      lastOn = formatDateTime(now);
      saveLastTimes();
    }
    relayState = true;
  } else {
    digitalWrite(relayPin, HIGH);
    if(relayState){
      DateTime now = rtc.now();
      lastOff = formatDateTime(now);
      saveLastTimes();
    }
    relayState = false;
  }
}

void checkSchedule() {
  DateTime now = rtc.now();
  int hour = now.hour();
  int minute = now.minute();

  if(scheduleEnabled){
    bool shouldBeOn;
    if(onHour == offHour && onMinute == offMinute){
      shouldBeOn = true; // selalu ON
    }
    else if(onHour < offHour || (onHour == offHour && onMinute < offMinute)){
      shouldBeOn = (hour > onHour || (hour==onHour && minute>=onMinute)) &&
                   (hour < offHour || (hour==offHour && minute<offMinute));
    } else {
      shouldBeOn = (hour > onHour || (hour==onHour && minute>=onMinute)) ||
                   (hour < offHour || (hour==offHour && minute<offMinute));
    }

    if(!manualOverride){
      if(shouldBeOn && !relayState) setRelay(true);
      if(!shouldBeOn && relayState) setRelay(false);
    } else {
      if(millis() - manualTime > manualDuration){
        manualOverride = false;
        if(shouldBeOn) setRelay(true); else setRelay(false);
      }
    }
  } else {
    if(manualOverride && millis() - manualTime > manualDuration){
      manualOverride = false;
    }
  }
}

String htmlPage(){
  String page = R"rawliteral(<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>Judul Punya Anda</title>
<style>
body { font-family:Arial, sans-serif; text-align:center; background:#f4f4f4; margin:0; padding:0; transition:0.3s; }
body.dark { background:#121212; color:#eaeaea; }
h2 { background:#0077cc; color:white; padding:15px; margin:0; }
.card { background:white; padding:15px; margin:15px; border-radius:12px; box-shadow:0 2px 6px rgba(0,0,0,0.2); transition:0.3s; }
body.dark .card { background:#1e1e1e; color:#eaeaea; }
input[type='time'] {
  padding:8px; margin:6px; font-size:16px; border-radius:6px; border:1px solid #ccc;
  background:#fff; color:#000; transition:0.3s;
}
body.dark input[type='time'] { background:#2a2a2a; color:#eaeaea; border:1px solid #555; }
.btn-blue {
  background:#0077cc; color:white; padding:10px 20px;
  border:none; border-radius:6px; cursor:pointer; font-size:16px;
}
.btn-blue:hover { background:#005fa3; }
.btn-blue:disabled { background:#aaa; cursor:not-allowed; }
button.on { background:green; color:white; padding:12px 20px; border:none; border-radius:8px; cursor:pointer; font-size:16px; }
button.off { background:red; color:white; padding:12px 20px; border:none; border-radius:8px; cursor:pointer; font-size:16px; }
button.reset { background:orange; color:white; padding:12px 20px; border:none; border-radius:8px; cursor:pointer; font-size:16px; }
.switch { position:relative; display:inline-block; width:160px; height:40px; margin:10px; }
.switch input { display:none; }
.slider { position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0;
background:#ccc; transition:.4s; border-radius:34px; display:flex; align-items:center; justify-content:center;
font-size:14px; font-weight:bold; color:white; padding:0 10px; }
.slider:before { position:absolute; content:""; height:32px; width:32px;
left:4px; bottom:4px; background:white; transition:.4s; border-radius:50%; }
input:checked + .slider { background:#0077cc; }
input:checked + .slider:before { transform:translateX(120px); }
#toast { visibility:hidden; min-width:200px; background:#333; color:#fff;
text-align:center; border-radius:8px; padding:12px; position:fixed; z-index:1; left:50%; bottom:30px;
font-size:16px; transform:translateX(-50%); opacity:0; transition:opacity 0.5s; }
#toast.show { visibility:visible; opacity:1; }
.footer { font-size:12px; color:#777; margin:20px 0; }
body.dark .footer { color:#aaa; }
</style>
</head>
<body %DARKCLASS%>
<h2>💡 Judul Punya Anda</h2>
<div class="card">
  <h3>📊 Status</h3>
  <p>Status Lampu: <b><span id="status">OFF</span></b></p>
  <p>Waktu RTC: <b><span id="rtc">00:00:00</span></b></p>
  <p>📅 Terakhir Hidup: <b><span id="lastOn">--:--:-- --/--/----</span></b></p>
  <p>📅 Terakhir Mati: <b><span id="lastOff">--:--:-- --/--/----</span></b></p>
</div>
<div class="card">
  <h3>⏰ Jadwal</h3>
  <form id="scheduleForm" onsubmit="event.preventDefault(); saveSchedule();">
    Lampu ON: <input type="time" id="onTimeInput" value="18:00" onfocus="dirtyOn=true" onblur="dirtyOn=false"><br><br>
    Lampu OFF: <input type="time" id="offTimeInput" value="06:00" onfocus="dirtyOff=true" onblur="dirtyOff=false"><br><br>
    <input type="submit" id="saveBtn" value="💾 Simpan Jadwal" class="btn-blue" disabled>
  </form><br>
  <label class="switch">
    <input type="checkbox" id="toggleSchedule" onchange="toggleSchedule(this.checked)">
    <span class="slider" id="labelSchedule">Jadwal OFF</span>
  </label>
</div>
<div class="card">
  <h3>🎛️ Kontrol Manual</h3>
  <button class="on" onclick="lampOn()">🟢 Hidupkan</button>
  <button class="off" onclick="lampOff()">🔴 Matikan</button>
</div>
<div class="card">
  <h3>🔄 Sinkronisasi & Reset</h3>
  <button class="btn-blue" onclick="syncRTC()">⏳ Sinkronkan Waktu dari HP</button><br><br>
  <button class="reset" onclick="resetToSchedule()">🔁 Reset ke Jadwal</button>
</div>
<div class="card">
  <h3>🎨 Tema</h3>
  <label class="switch">
    <input type="checkbox" id="toggleTheme" onchange="toggleTheme(this.checked)">
    <span class="slider" id="labelTheme">Terang</span>
  </label>
</div>
<div id="toast"></div>
<div class="footer">@2025 MjTs-140914™</div>
<script>
let dirtyOn=false, dirtyOff=false;
function showToast(msg){
  var x=document.getElementById("toast");
  x.innerText=msg;
  x.className="show";
  setTimeout(()=>{x.className=x.className.replace("show","");},2500);
}
function saveSchedule(){
  let on=document.getElementById("onTimeInput").value;
  let off=document.getElementById("offTimeInput").value;
  if(!on || !off){ showToast("❌ Isi waktu ON/OFF terlebih dahulu"); return; }
  fetch('/set?on='+encodeURIComponent(on)+'&off='+encodeURIComponent(off))
    .then(()=>{ showToast("✅ Jadwal Disimpan\\nON: "+on+" | OFF: "+off); });
}
function toggleSchedule(state){
  let label=document.getElementById("labelSchedule");
  let saveBtn=document.getElementById("saveBtn");
  if(state){
    label.innerText="Jadwal ON";
    saveBtn.disabled=false;
    fetch('/toggle?state=1').then(r=>r.json()).then(j=>{ showToast(j.toast || "✅ Jadwal Aktif"); });
  } else {
    label.innerText="Jadwal OFF";
    saveBtn.disabled=true;
    fetch('/toggle?state=0').then(r=>r.json()).then(j=>{ showToast(j.toast || "❌ Jadwal Nonaktif"); });
  }
}
function resetToSchedule(){
  fetch('/reset').then(()=>{
    document.getElementById("toggleSchedule").checked=true;
    toggleSchedule(true);
    showToast("🔁 Reset ke Jadwal\\n📅 Mode Jadwal AKTIF");
  });
}
function lampOn(){ fetch('/on').then(()=>{ showToast("🟢 Lampu Hidup"); update(); }); }
function lampOff(){ fetch('/off').then(()=>{ showToast("🔴 Lampu Mati"); update(); }); }
function syncRTC(){
  var now=new Date();
  var url='/sync?y='+now.getFullYear()+'&m='+(now.getMonth()+1)+'&d='+now.getDate()+'&h='+now.getHours()+'&min='+now.getMinutes()+'&s='+now.getSeconds();
  fetch(url).then(()=>{showToast("✅ RTC disinkronkan dari HP");});
}
function update(){
  fetch('/status').then(r=>r.json()).then(j=>{
    document.getElementById('status').innerText=j.status;
    document.getElementById('rtc').innerText=j.rtc;
    document.getElementById('lastOn').innerText=j.lastOn || '--:--:-- --/--/----';
    document.getElementById('lastOff').innerText=j.lastOff || '--:--:-- --/--/----';
    let onInput=document.getElementById('onTimeInput');
    let offInput=document.getElementById('offTimeInput');
    if(!dirtyOn && j.on) onInput.value=j.on;
    if(!dirtyOff && j.off) offInput.value=j.off;
    let toggle=document.getElementById('toggleSchedule');
    toggle.checked=(j.schedule==="true"||j.schedule===true);
    document.getElementById('labelSchedule').innerText=toggle.checked?'Jadwal ON':'Jadwal OFF';
    document.getElementById('saveBtn').disabled=!toggle.checked;
  }).catch(()=>{});
}
function toggleTheme(dark){
  let label=document.getElementById("labelTheme");
  if(dark){ document.body.classList.add("dark"); label.innerText="Gelap"; showToast("🌙 Mode Gelap"); fetch('/theme?dark=1'); }
  else { document.body.classList.remove("dark"); label.innerText="Terang"; showToast("☀️ Mode Terang"); fetch('/theme?dark=0'); }
}
window.onload=function(){
  if(document.body.classList.contains("dark")){
    document.getElementById("toggleTheme").checked=true;
    document.getElementById("labelTheme").innerText="Gelap";
  } else {
    document.getElementById("toggleTheme").checked=false;
    document.getElementById("labelTheme").innerText="Terang";
  }
  setInterval(update,1000);
  update();
};
</script>
</body>
</html>)rawliteral";

  if(darkTheme){
    page.replace("%DARKCLASS%", "class='dark'");
  } else {
    page.replace("%DARKCLASS%", "");
  }
  return page;
}


String configPage(){
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Konfigurasi WiFi</title>
  <style>
    body{font-family:Arial;text-align:center;background:#f4f4f4;}
    .card{background:white;padding:20px;margin:20px;border-radius:12px;box-shadow:0 2px 6px rgba(0,0,0,0.2);}
    .input-container{position:relative;display:block;margin:10px auto;width:250px;}
    .input-container input{width:100%;padding:10px 40px 10px 10px;font-size:16px;border:1px solid #ccc;border-radius:6px;box-sizing:border-box;background:#fff;color:#000;}
    .input-container .eye{position:absolute;right:10px;top:50%;transform:translateY(-50%);cursor:pointer;font-size:18px;color:#666;}
    button{padding:10px 20px;background:#0077cc;color:white;border:none;border-radius:6px;cursor:pointer;font-size:16px;margin-top:15px;}
    #toast{visibility:hidden;min-width:200px;background:#333;color:#fff;text-align:center;border-radius:8px;padding:12px;position:fixed;z-index:1;left:50%;bottom:30px;font-size:16px;transform:translateX(-50%);opacity:0;transition:opacity 0.5s;}
    #toast.show{visibility:visible;opacity:1;}
    body.dark{background:#121212;color:#eaeaea;}
    body.dark .card{background:#1e1e1e;color:#eaeaea;box-shadow:0 2px 6px rgba(255,255,255,0.1);}
    body.dark .input-container input{background:#2a2a2a;color:#eaeaea;border:1px solid #555;}
    body.dark .input-container .eye{color:#aaa;}
    .footer { font-size:12px; color:#777; margin:20px 0; }
    body.dark .footer { color:#aaa; }
  </style>
</head>
<body %DARKCLASS%>

<div class="card">
  <h3>⚙️ Konfigurasi WiFi</h3>
  <div class="input-container">
    <input type="text" id="ssid" placeholder="Masukkan SSID" value="Judul Punya Anda">
  </div>
  <div class="input-container">
    <input type="password" id="password" placeholder="Masukkan Password" value="11111112">
    <span class="eye" onclick="togglePass()">👁️</span>
  </div>
  <button onclick="saveConfig()">💾 Simpan</button>
  <br><br>
  <button onclick="goHome()" style="background:#666;margin-top:10px;">◀️ Kembali</button>
</div>

<div id="toast"></div>
<div class="footer">@2025 MjTs-140914™</div>

<script>
function togglePass(){
  let p=document.getElementById("password");
  p.type=(p.type==="password")?"text":"password";
}
function showToast(msg){
  var x=document.getElementById("toast");
  x.innerText=msg;
  x.className="show";
  setTimeout(()=>{x.className=x.className.replace("show","");},3000);
}
function saveConfig(){
  let ssid=document.getElementById("ssid").value.trim();
  let pass=document.getElementById("password").value;
  if(!ssid){ showToast("❌ SSID tidak boleh kosong"); return; }
  fetch('/config?ssid='+encodeURIComponent(ssid)+'&password='+encodeURIComponent(pass))
    .then(r=>r.json())
    .then(j=>{
      showToast(j.toast || "✅ SSID & Password disimpan");
      setTimeout(()=>{ window.location.href = '/'; }, 1500);
    })
    .catch(()=>{
      showToast("ℹ️ Perangkat sedang restart...");
    });
}
function goHome(){ window.location.href = '/'; }
</script>

</body>
</html>)rawliteral";

  if(darkTheme){
    page.replace("%DARKCLASS%", "class='dark'");
  } else {
    page.replace("%DARKCLASS%", "");
  }
  return page;
}


// ==== Setup ====
void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);

  Wire.begin(21, 22);
  if(!rtc.begin()) Serial.println("RTC tidak terdeteksi!");
  if(rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  loadSettings();
  checkSchedule();

  WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  if(MDNS.begin("alamat_anda")) Serial.println("mDNS responder started: alamat_anda.local");
  Serial.print("IP ESP32 (AP): "); Serial.println(WiFi.softAPIP());

  // root page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *res = request->beginResponse(200, "text/html; charset=UTF-8", htmlPage());
    res->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    res->addHeader("Pragma", "no-cache");
    res->addHeader("Expires", "0");
    request->send(res);
  });

  // config page
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("ssid") && request->hasParam("password")){
      String newSsid = request->getParam("ssid")->value();
      String newPass = request->getParam("password")->value();
      if(newSsid.length() == 0){
        request->send(400,"application/json","{\"toast\":\"❌ SSID tidak boleh kosong\"}");
        return;
      }
      ap_ssid = newSsid;
      ap_password = newPass;
      saveWiFiConfig();
      WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
      MDNS.end(); MDNS.begin("alamat_anda");
      AsyncWebServerResponse *res = request->beginResponse(200,"application/json","{\"toast\":\"✅ SSID & Password disimpan. Restart...\"}");
      res->addHeader("Connection","close");
      request->send(res);
      delay(2000);
      ESP.restart();
    } else {
      request->send(200,"text/html; charset=UTF-8", configPage());
    }
  });

  // status JSON
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    DateTime now = rtc.now();
    String currentTime = formatDateTime(now);
    String status = relayState?"ON":"OFF";
    String json = "{\"status\":\""+status+"\",\"rtc\":\""+currentTime+"\",\"on\":\""+formatTime(onHour,onMinute)+"\",\"off\":\""+formatTime(offHour,offMinute)+"\",\"schedule\":"+(scheduleEnabled?String("true"):String("false"))+",\"lastOn\":\""+lastOn+"\",\"lastOff\":\""+lastOff+"\"}";
    AsyncWebServerResponse *res = request->beginResponse(200,"application/json",json);
    res->addHeader("Cache-Control","no-store, no-cache, must-revalidate, max-age=0");
    res->addHeader("Pragma","no-cache");
    res->addHeader("Expires","0");
    request->send(res);
  });

  // endpoint kontrol
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *r){ setRelay(true); manualOverride=true; manualTime=millis(); r->redirect("/"); });
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *r){ setRelay(false); manualOverride=true; manualTime=millis(); r->redirect("/"); });
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *r){ manualOverride=false; checkSchedule(); r->redirect("/"); });
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *r){
    if(r->hasParam("on") && r->hasParam("off")){
      String onT=r->getParam("on")->value();
      String offT=r->getParam("off")->value();
      int oh=onT.substring(0,2).toInt(); int om=onT.substring(3,5).toInt();
      int fh=offT.substring(0,2).toInt(); int fm=offT.substring(3,5).toInt();
      onHour=oh; onMinute=om; offHour=fh; offMinute=fm; saveSchedule();
      r->send(200,"application/json","{\"toast\":\"✅ Jadwal disimpan\"}"); return;
    }
    r->redirect("/");
  });
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *r){
    if(r->hasParam("state")){
      scheduleEnabled = r->getParam("state")->value().toInt()==1; saveToggle();
      String msg=scheduleEnabled?"✅ Jadwal Aktif":"❌ Jadwal Nonaktif";
      r->send(200,"application/json","{\"toast\":\""+msg+"\"}");
    } else r->send(400,"application/json","{\"toast\":\"Parameter state tidak ada\"}");
  });
  server.on("/sync", HTTP_GET, [](AsyncWebServerRequest *r){
    if(r->hasParam("y")){
      int y=r->getParam("y")->value().toInt(); int m=r->getParam("m")->value().toInt();
      int d=r->getParam("d")->value().toInt(); int h=r->getParam("h")->value().toInt();
      int mi=r->getParam("min")->value().toInt(); int s=r->getParam("s")->value().toInt();
      rtc.adjust(DateTime(y,m,d,h,mi,s));
    }
    r->redirect("/");
  });

  // endpoint tema
  server.on("/theme", HTTP_GET, [](AsyncWebServerRequest *r){
    if(r->hasParam("dark")){
      darkTheme = r->getParam("dark")->value().toInt()==1; saveTheme();
      r->send(200,"application/json","{\"toast\":\"Tema disimpan\"}");
    } else {
      r->send(400,"application/json","{\"toast\":\"Parameter dark tidak ada\"}");
    }
  });

  server.begin();
}

void loop() {
  checkSchedule();
  delay(200);
}
