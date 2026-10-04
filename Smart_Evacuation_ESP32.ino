
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Safety_System";
const char* password = "12345678";

WebServer server(80);

#define ESP32_RX2 16
#define ESP32_TX2 17

String activeSensors = "";
bool vibrationActive = false;
String manualTests = "";
bool manualVibrationTest = false;
String lastMegaMessage = "Not connected";

// ESP32 4.3: تحديث الواجهة من الاختبار اليدوي + استقبال Arduino Mega 3.3
String megaScreenArrow[9];
bool megaStateReceived = false;
unsigned long lastMegaStateMs = 0;
const unsigned long MEGA_TIMEOUT_MS = 3000;

// =====================
// سجل الأحداث (Event Log)
// =====================
#define EVENT_LOG_MAX 30
String eventLog[EVENT_LOG_MAX];
int eventLogCount = 0;
String prevLoggedSensors = "";
bool prevLoggedVibration = false;

String uptimeStamp() {
 unsigned long s = millis() / 1000;
 unsigned long h = (s / 3600) % 24;
 unsigned long m = (s / 60) % 60;
 unsigned long sec = s % 60;
 char buf[10];
 sprintf(buf, "%02lu:%02lu:%02lu", h, m, sec);
 return String(buf);
}

void addLogEntry(String text) {
 String entry = "[" + uptimeStamp() + "] " + text;
 if (eventLogCount < EVENT_LOG_MAX) {
  eventLog[eventLogCount] = entry;
  eventLogCount++;
 } else {
  for (int i = 1; i < EVENT_LOG_MAX; i++) eventLog[i - 1] = eventLog[i];
  eventLog[EVENT_LOG_MAX - 1] = entry;
 }
}

void clearEventLog() {
 eventLogCount = 0;
}

String eventLogJoined() {
 String out = "";
 for (int i = eventLogCount - 1; i >= 0; i--) { // الأحدث أولًا
  out += eventLog[i];
  if (i > 0) out += "~";
 }
 return out;
}

// =====================
// أدوات القوائم
// =====================

bool hasItem(String list, String item) {
 String data = "," + list + ",";
 String token = "," + item + ",";
 return data.indexOf(token) >= 0;
}

void addItem(String &list, String item) {
 if (item.length() == 0) return;
 if (hasItem(list, item)) return;

 if (list.length() == 0) {
  list = item;
 } else {
  list += ",";
  list += item;
 }
}

String unionLists(String a, String b) {
 int start = 0;
 while (start < b.length()) {
  int comma = b.indexOf(',', start);
  if (comma == -1) comma = b.length();

  String item = b.substring(start, comma);
  item.trim();
  if (item.length() > 0) addItem(a, item);

  start = comma + 1;
 }
 return a;
}

void removeItem(String &list, String item) {
 if (list.length() == 0) return;

 String data = "," + list + ",";
 String token = "," + item + ",";
 data.replace(token, ",");

 if (data == ",") {
  list = "";
  return;
 }

 if (data.startsWith(",")) data.remove(0, 1);
 if (data.endsWith(",")) data.remove(data.length() - 1, 1);

 list = data;
}

void toggleItem(String &list, String item) {
 if (hasItem(list, item)) removeItem(list, item);
 else addItem(list, item);
}

void resetAll() {
 activeSensors = "";
 vibrationActive = false;
 manualTests = "";
 manualVibrationTest = false;
 megaStateReceived = false;
 for (int i = 1; i <= 8; i++) megaScreenArrow[i] = "";
 lastMegaMessage = "RESET";
}

// =====================
// بيانات الحساسات
// =====================

String sensorZone(String s) {
 if (s == "IR22") return "adiya2";
 if (s == "IR23") return "tamreed";
 if (s == "IR24") return "amaliyat";
 if (s == "IR25") return "makhzan";
 if (s == "IR26") return "idara";
 if (s == "IR27") return "entizar";
 if (s == "IR28") return "ashea";
 if (s == "IR29") return "tawari";
 if (s == "IR30") return "istiqbal";
 if (s == "IR31") return "mukhtabar";
 if (s == "IR32") return "saydaliya";

 if (s == "MQ233") return "adiya1";
 if (s == "MQ234") return "tamreed";
 if (s == "MQ235") return "makhzan";
 if (s == "MQ236") return "matbakh";
 if (s == "MQ237") return "dawra";
 if (s == "MQ238") return "saydaliya";
 if (s == "MQ239") return "corrTopLeft";
 if (s == "MQ240") return "corrTopRight";

 if (s == "MQ541") return "matbakh";
 if (s == "MQ542") return "mukhtabar";

 if (s == "MQ2BL") return "corrBottomLeft";
 if (s == "MQ2BR") return "corrBottomRight";
 if (s == "MQ252") return "corrBottomLeft";
 if (s == "MQ253") return "corrBottomRight";

 return "";
}

String sensorLabel(String s) {
 if (s == "IR22") return "IR-22";
 if (s == "IR23") return "IR-23";
 if (s == "IR24") return "IR-24";
 if (s == "IR25") return "IR-25";
 if (s == "IR26") return "IR-26";
 if (s == "IR27") return "IR-27";
 if (s == "IR28") return "IR-28";
 if (s == "IR29") return "IR-29";
 if (s == "IR30") return "IR-30";
 if (s == "IR31") return "IR-31";
 if (s == "IR32") return "IR-32";

 if (s == "MQ233") return "MQ2-33";
 if (s == "MQ234") return "MQ2-34";
 if (s == "MQ235") return "MQ2-35";
 if (s == "MQ236") return "MQ2-36";
 if (s == "MQ237") return "MQ2-37";
 if (s == "MQ238") return "MQ2-38";
 if (s == "MQ239") return "MQ2-39";
 if (s == "MQ240") return "MQ2-40";

 if (s == "MQ541") return "MQ5-41";
 if (s == "MQ542") return "MQ5-42";

 if (s == "MQ2BL") return "MQ2 سفلي أيسر";
 if (s == "MQ2BR") return "MQ2 سفلي أيمن";
 if (s == "MQ252") return "MQ2-52";
 if (s == "MQ253") return "MQ2-53";

 return s;
}

String sensorType(String s) {
 if (s.startsWith("IR")) return "حريق";
 if (s.startsWith("MQ2")) return "غاز MQ2";
 if (s.startsWith("MQ5")) return "غاز MQ5";
 return "غير معروف";
}

String zoneName(String z) {
 if (z == "adiya1") return "العيادة 1";
 if (z == "adiya2") return "العيادة 2";
 if (z == "tamreed") return "غرفة التمريض";
 if (z == "amaliyat") return "العمليات الصغرى";
 if (z == "makhzan") return "المخزن";
 if (z == "idara") return "الإدارة";
 if (z == "entizar") return "صالة الانتظار";
 if (z == "ashea") return "الأشعة";
 if (z == "tawari") return "قسم الطوارئ";
 if (z == "istiqbal") return "الاستقبال";
 if (z == "mukhtabar") return "المختبر";
 if (z == "saydaliya") return "الصيدلية";
 if (z == "matbakh") return "المطبخ";
 if (z == "dawra") return "دورة المياه";
 if (z == "corrTopLeft") return "الممر العلوي الأيسر";
 if (z == "corrTopRight") return "الممر العلوي الأيمن";
 if (z == "corrLeft") return "الممر الأيسر";
 if (z == "corrRight") return "الممر الأيمن";
 if (z == "corrBottomLeft") return "الممر السفلي الأيسر";
 if (z == "corrBottomRight") return "الممر السفلي الأيمن";
 return z;
}

String activeZonesList() {
 String zones = "";
 int start = 0;

 while (start < activeSensors.length()) {
  int comma = activeSensors.indexOf(',', start);
  if (comma == -1) comma = activeSensors.length();

  String s = activeSensors.substring(start, comma);
  String z = sensorZone(s);

  if (z.length() > 0 && !hasItem(zones, z)) {
   addItem(zones, z);
  }

  start = comma + 1;
 }

 return zones;
}

String visualZonesList() {
 String zones = activeZonesList();

 // وميض بصري فقط للممرات القريبة بدون تغيير منطق إشارات الشاشات
 if (hasItem(zones, "matbakh")) addItem(zones, "corrLeft");
 if (hasItem(zones, "ashea")) addItem(zones, "corrRight");

 return zones;
}

bool anyFire() {
 return activeSensors.indexOf("IR") >= 0;
}

bool anyGas() {
 return activeSensors.indexOf("MQ2") >= 0 || activeSensors.indexOf("MQ5") >= 0;
}

String systemStatusText() {
 if (vibrationActive && activeSensors.length() > 0) return "زلزال + خطر";
 if (vibrationActive) return "زلزال";
 if (anyFire() && anyGas()) return "حريق + غاز";
 if (anyFire()) return "حريق";
 if (anyGas()) return "غاز";
 return "طبيعي";
}

String systemStatusClass() {
 if (vibrationActive && activeSensors.length() > 0) return "mixed";
 if (vibrationActive) return "vibration";
 if (activeSensors.length() > 0) return "dangerText";
 return "normal";
}

String zonesTextHTML() {
 String zones = activeZonesList();

 if (zones.length() == 0) return "لا توجد أخطار";

 String text = "";
 int start = 0;

 while (start < zones.length()) {
  int comma = zones.indexOf(',', start);
  if (comma == -1) comma = zones.length();

  String z = zones.substring(start, comma);
  text += "• " + zoneName(z) + "<br>";

  start = comma + 1;
 }

 return text;
}

String sensorsTextHTML() {
 if (activeSensors.length() == 0) return "لا توجد حساسات نشطة";

 String text = "";
 int start = 0;

 while (start < activeSensors.length()) {
  int comma = activeSensors.indexOf(',', start);
  if (comma == -1) comma = activeSensors.length();

  String s = activeSensors.substring(start, comma);
  String z = sensorZone(s);

  text += "• " + sensorLabel(s) + " - " + sensorType(s) + " - " + zoneName(z) + "<br>";

  start = comma + 1;
 }

 return text;
}

// =====================
// المخارج
// =====================

String exitRedList() {
 String red = "";
 String zones = activeZonesList();

 if (hasItem(zones, "corrTopLeft") || hasItem(zones, "corrLeft")) addItem(red, "e3");
 if (hasItem(zones, "corrTopRight") || hasItem(zones, "corrRight")) addItem(red, "e2");

 if (hasItem(zones, "corrBottomLeft")) {
  addItem(red, "e4");
 }

 if (hasItem(zones, "corrBottomRight")) {
  addItem(red, "e1");
 }

 return red;
}

String exitOrangeList() {
 String orange = "";
 String zones = activeZonesList();
 String red = exitRedList();

 if (!hasItem(red, "e1")) {
  if (hasItem(zones, "saydaliya") || hasItem(zones, "mukhtabar") || hasItem(zones, "istiqbal")) {
   addItem(orange, "e1");
  }
 }

 if (!hasItem(red, "e2")) {
  if (hasItem(zones, "idara") || hasItem(zones, "makhzan") || hasItem(zones, "amaliyat") || hasItem(zones, "ashea") || hasItem(zones, "saydaliya")) {
   addItem(orange, "e2");
  }
 }

 if (!hasItem(red, "e3")) {
  if (hasItem(zones, "adiya1") || hasItem(zones, "adiya2") || hasItem(zones, "tamreed") || hasItem(zones, "matbakh")) {
   addItem(orange, "e3");
  }
 }

 if (!hasItem(red, "e4")) {
  if (hasItem(zones, "matbakh") || hasItem(zones, "dawra") || hasItem(zones, "tawari") || hasItem(zones, "istiqbal")) {
   addItem(orange, "e4");
  }
 }

 if (!hasItem(red, "mainEntrance")) {
  if (hasItem(zones, "istiqbal") || hasItem(zones, "entizar")) {
   addItem(orange, "mainEntrance");
  }
 }

 return orange;
}

String exitsTextHTML() {
 String red = exitRedList();
 String orange = exitOrangeList();

 String text = "";

 String exits[5] = {"e1", "e2", "e3", "e4", "mainEntrance"};
 String names[5] = {"E1", "E2", "E3", "E4", "المدخل الرئيسي"};

 for (int i = 0; i < 5; i++) {
  String e = exits[i];
  String label = names[i];

  if (hasItem(red, e)) {
   text += "• " + label + " خطر مباشر<br>";
  } else if (hasItem(orange, e)) {
   text += "• " + label + " غير مفضل<br>";
  } else {
   text += "• " + label + " آمن<br>";
  }
 }

 return text;
}

// =====================
// اتجاهات الشاشات
// =====================

String defaultScreenArrow(int n) {
 if (n == 1) return "↓";
 if (n == 2) return "→";
 if (n == 3) return "→";
 if (n == 4) return "↓";
 if (n == 5) return "↓";
 if (n == 6) return "←";
 if (n == 7) return "←";
 if (n == 8) return "↔";
 return "";
}

String screenArrow(int n) {
 if (megaOnline() && n >= 1 && n <= 8 && megaScreenArrow[n].length() > 0) {
  return megaScreenArrow[n];
 }

 String zones = activeZonesList();

 // نفس منطق إشارات V2، مع اعتبار الممر العلوي الأيسر مثل جهة اليسار
 // والممر العلوي الأيمن مثل جهة اليمين، دون أخذ وميض الممرات الإضافي في الحساب.
 bool corrLeftDanger = hasItem(zones, "corrLeft") || hasItem(zones, "corrTopLeft");
 bool corrRightDanger = hasItem(zones, "corrRight") || hasItem(zones, "corrTopRight");

 bool rightDanger = corrRightDanger || hasItem(zones, "idara") || hasItem(zones, "makhzan") || hasItem(zones, "amaliyat") || hasItem(zones, "ashea") || hasItem(zones, "mukhtabar") || hasItem(zones, "saydaliya") || hasItem(zones, "corrBottomRight");
 bool leftDanger = corrLeftDanger || hasItem(zones, "adiya1") || hasItem(zones, "adiya2") || hasItem(zones, "tamreed") || hasItem(zones, "matbakh") || hasItem(zones, "dawra") || hasItem(zones, "tawari") || hasItem(zones, "corrBottomLeft");
 bool centerDanger = hasItem(zones, "entizar") || hasItem(zones, "istiqbal");

 if (vibrationActive && activeSensors.length() == 0) {
  return defaultScreenArrow(n);
 }

 if (n == 1) {
  if (hasItem(zones, "istiqbal") || hasItem(zones, "corrBottomLeft") || hasItem(zones, "corrBottomRight")) {
   if (rightDanger && !leftDanger) return "←";
   if (leftDanger && !rightDanger) return "→";
   return "↔";
  }
  return "↓";
 }

 if (n == 2) {
  if (hasItem(zones, "saydaliya") || hasItem(zones, "mukhtabar") || hasItem(zones, "corrBottomRight")) return "←";
  return "→";
 }

 if (n == 3) {
  if (corrRightDanger || hasItem(zones, "ashea") || hasItem(zones, "amaliyat")) return "←";
  return "→";
 }

 if (n == 4) {
  if (corrRightDanger) return "←";
  return "↓";
 }

 if (n == 5) {
  if (corrLeftDanger) return "→";
  return "↓";
 }

 if (n == 6) {
  if (corrLeftDanger || hasItem(zones, "matbakh")) return "→";
  return "←";
 }

 if (n == 7) {
  if (hasItem(zones, "tawari") || hasItem(zones, "dawra") || hasItem(zones, "corrBottomLeft")) return "→";
  return "←";
 }

 if (n == 8) {
  if (centerDanger && rightDanger && !leftDanger) return "←";
  if (centerDanger && leftDanger && !rightDanger) return "→";
  if (rightDanger && !leftDanger) return "←";
  if (leftDanger && !rightDanger) return "→";
  if (rightDanger && leftDanger && !centerDanger && !vibrationActive) return "↔";
  return "↔";
 }

 return defaultScreenArrow(n);
}

bool screenChanged(int n) {
 if (vibrationActive) return true;
 return screenArrow(n) != defaultScreenArrow(n);
}

String screenJson() {
 String json = "{";
 for (int i = 1; i <= 8; i++) {
  if (i > 1) json += ",";
  json += "\"s";
  json += String(i);
  json += "\":\"";
  json += screenArrow(i);
  json += "\",\"b";
  json += String(i);
  json += "\":\"";
  json += screenChanged(i) ? "1" : "0";
  json += "\"";
 }
 json += "}";
 return json;
}

// =====================
// استقبال الحالة من Arduino Mega 3.3
// =====================

String arrowFromCode(char c) {
 if (c == 'R') return "→";
 if (c == 'L') return "←";
 if (c == 'U') return "↑";
 if (c == 'D') return "↓";
 if (c == 'B') return "↔";
 if (c == 'X') return "X";
 return "";
}

void parseScreensFromMega(String scr) {
 for (int i = 1; i <= 8; i++) megaScreenArrow[i] = defaultScreenArrow(i);

 int start = 0;
 while (start < scr.length()) {
  int comma = scr.indexOf(',', start);
  if (comma == -1) comma = scr.length();

  String item = scr.substring(start, comma);
  item.trim();

  int colon = item.indexOf(':');
  if (colon > 0 && item.length() > colon + 1) {
   int n = item.substring(0, colon).toInt();
   char c = item.charAt(colon + 1);
   if (n >= 1 && n <= 8) {
    String a = arrowFromCode(c);
    if (a.length() > 0) megaScreenArrow[n] = a;
   }
  }

  start = comma + 1;
 }
}

String getFieldValue(String msg, String key, String nextKey) {
 int p = msg.indexOf(key);
 if (p < 0) return "";
 p += key.length();

 int end = -1;
 if (nextKey.length() > 0) end = msg.indexOf(nextKey, p);
 if (end < 0) end = msg.length();

 return msg.substring(p, end);
}

void logSensorChanges(String oldList, String newList) {
 int start = 0;
 while (start < newList.length()) {
  int comma = newList.indexOf(',', start);
  if (comma == -1) comma = newList.length();
  String item = newList.substring(start, comma);
  item.trim();
  if (item.length() > 0 && !hasItem(oldList, item)) {
   addLogEntry(String("بدأ: ") + sensorLabel(item));
  }
  start = comma + 1;
 }
 start = 0;
 while (start < oldList.length()) {
  int comma = oldList.indexOf(',', start);
  if (comma == -1) comma = oldList.length();
  String item = oldList.substring(start, comma);
  item.trim();
  if (item.length() > 0 && !hasItem(newList, item)) {
   addLogEntry(String("انتهى: ") + sensorLabel(item));
  }
  start = comma + 1;
 }
}

void parseStateMessage(String msg) {
 // Format from Arduino Mega 3.3:
 // STATE:VIB=0;S=IR27,MQ236;SCR=1:D,2:R,3:R,4:D,5:D,6:L,7:L,8:B
 String vib = getFieldValue(msg, "VIB=", ";S=");
 String sensors = getFieldValue(msg, ";S=", ";SCR=");
 String scr = getFieldValue(msg, ";SCR=", "");

 bool newVibration = (vib == "1");

 logSensorChanges(prevLoggedSensors, sensors);
 if (newVibration && !prevLoggedVibration) addLogEntry("بدأ: اهتزاز (زلزال)");
 if (!newVibration && prevLoggedVibration) addLogEntry("انتهى: اهتزاز (زلزال)");
 prevLoggedSensors = sensors;
 prevLoggedVibration = newVibration;

 vibrationActive = newVibration;
 activeSensors = sensors;
 parseScreensFromMega(scr);

 megaStateReceived = true;
 lastMegaStateMs = millis();
 lastMegaMessage = msg;
}

bool megaOnline() {
 return megaStateReceived && (millis() - lastMegaStateMs < MEGA_TIMEOUT_MS);
}

// =====================
// تحويل رسائل Arduino Mega لاحقًا
// =====================

String sensorFromMegaMessage(String msg) {
 if (msg == "FIRE:ADIYA2") return "IR22";
 if (msg == "FIRE:TAMREED") return "IR23";
 if (msg == "FIRE:AMALIYAT") return "IR24";
 if (msg == "FIRE:MAKHZAN") return "IR25";
 if (msg == "FIRE:IDARA") return "IR26";
 if (msg == "FIRE:ENTIZAR") return "IR27";
 if (msg == "FIRE:ASHEA") return "IR28";
 if (msg == "FIRE:TAWARI") return "IR29";
 if (msg == "FIRE:ISTIQBAL") return "IR30";
 if (msg == "FIRE:MUKHTABAR") return "IR31";
 if (msg == "FIRE:SAYDALIYA") return "IR32";

 if (msg == "GAS:ADIYA1") return "MQ233";
 if (msg == "GAS:TAMREED") return "MQ234";
 if (msg == "GAS:MAKHZAN") return "MQ235";
 if (msg == "GAS:MATBAKH") return "MQ236";
 if (msg == "GAS:DAWRA") return "MQ237";
 if (msg == "GAS:SAYDALIYA") return "MQ238";
 if (msg == "GAS:CORRIDOR_LEFT") return "MQ239";
 if (msg == "GAS:CORRIDOR_RIGHT") return "MQ240";

 if (msg == "GAS5:MATBAKH") return "MQ541";
 if (msg == "GAS5:MUKHTABAR") return "MQ542";

 if (msg == "GAS:LOWER_LEFT") return "MQ252";
 if (msg == "GAS:LOWER_RIGHT") return "MQ253";

 return "";
}

void parseMegaMessage(String msg) {
 msg.trim();
 if (msg.length() == 0) return;

 if (msg.startsWith("STATE:")) {
  parseStateMessage(msg);
  return;
 }

 lastMegaMessage = msg;

 // دعم الرسائل القديمة إذا احتجناها أثناء التجريب
 if (msg == "NORMAL") {
  addLogEntry("تصفير النظام (زر الميجا)");
  resetAll();
  return;
 }

 if (msg == "VIBRATION") {
  vibrationActive = true;
  return;
 }

 if (msg == "NO_VIBRATION") {
  vibrationActive = false;
  return;
 }

 String s = sensorFromMegaMessage(msg);
 if (s.length() > 0) {
  addItem(activeSensors, s);
 }
}

// =====================
// صفحة الموقع
// =====================

String htmlPage() {
 String page = R"HTML(
<!DOCTYPE html>
<html lang='ar' dir='rtl'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>Smart Evacuation System - ESP32 4.3</title>

<style>
*{box-sizing:border-box}
body{
 margin:0;
 font-family:Arial,Tahoma,sans-serif;
 background:#e9eef3;
 color:#222;
 overflow:hidden;
}

.topbar{
 height:46px;
 background:#073b63;
 color:white;
 display:flex;
 align-items:center;
 justify-content:space-between;
 padding:0 14px;
 font-size:18px;
 font-weight:bold;
}

.settingsBtn{
 background:white;
 color:#073b63;
 border:none;
 border-radius:8px;
 font-size:20px;
 padding:5px 12px;
 cursor:pointer;
}

.settingsBtn.connected{
 background:#2ecc71;
 color:white;
}

.eventLogBox{
 max-height:260px;
 overflow-y:auto;
 background:#f4f6f8;
 border-radius:8px;
 padding:8px;
 margin-top:8px;
 font-size:12px;
 direction:rtl;
 text-align:right;
}

.eventLogBox .logLine{
 padding:3px 0;
 border-bottom:1px solid #e0e0e0;
}

.layout{
 height:calc(100vh - 46px);
 display:grid;
 grid-template-columns:300px 1fr;
 gap:6px;
 padding:6px;
 direction:rtl;
}

.layout.settingsOpen{
 grid-template-columns:300px 1fr 360px;
}

.side{
 background:white;
 border-radius:12px;
 padding:10px;
 overflow:auto;
 box-shadow:0 2px 8px rgba(0,0,0,.18);
}

.settingsPanel{
 display:none;
 background:white;
 border-radius:12px;
 padding:10px;
 overflow:auto;
 box-shadow:0 2px 8px rgba(0,0,0,.18);
 direction:rtl;
}

.layout.settingsOpen .settingsPanel{
 display:block;
}

.main{
 background:white;
 border-radius:12px;
 padding:6px;
 display:flex;
 align-items:center;
 justify-content:center;
 box-shadow:0 2px 8px rgba(0,0,0,.18);
 overflow:hidden;
}

.card{
 background:#f8f9fb;
 border:1px solid #d6dce2;
 border-radius:10px;
 padding:10px;
 margin-bottom:8px;
 font-size:15px;
 line-height:1.65;
}

.card h3{
 margin:0 0 6px 0;
 color:#073b63;
 font-size:16px;
}

.statusText{
 font-size:28px;
 font-weight:bold;
}

.normal{color:green}
.dangerText{color:red;animation:blink .8s infinite}
.vibration{color:#e67e22;animation:blink .45s infinite}
.mixed{color:#b00020;animation:blink .5s infinite}

@keyframes blink{
 0%{opacity:1}
 50%{opacity:.25}
 100%{opacity:1}
}

.map{
 width:100%;
 height:100%;
 max-height:calc(100vh - 70px);
 aspect-ratio:1.42/1;
 background:#fafafa;
 border:4px solid #222;
 border-radius:10px;
 position:relative;
 direction:ltr;
}

.map.vibrationFrame{
 border-color:#f39c12;
 animation:orangeFrame .45s infinite;
}

@keyframes orangeFrame{
 0%{box-shadow:0 0 0 0 rgba(243,156,18,.8)}
 50%{box-shadow:0 0 18px 6px rgba(243,156,18,.8)}
 100%{box-shadow:0 0 0 0 rgba(243,156,18,.8)}
}

.room{
 position:absolute;
 border:2px solid #333;
 background:white;
 display:flex;
 align-items:center;
 justify-content:center;
 text-align:center;
 font-size:13px;
 font-weight:bold;
 direction:rtl;
 padding:18px 4px 4px 4px;
 overflow:hidden;
}

.corridor{
 position:absolute;
 background:#cfd5db;
 border:2px dashed #777;
 opacity:.9;
}

.corridorLabel{
 position:absolute;
 font-size:12px;
 font-weight:bold;
 color:#333;
 background:rgba(255,255,255,.75);
 padding:2px 5px;
 border-radius:5px;
 z-index:4;
 direction:rtl;
}

.room.dangerZone,
.corridor.dangerZone{
 background:#ff7777!important;
 animation:blink .75s infinite;
}

.exit{
 position:absolute;
 background:#27ae60;
 color:white;
 border:2px solid #145a32;
 font-weight:bold;
 display:flex;
 align-items:center;
 justify-content:center;
 border-radius:4px;
 font-size:15px;
 z-index:12;
}

.exit.exitOrange{
 background:#f39c12!important;
 color:white;
}

.exit.exitRed{
 background:#e74c3c!important;
 color:white;
 animation:blink .75s infinite;
}

.screen{
 position:absolute;
 background:#f1c40f;
 color:#5a4500;
 border:2px solid #b7950b;
 border-radius:5px;
 font-weight:bold;
 display:flex;
 align-items:center;
 justify-content:center;
 gap:3px;
 font-size:17px;
 z-index:20;
 direction:ltr;
}

.scrNum{
 display:inline-block;
 transform:none!important;
}

.scrArrow{
 display:inline-block;
 font-size:18px;
 line-height:1;
}

#scr4 .scrArrow{
 transform:rotate(-90deg);
}

#scr5 .scrArrow{
 transform:rotate(90deg);
}

.screen.changed{
 animation:blink .55s infinite;
 outline:3px solid #111;
}

.sensor{
 position:absolute;
 min-width:35px;
 height:20px;
 border-radius:10px;
 color:white;
 font-size:10px;
 font-weight:bold;
 display:flex;
 align-items:center;
 justify-content:center;
 z-index:18;
 direction:ltr;
 padding:0 4px;
}

.ir{background:#e60000}
.mq2{background:#00a000}
.mq5{background:#1e73e8}

.sensor.active{
 outline:4px solid #111;
 animation:blink .5s infinite;
}

.dot{
 display:inline-block;
 width:10px;
 height:10px;
 border-radius:50%;
 margin-left:4px;
}

.btn{
 border:none;
 border-radius:8px;
 padding:10px 12px;
 margin:5px 3px;
 color:white;
 background:#073b63;
 cursor:pointer;
 font-size:15px;
 width:100%;
 text-align:right;
}

.btnFire{background:#c0392b}
.btnGas{background:#d35400}
.btnVib{background:#e67e22}
.btnOk{background:#27ae60}
.btn:hover{opacity:.85}

.small{
 color:#555;
 font-size:13px;
}

#r_ad1{left:7%;top:3%;width:14%;height:20%}
#r_ad2{left:21.5%;top:3%;width:13%;height:20%}
#r_tam{left:35%;top:3%;width:19%;height:20%}
#r_amal{left:55%;top:3%;width:10%;height:20%}
#r_makh{left:66%;top:3%;width:10%;height:20%}
#r_idara{left:77%;top:3%;width:15%;height:20%}

#c_top_left{left:4%;top:25%;width:46%;height:9%}
#c_top_right{left:50%;top:25%;width:46%;height:9%}
#c_left{left:27%;top:34%;width:8%;height:34%}
#c_right{left:67%;top:34%;width:8%;height:34%}
#c_bottom_left{left:8%;top:68%;width:30%;height:8%}
#c_bottom_right{left:62%;top:68%;width:30%;height:8%}
#c_bottom_center{left:38%;top:68%;width:24%;height:8%}

#lbl_top_left{left:16%;top:27.2%}
#lbl_top_right{left:66%;top:27%}
#lbl_left{left:27%;top:47%}
#lbl_right{left:66%;top:47%}
#lbl_bl{left:21%;top:72%}
#lbl_br{left:63%;top:73%}

#r_mat{left:4%;top:36%;width:22%;height:25%}
#r_ent{left:36%;top:36%;width:28%;height:20%}
#r_ash{left:76%;top:36%;width:20%;height:25%}
#r_dawra{left:4%;top:77%;width:10%;height:17%}
#r_taw{left:20%;top:77%;width:18%;height:17%}
#r_ist{left:40%;top:78%;width:20%;height:16%}
#r_mukh{left:66%;top:77%;width:18%;height:17%}
#r_sayd{left:87%;top:77%;width:9%;height:17%}

#e3{left:-1%;top:3%;width:6%;height:7%}
#e2{right:-1%;top:3%;width:6%;height:7%}
#e4{left:8%;bottom:-1%;width:7%;height:7%}
#e1{right:8%;bottom:-1%;width:7%;height:7%}
#mainEntrance{left:42%;bottom:-1%;width:17%;height:6%;background:#555;color:white;font-size:12px}

#scr1{left:50%;top:72%;width:6%;height:5%}
#scr2{left:64%;top:64%;width:7%;height:5%}
#scr3{left:66%;top:34%;width:7%;height:5%}
#scr4{left:76%;top:25%;width:5%;height:7%}
#scr5{left:24%;top:25%;width:5%;height:7%}
#scr6{left:28%;top:34%;width:7%;height:5%}
#scr7{left:28%;top:67%;width:7%;height:5%}
#scr8{left:47%;top:58%;width:8%;height:5%}

#IR22{left:29%;top:5%}
#IR23{left:40%;top:5%}
#IR24{left:57%;top:5%}
#IR25{left:68%;top:5%}
#IR26{left:80%;top:5%}
#IR27{left:48%;top:40%}
#IR28{left:80%;top:40%}
#IR29{left:25%;top:79%}
#IR30{left:46%;top:82%}
#IR31{left:72%;top:79%}
#IR32{left:89%;top:80%}

#MQ233{left:10%;top:5%}
#MQ234{left:48%;top:5%}
#MQ235{left:71%;top:5%}
#MQ236{left:6%;top:43%}
#MQ237{left:6%;top:82%}
#MQ238{left:89%;top:87%}
#MQ239{left:11%;top:28%}
#MQ240{left:88%;top:28%}

#MQ541{left:6%;top:52%}
#MQ542{left:70%;top:86%}

#MQ252{left:11%;top:69%}
#MQ253{left:87%;top:69%}

.btn.manualActive{outline:3px solid #ff9800;background:#ffe0b2!important;color:#111!important;font-weight:900;}
</style>
</head>

<body>

<div class="topbar">
 <div>Smart Evacuation System</div>
 <button id="settingsBtn" class="settingsBtn" onclick="toggleSettings()">⚙</button>
</div>

<div id="layout" class="layout">
 <aside class="side">
  <div class="card">
   <h3>حالة النظام</h3>
   <div id="statusText" class="statusText normal">طبيعي</div>
   <div class="small">آخر رسالة: <span id="megaText">Not connected</span></div>
  </div>

  <div class="card">
   <h3>الأخطار الحالية</h3>
   <div id="zonesText">لا توجد أخطار</div>
  </div>

  <div class="card">
   <h3>الحساسات النشطة</h3>
   <div id="sensorsText">لا توجد حساسات نشطة</div>
  </div>

  <div class="card">
   <h3>حالة المخارج</h3>
   <div id="exitsText">E1 آمن<br>E2 آمن<br>E3 آمن<br>E4 آمن</div>
  </div>

  <div class="card">
   <h3>الدليل</h3>
   <span class="dot" style="background:#e60000"></span> IR حريق<br>
   <span class="dot" style="background:#00a000"></span> MQ2 غاز<br>
   <span class="dot" style="background:#1e73e8"></span> MQ5 غاز<br>
   <span class="dot" style="background:#f39c12"></span> مخرج غير مفضل<br>
   <span class="dot" style="background:#e74c3c"></span> خطر مباشر
  </div>
 </aside>

 <main class="main">
  <div id="map" class="map">
   <div id="r_ad1" class="room" data-zone="adiya1">عيادة 1</div>
   <div id="r_ad2" class="room" data-zone="adiya2">عيادة 2</div>
   <div id="r_tam" class="room" data-zone="tamreed">غرفة تمريض</div>
   <div id="r_amal" class="room" data-zone="amaliyat">عمليات صغرى</div>
   <div id="r_makh" class="room" data-zone="makhzan">مخزن</div>
   <div id="r_idara" class="room" data-zone="idara">الإدارة</div>

   <div id="c_top_left" class="corridor" data-zone="corrTopLeft"></div>
   <div id="c_top_right" class="corridor" data-zone="corrTopRight"></div>
   <div id="c_left" class="corridor" data-zone="corrLeft"></div>
   <div id="c_right" class="corridor" data-zone="corrRight"></div>
   <div id="c_bottom_left" class="corridor" data-zone="corrBottomLeft"></div>
   <div id="c_bottom_right" class="corridor" data-zone="corrBottomRight"></div>
   <div id="c_bottom_center" class="corridor"></div>

   <div id="lbl_top_left" class="corridorLabel">العلوي الأيسر</div>
   <div id="lbl_top_right" class="corridorLabel">العلوي الأيمن</div>
   <div id="lbl_left" class="corridorLabel">الممر الأيسر</div>
   <div id="lbl_right" class="corridorLabel">الممر الأيمن</div>
   <div id="lbl_bl" class="corridorLabel">ممر سفلي أيسر</div>
   <div id="lbl_br" class="corridorLabel">ممر سفلي أيمن</div>

   <div id="r_mat" class="room" data-zone="matbakh">مطبخ</div>
   <div id="r_ent" class="room" data-zone="entizar">صالة الانتظار</div>
   <div id="r_ash" class="room" data-zone="ashea">الأشعة</div>
   <div id="r_dawra" class="room" data-zone="dawra">دورة مياه</div>
   <div id="r_taw" class="room" data-zone="tawari">قسم الطوارئ</div>
   <div id="r_ist" class="room" data-zone="istiqbal">الاستقبال</div>
   <div id="r_mukh" class="room" data-zone="mukhtabar">مختبر</div>
   <div id="r_sayd" class="room" data-zone="saydaliya">صيدلية</div>

   <div id="e3" class="exit">E3</div>
   <div id="e2" class="exit">E2</div>
   <div id="e4" class="exit">E4</div>
   <div id="e1" class="exit">E1</div>
   <div id="mainEntrance" class="exit">المدخل الرئيسي</div>

   <div id="scr1" class="screen"><span class="scrNum">1</span><span class="scrArrow">↓</span></div>
   <div id="scr2" class="screen"><span class="scrNum">2</span><span class="scrArrow">→</span></div>
   <div id="scr3" class="screen"><span class="scrNum">3</span><span class="scrArrow">→</span></div>
   <div id="scr4" class="screen"><span class="scrNum">4</span><span class="scrArrow">↓</span></div>
   <div id="scr5" class="screen"><span class="scrNum">5</span><span class="scrArrow">↓</span></div>
   <div id="scr6" class="screen"><span class="scrNum">6</span><span class="scrArrow">←</span></div>
   <div id="scr7" class="screen"><span class="scrNum">7</span><span class="scrArrow">←</span></div>
   <div id="scr8" class="screen"><span class="scrNum">8</span><span class="scrArrow">↔</span></div>

   <div id="IR22" class="sensor ir">IR22</div>
   <div id="IR23" class="sensor ir">IR23</div>
   <div id="IR24" class="sensor ir">IR24</div>
   <div id="IR25" class="sensor ir">IR25</div>
   <div id="IR26" class="sensor ir">IR26</div>
   <div id="IR27" class="sensor ir">IR27</div>
   <div id="IR28" class="sensor ir">IR28</div>
   <div id="IR29" class="sensor ir">IR29</div>
   <div id="IR30" class="sensor ir">IR30</div>
   <div id="IR31" class="sensor ir">IR31</div>
   <div id="IR32" class="sensor ir">IR32</div>

   <div id="MQ233" class="sensor mq2">33</div>
   <div id="MQ234" class="sensor mq2">34</div>
   <div id="MQ235" class="sensor mq2">35</div>
   <div id="MQ236" class="sensor mq2">36</div>
   <div id="MQ237" class="sensor mq2">37</div>
   <div id="MQ238" class="sensor mq2">38</div>
   <div id="MQ239" class="sensor mq2">39</div>
   <div id="MQ240" class="sensor mq2">40</div>

   <div id="MQ541" class="sensor mq5">41</div>
   <div id="MQ542" class="sensor mq5">42</div>

   <div id="MQ252" class="sensor mq2">52</div>
   <div id="MQ253" class="sensor mq2">53</div>
  </div>
 </main>

 <section class="settingsPanel">
  <div class="card">
   <h3>الإعدادات والفحص اليدوي</h3>
   <button class="btn btnOk" onclick="resetAll()">مسح الكل / طبيعي</button>
   <button id="btnVibration" class="btn btnVib" onclick="toggleVibration()">تفعيل / إلغاء الزلزال</button>
  </div>

  <div class="card">
   <h3>سجل الأحداث</h3>
   <button class="btn btnOk" onclick="clearLog()">مسح السجل</button>
   <div id="eventLogBox" class="eventLogBox"></div>
  </div>

  <div class="card">
   <h3>حساسات الحريق IR</h3>
   <button class="btn btnFire" data-sensor="IR22" onclick="toggleSensor('IR22')">IR-22 عيادة 2</button>
   <button class="btn btnFire" data-sensor="IR23" onclick="toggleSensor('IR23')">IR-23 تمريض</button>
   <button class="btn btnFire" data-sensor="IR24" onclick="toggleSensor('IR24')">IR-24 عمليات</button>
   <button class="btn btnFire" data-sensor="IR25" onclick="toggleSensor('IR25')">IR-25 مخزن</button>
   <button class="btn btnFire" data-sensor="IR26" onclick="toggleSensor('IR26')">IR-26 إدارة</button>
   <button class="btn btnFire" data-sensor="IR27" onclick="toggleSensor('IR27')">IR-27 انتظار</button>
   <button class="btn btnFire" data-sensor="IR28" onclick="toggleSensor('IR28')">IR-28 أشعة</button>
   <button class="btn btnFire" data-sensor="IR29" onclick="toggleSensor('IR29')">IR-29 طوارئ</button>
   <button class="btn btnFire" data-sensor="IR30" onclick="toggleSensor('IR30')">IR-30 استقبال</button>
   <button class="btn btnFire" data-sensor="IR31" onclick="toggleSensor('IR31')">IR-31 مختبر</button>
   <button class="btn btnFire" data-sensor="IR32" onclick="toggleSensor('IR32')">IR-32 صيدلية</button>
  </div>

  <div class="card">
   <h3>حساسات الغاز MQ2</h3>
   <button class="btn btnGas" data-sensor="MQ233" onclick="toggleSensor('MQ233')">MQ2-33 عيادة 1</button>
   <button class="btn btnGas" data-sensor="MQ234" onclick="toggleSensor('MQ234')">MQ2-34 تمريض</button>
   <button class="btn btnGas" data-sensor="MQ235" onclick="toggleSensor('MQ235')">MQ2-35 مخزن</button>
   <button class="btn btnGas" data-sensor="MQ236" onclick="toggleSensor('MQ236')">MQ2-36 مطبخ</button>
   <button class="btn btnGas" data-sensor="MQ237" onclick="toggleSensor('MQ237')">MQ2-37 دورة مياه</button>
   <button class="btn btnGas" data-sensor="MQ238" onclick="toggleSensor('MQ238')">MQ2-38 صيدلية</button>
   <button class="btn btnGas" data-sensor="MQ239" onclick="toggleSensor('MQ239')">MQ2-39 علوي أيسر</button>
   <button class="btn btnGas" data-sensor="MQ240" onclick="toggleSensor('MQ240')">MQ2-40 علوي أيمن</button>
   <button class="btn btnGas" data-sensor="MQ252" onclick="toggleSensor('MQ252')">MQ2 ممر سفلي أيسر</button>
   <button class="btn btnGas" data-sensor="MQ253" onclick="toggleSensor('MQ253')">MQ2 ممر سفلي أيمن</button>
  </div>

  <div class="card">
   <h3>حساسات MQ5</h3>
   <button class="btn btnGas" data-sensor="MQ541" onclick="toggleSensor('MQ541')">MQ5-41 مطبخ</button>
   <button class="btn btnGas" data-sensor="MQ542" onclick="toggleSensor('MQ542')">MQ5-42 مختبر</button>
  </div>

  <div class="card">
   <h3>أوامر Arduino Mega</h3>
   <button class="btn" onclick="sendCmd('TEST')">اختبار الشاشات</button>
   <button class="btn" onclick="sendCmd('BUZZER')">اختبار الجرس</button>
   <button class="btn" onclick="sendCmd('RESET')">إعادة ضبط Arduino</button>
   <p class="small">الأوامر ترسل إلى Arduino Mega 3.3. لمسح الاختبارات اضغط مسح الكل / طبيعي.</p>
  </div>
 </section>
</div>

<script>
function toggleSettings(){
 document.getElementById('layout').classList.toggle('settingsOpen');
}

function clearVisuals(){
 document.querySelectorAll('.room,.corridor').forEach(function(e){
  e.classList.remove('dangerZone');
 });

 document.querySelectorAll('.sensor').forEach(function(e){
  e.classList.remove('active');
 });

 document.querySelectorAll('.exit').forEach(function(e){
  e.classList.remove('exitRed');
  e.classList.remove('exitOrange');
 });

 document.querySelectorAll('.screen').forEach(function(e){
  e.classList.remove('changed');
 });
}

function uiArrowForMap(n, arrow){
 // ESP32 4.3: عكس بصري فقط للشاشة 4 و5 على الموقع.
 // لا يغير أوامر Arduino Mega ولا يغير الشاشات الحقيقية.
 if((n == 4 || n == 5) && arrow == "→") return "←";
 if((n == 4 || n == 5) && arrow == "←") return "→";
 return arrow;
}

function setScreen(n, arrow, changed){
 var el = document.getElementById('scr'+n);
 if(!el) return;
 el.querySelector('.scrNum').innerHTML = n;
 el.querySelector('.scrArrow').innerHTML = uiArrowForMap(n, arrow);
 if(changed == "1") el.classList.add('changed');
}

function updateStatus(){
 fetch('/status')
 .then(function(r){return r.json();})
 .then(function(data){
  document.getElementById('statusText').innerHTML = data.statusText;
  document.getElementById('statusText').className = 'statusText ' + data.statusClass;
  document.getElementById('megaText').innerHTML = data.lastMega;
  document.getElementById('zonesText').innerHTML = data.zonesText;
  document.getElementById('sensorsText').innerHTML = data.sensorsText;
  document.getElementById('exitsText').innerHTML = data.exitsText;

  var sbtn = document.getElementById('settingsBtn');
  if (sbtn) {
   if (data.megaConnected == "1") sbtn.classList.add('connected');
   else sbtn.classList.remove('connected');
  }

  var logBox = document.getElementById('eventLogBox');
  if (logBox) {
   if (data.eventLog.length > 0) {
    var lines = data.eventLog.split('~');
    var html = '';
    for (var li = 0; li < lines.length; li++) {
     html += '<div class="logLine">' + lines[li] + '</div>';
    }
    logBox.innerHTML = html;
   } else {
    logBox.innerHTML = '<div class="logLine">لا يوجد أحداث بعد</div>';
   }
  }

  var map = document.getElementById('map');
  if(data.vibration == "1") map.classList.add('vibrationFrame');
  else map.classList.remove('vibrationFrame');

  clearVisuals();

  if(data.visualZones.length > 0){
   var zones = data.visualZones.split(',');
   for(var i=0;i<zones.length;i++){
    var z = zones[i];
    document.querySelectorAll('[data-zone="'+z+'"]').forEach(function(e){
     e.classList.add('dangerZone');
    });
   }
  }

  if(data.sensors.length > 0){
   var sensors = data.sensors.split(',');
   for(var j=0;j<sensors.length;j++){
    var s = document.getElementById(sensors[j]);
    if(s) s.classList.add('active');
   }
  }

  if(data.exitRed.length > 0){
   var reds = data.exitRed.split(',');
   for(var r=0;r<reds.length;r++){
    var er = document.getElementById(reds[r]);
    if(er) er.classList.add('exitRed');
   }
  }

  if(data.exitOrange.length > 0){
   var oranges = data.exitOrange.split(',');
   for(var o=0;o<oranges.length;o++){
    var eo = document.getElementById(oranges[o]);
    if(eo) eo.classList.add('exitOrange');
   }
  }

  var scr = data.screens;
  for(var n=1;n<=8;n++){
   setScreen(n, scr['s'+n], scr['b'+n]);
  }

  updateManualButtons(data.manualTests, data.manualVibration);
 });
}

function updateManualButtons(list, vib){
 document.querySelectorAll('[data-sensor]').forEach(function(btn){
  var s = btn.getAttribute('data-sensor');
  if((',' + list + ',').indexOf(',' + s + ',') >= 0) btn.classList.add('manualActive');
  else btn.classList.remove('manualActive');
 });
 var vb = document.getElementById('btnVibration');
 if(vb){
  if(vib == "1") vb.classList.add('manualActive');
  else vb.classList.remove('manualActive');
 }
}

function toggleSensor(s){
 fetch('/toggle?s=' + encodeURIComponent(s)).then(function(){updateStatus();});
}

function toggleVibration(){
 fetch('/vibration').then(function(){updateStatus();});
}

function resetAll(){
 fetch('/reset').then(function(){updateStatus();});
}

function clearLog(){
 fetch('/clearlog').then(function(){updateStatus();});
}

function sendCmd(c){
 fetch('/cmd?c=' + encodeURIComponent(c)).then(function(){updateStatus();});
}

setInterval(updateStatus,1000);
updateStatus();
</script>

</body>
</html>
)HTML";

 return page;
}

// =====================
// JSON STATUS
// =====================

void handleStatus() {
 // ESP32 4.3:
 // الواجهة تعتمد على الحالة النهائية:
 // حساسات Arduino Mega الحقيقية + الاختبارات اليدوية المفعلة من الموقع.
 // هذا يجعل القائمة اليمنى تتغير فورًا عند الاختبار، ولا تبقى "طبيعي" أثناء وجود إنذار.
 String savedActiveSensors = activeSensors;
 bool savedVibrationActive = vibrationActive;

 activeSensors = unionLists(activeSensors, manualTests);
 vibrationActive = vibrationActive || manualVibrationTest;

 String json = "{";

 json += "\"statusText\":\"" + systemStatusText() + "\",";
 json += "\"statusClass\":\"" + systemStatusClass() + "\",";
 json += "\"vibration\":\"" + String(vibrationActive ? "1" : "0") + "\",";
 json += "\"zones\":\"" + activeZonesList() + "\",";
 json += "\"visualZones\":\"" + visualZonesList() + "\",";
 json += "\"sensors\":\"" + activeSensors + "\",";
 json += "\"zonesText\":\"" + zonesTextHTML() + "\",";
 json += "\"sensorsText\":\"" + sensorsTextHTML() + "\",";
 json += "\"exitsText\":\"" + exitsTextHTML() + "\",";
 json += "\"exitRed\":\"" + exitRedList() + "\",";
 json += "\"exitOrange\":\"" + exitOrangeList() + "\",";
 json += "\"lastMega\":\"" + lastMegaMessage + "\",";
 json += "\"megaConnected\":\"" + String(megaOnline() ? "1" : "0") + "\",";
 json += "\"manualTests\":\"" + manualTests + "\",";
 json += "\"manualVibration\":\"" + String(manualVibrationTest ? "1" : "0") + "\",";
 json += "\"eventLog\":\"" + eventLogJoined() + "\",";
 json += "\"screens\":" + screenJson();

 json += "}";

 activeSensors = savedActiveSensors;
 vibrationActive = savedVibrationActive;

 server.send(200, "application/json; charset=utf-8", json);
}


// =====================
// HANDLERS
// =====================

void handleRoot() {
 server.send(200, "text/html; charset=utf-8", htmlPage());
}

void handleToggle() {
 if (server.hasArg("s")) {
  String s = server.arg("s");

  if (hasItem(manualTests, s)) {
   removeItem(manualTests, s);
   Serial2.println("UNTEST:" + s);
   lastMegaMessage = "SEND UNTEST:" + s;
   addLogEntry("إيقاف اختبار يدوي: " + sensorLabel(s));
  } else {
   addItem(manualTests, s);
   Serial2.println("TEST:" + s);
   lastMegaMessage = "SEND TEST:" + s;
   addLogEntry("اختبار يدوي: " + sensorLabel(s));
  }
 }

 server.send(200, "text/plain; charset=utf-8", "OK");
}

void handleVibration() {
 if (manualVibrationTest) {
  manualVibrationTest = false;
  Serial2.println("UNTEST:VIB");
  lastMegaMessage = "SEND UNTEST:VIB";
  addLogEntry("إيقاف اختبار يدوي: اهتزاز");
 } else {
  manualVibrationTest = true;
  Serial2.println("TEST:VIB");
  lastMegaMessage = "SEND TEST:VIB";
  addLogEntry("اختبار يدوي: اهتزاز");
 }
 server.send(200, "text/plain; charset=utf-8", "OK");
}

void handleReset() {
 Serial2.println("CLEAR");
 resetAll();
 lastMegaMessage = "SEND CLEAR";
 addLogEntry("تصفير النظام (زر الموقع)");
 server.send(200, "text/plain; charset=utf-8", "OK");
}

void handleClearLog() {
 clearEventLog();
 server.send(200, "text/plain; charset=utf-8", "OK");
}

void handleCmd() {
 if (server.hasArg("c")) {
  String c = server.arg("c");

  if (c == "TEST") Serial2.println("CMD:TEST");
  else if (c == "BUZZER") Serial2.println("CMD:BUZZER");
  else if (c == "RESET") {
   Serial2.println("CMD:RESET");
   resetAll();
  } else {
   Serial2.println("CMD:" + c);
  }

  lastMegaMessage = "SEND CMD:" + c;
 }

 server.send(200, "text/plain; charset=utf-8", "OK");
}

// =====================
// SETUP
// =====================

void setup() {
 Serial.begin(115200);
 Serial2.begin(9600, SERIAL_8N1, ESP32_RX2, ESP32_TX2);

 WiFi.mode(WIFI_AP);
 WiFi.softAP(ssid, password);

 server.on("/", handleRoot);
 server.on("/status", handleStatus);
 server.on("/toggle", handleToggle);
 server.on("/vibration", handleVibration);
 server.on("/reset", handleReset);
 server.on("/cmd", handleCmd);
 server.on("/clearlog", handleClearLog);

 server.begin();

 Serial.println("ESP32 Web Server Started");
 Serial.print("WiFi: ");
 Serial.println(ssid);
 Serial.print("Password: ");
 Serial.println(password);
 Serial.print("IP: ");
 Serial.println(WiFi.softAPIP());
}

// =====================
// LOOP
// =====================

void loop() {
 server.handleClient();

 while (Serial2.available()) {
  String msg = Serial2.readStringUntil('\n');
  parseMegaMessage(msg);
 }
}



