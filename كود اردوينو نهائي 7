Arduino Mega 3.5.2
Arduino Mega 3.5.2 LCD Compile Fix
نسخة مصححة لحل خطأ: LcdInfo does not name a type. استخدم ملف INO للرفع على Arduino Mega.
/* ============================================================================
   نظام إرشاد مخارج السلامة - Arduino Mega 3.5.2 LCD Compile Fix
   Arduino Mega 2560 + MAX7219 LED Matrix + Fire/Gas/Vibration Sensors
   Arduino Mega 3.5.2 LCD Compile Fix: إضافة شاشة LCD 1604 I2C لعرض حالة النظام ونوع الحساس ومكان الخطر والمخارج الآمنة
 
   Fire/Gas sensors:
   - Normal = HIGH
   - Danger = LOW
 
   Vibration sensors:
   - Normal = LOW
   - Danger pulse = HIGH
 
   ESP32 Serial link:
   - Mega TX1 D18 -> ESP32 RX2 GPIO16 عبر level shifter أو voltage divider
   - Mega RX1 D19 <- ESP32 TX2 GPIO17
   - GND مشترك
 
   أوامر من ESP32 إلى Mega عبر Serial1:
   - TEST:IR27        تفعيل اختبار حساس معين
   - TEST:MQ236       تفعيل اختبار حساس معين
   - TEST:MQ541       تفعيل اختبار حساس معين
   - TEST:VIB         تفعيل اختبار الزلزال
   - UNTEST:IR27      إلغاء اختبار حساس معين
   - UNTEST:VIB       إلغاء اختبار الزلزال
   - CLEAR            مسح كل الاختبارات اليدوية
   - CMD:TEST         اختبار الشاشات
   - CMD:BUZZER       اختبار الجرس
   - CMD:RESET        مسح الاختبار والرجوع للحالة الطبيعية
 
   رسالة الحالة من Mega إلى ESP32:
   STATE:VIB=0;S=IR27,MQ236;SCR=1:D,2:R,3:R,4:D,5:D,6:L,7:L,8:B
============================================================================ */
 
#include <LedControl.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
 
// =====================
// إعدادات عامة
// =====================
 
#define HAZARD_ACTIVE_STATE LOW
#define VIBRATION_ACTIVE_STATE HIGH
 
#define FIRE_GAS_HOLD_MS 5000UL
#define VIBRATION_HOLD_MS 10000UL
 
#define DEBOUNCE_COUNT 4
#define SAMPLE_INTERVAL_MS 30UL
#define STATE_SEND_INTERVAL_MS 1000UL
 
#define BUZZER_PIN 43
 
#define RESET_BUTTON_PIN 2      // زر بوش بوتين لتصفير النظام
#define RED_LED_RELAY_PIN 3     // رولي يشغّل لمبة/فلاشر أحمر (إنذار بصري)
 
 
// =====================
// LCD 1604 I2C
// =====================
 
#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 4
 
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
unsigned long lastLcdUpdateMs = 0;
#define LCD_UPDATE_INTERVAL_MS 500UL
 
// =====================
// MAX7219 GROUPS
// =====================
 
// SCR1 - SCR4
#define DIN_G1 44
#define CLK_G1 45
#define CS_G1  46
 
// SCR5 - SCR8
#define DIN_G2 47
#define CLK_G2 48
#define CS_G2  49
 
LedControl lc1 = LedControl(DIN_G1, CLK_G1, CS_G1, 4);
LedControl lc2 = LedControl(DIN_G2, CLK_G2, CS_G2, 4);
 
// =====================
// Fire sensors
// =====================
 
#define F_EYADAH2    22
#define F_TAMREED   23
#define F_AMALIYAT  24
#define F_MAKHZAN   25
#define F_IDARA     26
#define F_ENTIZAR   27
#define F_ASHEA     28
#define F_TAWARI    29
#define F_ISTIQBAL  30
#define F_MUKHTABAR 31
#define F_SAYDALIYA 32
 
// =====================
// MQ2 sensors
// =====================
 
#define G_EYADAH1         33
#define G_TAMREED        34
#define G_MAKHZAN        35
#define G_MATBAKH        36
#define G_DAWRA          37
#define G_SAYDALIYA      38
#define G_CORRIDOR_LEFT  39
#define G_CORRIDOR_RIGHT 40
#define G_LOWER_LEFT      52
#define G_LOWER_RIGHT     53
 
// =====================
// MQ5 sensors
// =====================
 
#define M_MATBAKH    41
#define M_MUKHTABAR  42
 
// =====================
// Vibration sensors
// =====================
 
#define VIB_RIGHT 50
#define VIB_LEFT  51
 
// =====================
// اتجاهات الشاشات
// =====================
 
enum Direction {
  DIR_NONE,
  DIR_RIGHT,
  DIR_LEFT,
  DIR_UP,
  DIR_DOWN,
  DIR_BOTH,
  DIR_WARNING
};
 
Direction screenDir[9];
Direction normalDir[9] = {
  DIR_NONE,
  DIR_DOWN,   // SCR1
  DIR_RIGHT,  // SCR2
  DIR_RIGHT,  // SCR3
  DIR_DOWN,   // SCR4
  DIR_DOWN,   // SCR5
  DIR_LEFT,   // SCR6
  DIR_LEFT,   // SCR7
  DIR_BOTH    // SCR8
};
 
bool screenBlink[9];
 
// تعديل اتجاه الشاشات بعد التركيب
byte screenRotation[9] = {
  0,
  0, // SCR1
  0, // SCR2
  0, // SCR3
  0, // SCR4
  0, // SCR5
  0, // SCR6
  0, // SCR7
  0  // SCR8
};
 
bool screenMirrorH[9] = {
  false,
  true,   // SCR1 معكوس أفقيًا حسب آخر تعديل
  false,
  false,
  false,
  false,
  false,
  false,
  false
};
 
bool screenMirrorV[9] = {
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
  false
};
 
// =====================
// رسومات الأسهم 8x8
// =====================
 
byte IMG_RIGHT[8] = {
  B00010000,
  B00011000,
  B11111100,
  B11111110,
  B11111110,
  B11111100,
  B00011000,
  B00010000
};
 
byte IMG_LEFT[8] = {
  B00001000,
  B00011000,
  B00111111,
  B01111111,
  B01111111,
  B00111111,
  B00011000,
  B00001000
};
 
byte IMG_DOWN[8] = {
  B00011000,
  B00011000,
  B00011000,
  B00011000,
  B11111111,
  B01111110,
  B00111100,
  B00011000
};
 
byte IMG_UP[8] = {
  B00011000,
  B00111100,
  B01111110,
  B11111111,
  B00011000,
  B00011000,
  B00011000,
  B00011000
};
 
byte IMG_BOTH[8] = {
  B00000000,
  B00100100,
  B01100110,
  B11111111,
  B11111111,
  B01100110,
  B00100100,
  B00000000
};
 
byte IMG_X[8] = {
  B10000001,
  B01000010,
  B00100100,
  B00011000,
  B00011000,
  B00100100,
  B01000010,
  B10000001
};
 
byte IMG_EMPTY[8] = {
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000
};
 
// =====================
// Debounce structure
// =====================
 
struct InputMonitor {
  byte pin;
  byte activeState;
  unsigned long holdMs;
  bool stableActive;
  bool lastRaw;
  byte count;
  unsigned long lastActiveMs;
};
 
InputMonitor fireMon[11];
InputMonitor mq2Mon[10];
InputMonitor mq5Mon[2];
InputMonitor vibMon[2];
 
unsigned long lastSampleMs = 0;
unsigned long lastStateSendMs = 0;
 
// =====================
// Manual test flags from ESP32
// =====================
 
bool testFire[11];
bool testMQ2[10];
bool testMQ5[2];
bool testVibration = false;
 
// =====================
// أدوات الإدخال
// =====================
 
void initMonitor(InputMonitor &m, byte pin, byte activeState, unsigned long holdMs) {
  m.pin = pin;
  m.activeState = activeState;
  m.holdMs = holdMs;
  m.stableActive = false;
  m.lastRaw = false;
  m.count = 0;
  m.lastActiveMs = 0;
}
 
void updateMonitor(InputMonitor &m) {
  bool rawActive = (digitalRead(m.pin) == m.activeState);
 
  if (rawActive == m.lastRaw) {
    if (m.count < DEBOUNCE_COUNT) m.count++;
  } else {
    m.count = 0;
    m.lastRaw = rawActive;
  }
 
  if (m.count >= DEBOUNCE_COUNT) {
    m.stableActive = rawActive;
  }
 
  if (m.stableActive) {
    m.lastActiveMs = millis();
  }
}
 
// حساسات الاهتزاز تعطي نبضة قصيرة أحيانًا، لذلك لا نستعمل معها نفس تأكيد الغاز/الحريق.
// أي نبضة HIGH واحدة يتم حفظها لمدة VIBRATION_HOLD_MS.
void updateVibrationMonitor(InputMonitor &m) {
  bool rawActive = (digitalRead(m.pin) == m.activeState);
  m.stableActive = rawActive;
  if (rawActive) {
    m.lastActiveMs = millis();
  }
}
 
bool isActive(InputMonitor &m) {
  if (m.stableActive) return true;
  if (m.lastActiveMs > 0 && millis() - m.lastActiveMs < m.holdMs) return true;
  return false;
}
 
// =====================
// أدوات الرسم
// =====================
 
byte reverseBits(byte b) {
  byte r = 0;
  for (byte i = 0; i < 8; i++) {
    r <<= 1;
    r |= (b & 1);
    b >>= 1;
  }
  return r;
}
 
void copyImage(byte src[8], byte dst[8]) {
  for (byte i = 0; i < 8; i++) dst[i] = src[i];
}
 
void mirrorHorizontal(byte img[8]) {
  for (byte i = 0; i < 8; i++) img[i] = reverseBits(img[i]);
}
 
void mirrorVertical(byte img[8]) {
  for (byte i = 0; i < 4; i++) {
    byte tmp = img[i];
    img[i] = img[7 - i];
    img[7 - i] = tmp;
  }
}
 
void rotateCW(byte img[8]) {
  byte out[8];
 
  for (byte r = 0; r < 8; r++) out[r] = 0;
 
  for (byte r = 0; r < 8; r++) {
    for (byte c = 0; c < 8; c++) {
      bool bit = img[r] & (1 << (7 - c));
      if (bit) {
        out[c] |= (1 << r);
      }
    }
  }
 
  for (byte i = 0; i < 8; i++) img[i] = out[i];
}
 
void applyTransform(byte img[8], byte screenNumber) {
  if (screenMirrorH[screenNumber]) mirrorHorizontal(img);
  if (screenMirrorV[screenNumber]) mirrorVertical(img);
 
  byte rot = screenRotation[screenNumber] % 4;
  for (byte i = 0; i < rot; i++) rotateCW(img);
}
 
void getImage(Direction dir, byte img[8]) {
  if (dir == DIR_RIGHT) copyImage(IMG_RIGHT, img);
  else if (dir == DIR_LEFT) copyImage(IMG_LEFT, img);
  else if (dir == DIR_UP) copyImage(IMG_UP, img);
  else if (dir == DIR_DOWN) copyImage(IMG_DOWN, img);
  else if (dir == DIR_BOTH) copyImage(IMG_BOTH, img);
  else if (dir == DIR_WARNING) copyImage(IMG_X, img);
  else copyImage(IMG_EMPTY, img);
}
 
void writeScreen(byte screenNumber, Direction dir, bool showIt) {
  byte img[8];
 
  if (showIt) getImage(dir, img);
  else copyImage(IMG_EMPTY, img);
 
  applyTransform(img, screenNumber);
 
  LedControl *lc;
  byte device;
 
  if (screenNumber >= 1 && screenNumber <= 4) {
    lc = &lc1;
    device = screenNumber - 1;
  } else {
    lc = &lc2;
    device = screenNumber - 5;
  }
 
  for (byte row = 0; row < 8; row++) {
    lc->setRow(device, row, img[row]);
  }
}
 
void clearAllScreens() {
  for (byte s = 1; s <= 8; s++) {
    writeScreen(s, DIR_NONE, true);
  }
}
 
void displayScreens() {
  bool blinkPhase = (millis() / 400) % 2;
 
  for (byte s = 1; s <= 8; s++) {
    bool showIt = true;
 
    if (screenBlink[s]) {
      showIt = blinkPhase;
    }
 
    if (screenDir[s] == DIR_WARNING) {
      showIt = blinkPhase;
    }
 
    writeScreen(s, screenDir[s], showIt);
  }
}
 
// =====================
// قراءة الأخطار
// =====================
 
bool anyFire = false;
bool anyGas = false;
bool anyVibration = false;
bool resetButtonStableState = HIGH;
bool resetButtonRawLastState = HIGH;
unsigned long resetButtonLastChangeMs = 0;
const unsigned long RESET_BUTTON_DEBOUNCE_MS = 50;
 
bool dEyadah2, dTamreedFire, dAmaliyat, dMakhzanFire, dIdara;
bool dEntizar, dAshea, dTawari, dIstiqbal, dMukhtabarFire, dSaydaliyaFire;
 
bool dEyadah1Gas, dTamreedGas, dMakhzanGas, dMatbakhMQ2, dDawraGas;
bool dSaydaliyaGas, dCorrLeft, dCorrRight;
bool dLowerLeftGas, dLowerRightGas;
 
bool dMatbakhMQ5, dMukhtabarMQ5;
 
bool dMatbakh, dMakhzan, dTamreed, dMukhtabar, dSaydaliya;
 
void updateInputs() {
  if (millis() - lastSampleMs < SAMPLE_INTERVAL_MS) return;
  lastSampleMs = millis();
 
  for (byte i = 0; i < 11; i++) updateMonitor(fireMon[i]);
  for (byte i = 0; i < 10; i++) updateMonitor(mq2Mon[i]);
  for (byte i = 0; i < 2; i++) updateMonitor(mq5Mon[i]);
  for (byte i = 0; i < 2; i++) updateVibrationMonitor(vibMon[i]);
 
  dEyadah2        = isActive(fireMon[0])  || testFire[0];
  dTamreedFire   = isActive(fireMon[1])  || testFire[1];
  dAmaliyat      = isActive(fireMon[2])  || testFire[2];
  dMakhzanFire   = isActive(fireMon[3])  || testFire[3];
  dIdara         = isActive(fireMon[4])  || testFire[4];
  dEntizar       = isActive(fireMon[5])  || testFire[5];
  dAshea         = isActive(fireMon[6])  || testFire[6];
  dTawari        = isActive(fireMon[7])  || testFire[7];
  dIstiqbal      = isActive(fireMon[8])  || testFire[8];
  dMukhtabarFire = isActive(fireMon[9])  || testFire[9];
  dSaydaliyaFire = isActive(fireMon[10]) || testFire[10];
 
  dEyadah1Gas     = isActive(mq2Mon[0]) || testMQ2[0];
  dTamreedGas    = isActive(mq2Mon[1]) || testMQ2[1];
  dMakhzanGas    = isActive(mq2Mon[2]) || testMQ2[2];
  dMatbakhMQ2    = isActive(mq2Mon[3]) || testMQ2[3];
  dDawraGas      = isActive(mq2Mon[4]) || testMQ2[4];
  dSaydaliyaGas  = isActive(mq2Mon[5]) || testMQ2[5];
  dCorrLeft      = isActive(mq2Mon[6]) || testMQ2[6];
  dCorrRight     = isActive(mq2Mon[7]) || testMQ2[7];
  dLowerLeftGas  = isActive(mq2Mon[8]) || testMQ2[8];
  dLowerRightGas = isActive(mq2Mon[9]) || testMQ2[9];
 
  dMatbakhMQ5    = isActive(mq5Mon[0]) || testMQ5[0];
  dMukhtabarMQ5  = isActive(mq5Mon[1]) || testMQ5[1];
 
  dMatbakh   = dMatbakhMQ2 || dMatbakhMQ5;
  dMakhzan   = dMakhzanFire || dMakhzanGas;
  dTamreed   = dTamreedFire || dTamreedGas;
  dMukhtabar = dMukhtabarFire || dMukhtabarMQ5;
  dSaydaliya = dSaydaliyaFire || dSaydaliyaGas;
 
  anyFire = dEyadah2 || dTamreedFire || dAmaliyat || dMakhzanFire || dIdara ||
            dEntizar || dAshea || dTawari || dIstiqbal || dMukhtabarFire || dSaydaliyaFire;
 
  anyGas = dEyadah1Gas || dTamreedGas || dMakhzanGas || dMatbakhMQ2 || dDawraGas ||
           dSaydaliyaGas || dCorrLeft || dCorrRight || dLowerLeftGas || dLowerRightGas || dMatbakhMQ5 || dMukhtabarMQ5;
 
  anyVibration = isActive(vibMon[0]) || isActive(vibMon[1]) || testVibration;
}
 
// =====================
// Manual test from ESP32
// =====================
 
void clearManualTests() {
  for (byte i = 0; i < 11; i++) testFire[i] = false;
  for (byte i = 0; i < 10; i++) testMQ2[i] = false;
  for (byte i = 0; i < 2; i++) testMQ5[i] = false;
  testVibration = false;
}
 
void setManualTest(String code, bool active) {
  code.trim();
 
  if (code == "IR22") testFire[0] = active;
  else if (code == "IR23") testFire[1] = active;
  else if (code == "IR24") testFire[2] = active;
  else if (code == "IR25") testFire[3] = active;
  else if (code == "IR26") testFire[4] = active;
  else if (code == "IR27") testFire[5] = active;
  else if (code == "IR28") testFire[6] = active;
  else if (code == "IR29") testFire[7] = active;
  else if (code == "IR30") testFire[8] = active;
  else if (code == "IR31") testFire[9] = active;
  else if (code == "IR32") testFire[10] = active;
 
  else if (code == "MQ233") testMQ2[0] = active;
  else if (code == "MQ234") testMQ2[1] = active;
  else if (code == "MQ235") testMQ2[2] = active;
  else if (code == "MQ236") testMQ2[3] = active;
  else if (code == "MQ237") testMQ2[4] = active;
  else if (code == "MQ238") testMQ2[5] = active;
  else if (code == "MQ239") testMQ2[6] = active;
  else if (code == "MQ240") testMQ2[7] = active;
  else if (code == "MQ252") { testMQ2[8] = active; Serial.println("   matched MQ252 -> testMQ2[8]"); }
  else if (code == "MQ253") { testMQ2[9] = active; Serial.println("   matched MQ253 -> testMQ2[9]"); }
 
  else if (code == "MQ541") testMQ5[0] = active;
  else if (code == "MQ542") testMQ5[1] = active;
 
  else if (code == "VIB") testVibration = active;
}
 
void applyManualTest(String code) {
  setManualTest(code, true);
}
 
void removeManualTest(String code) {
  setManualTest(code, false);
}
 
void testBuzzerOnce() {
  tone(BUZZER_PIN, 1800);
  delay(300);
  noTone(BUZZER_PIN);
}
 
void handleSerialCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  Serial.print("RX from ESP32: [");
  Serial.print(cmd);
  Serial.println("]");
 
  if (cmd.startsWith("TEST:")) {
    String code = cmd.substring(5);
    applyManualTest(code);
    Serial.print("-> applyManualTest(");
    Serial.print(code);
    Serial.println(")");
  } else if (cmd.startsWith("UNTEST:")) {
    String code = cmd.substring(7);
    removeManualTest(code);
    Serial.print("-> removeManualTest(");
    Serial.print(code);
    Serial.println(")");
  } else if (cmd == "CLEAR") {
    clearManualTests();
  } else if (cmd == "CMD:TEST") {
    selfTest();
  } else if (cmd == "CMD:BUZZER") {
    testBuzzerOnce();
  } else if (cmd == "CMD:RESET") {
    clearManualTests();
    resetDirectionsToNormal();
    clearAllScreens();
  }
}
 
void readESP32Commands() {
  while (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    handleSerialCommand(cmd);
  }
}
 
// =====================
// منطق الشاشات
// =====================
 
void resetDirectionsToNormal() {
  for (byte i = 1; i <= 8; i++) {
    screenDir[i] = normalDir[i];
    screenBlink[i] = false;
  }
}
 
void blinkAllScreens() {
  for (byte i = 1; i <= 8; i++) {
    screenBlink[i] = true;
  }
}
 
void updateDisplayLogic() {
  resetDirectionsToNormal();
 
  bool anyDanger = anyFire || anyGas || anyVibration;
 
  if (!anyDanger) {
    return;
  }
 
  // تقسيم الخطر حسب الجهات في المخطط
  // dCorrRight = الحساس 40: طريق E2 / الممر العلوي الأيمن
  // dCorrLeft  = الحساس 39: طريق E3 / الممر العلوي الأيسر
  // ملاحظة: خطر المطبخ D36/D41 لا يدخل في leftSideDanger لأنه لا يغلق مسار E3 ولا يغيّر SCR6/SCR8 إلى الوسط
  bool rightSideDanger =
    dCorrRight || dIdara || dMakhzan || dAmaliyat || dAshea || dMukhtabar || dSaydaliya || dLowerRightGas;
 
  bool leftSideDanger =
    dCorrLeft || dEyadah1Gas || dEyadah2 || dTamreed || dDawraGas || dTawari || dLowerLeftGas;
 
  bool centerDanger =
    dEntizar || dIstiqbal;
 
  if (anyVibration) {
    blinkAllScreens();
  }
 
  // SCR1 - الاستقبال / المدخل الرئيسي
  // الطبيعي: DOWN إلى المدخل الرئيسي.
  // إذا الاستقبال خطر، يختار يمين أو يسار حسب الجهة الأقل خطراً.
  if (dIstiqbal) {
    if (rightSideDanger && !leftSideDanger) screenDir[1] = DIR_LEFT;
    else if (leftSideDanger && !rightSideDanger) screenDir[1] = DIR_RIGHT;
    else screenDir[1] = DIR_BOTH;
 
    screenBlink[1] = true;
  }
 
  // SCR2 - أسفل اليمين
  // الطبيعي: RIGHT إلى E1.
  // إذا المختبر أو الصيدلية خطر، لا يوجه إلى E1 بل يرجع إلى الوسط/المدخل.
  // MQ53 - خطر الممر السفلي الأيمن يوجه نفس توجيه المختبر/الصيدلية
  if (dMukhtabar || dSaydaliya || dLowerRightGas) {
    screenDir[2] = DIR_LEFT;
    screenBlink[2] = true;
  }
 
  // SCR3 - يمين الوسط
  // الطبيعي: RIGHT إلى E2.
  // خطر الأشعة وحده لا يمنع E2، لذلك تبقى الشاشة على RIGHT.
  // عند خطر الحساس 40 فقط، لا توجه إلى E2 وتتحول إلى LEFT نحو الوسط.
  if (dCorrRight) {
    screenDir[3] = DIR_LEFT;
    screenBlink[3] = true;
  }
 
  // SCR4 - الممر العلوي الأيمن
  // DOWN = إلى E2.
  // RIGHT = إلى SCR2 ثم E1 أو المدخل الرئيسي.
  // إذا الحساس 40 خطر، طريق E2 غير آمن، فتتحول إلى RIGHT.
  if (dCorrRight) {
    screenDir[4] = DIR_RIGHT;
    screenBlink[4] = true;
  }
 
  // SCR5 - الممر العلوي الأيسر
  // DOWN = إلى E3.
  // LEFT = إلى SCR7 ثم E4 أو المدخل الرئيسي.
  // إذا الحساس 39 خطر، طريق E3 غير آمن، فتتحول إلى LEFT.
  if (dCorrLeft) {
    screenDir[5] = DIR_LEFT;
    screenBlink[5] = true;
  }
 
  // SCR6 - يسار الوسط
  // الطبيعي: LEFT إلى E3.
  // خطر التمريض لا يغير SCR6.
  // خطر المطبخ لا يغير SCR6: تبقى LEFT نحو E3.
  // فقط عند خطر الحساس 39 / الممر العلوي الأيسر، تتحول إلى RIGHT نحو الوسط.
  if (dCorrLeft) {
    screenDir[6] = DIR_RIGHT;
    screenBlink[6] = true;
  }
 
  // SCR7 - أسفل اليسار
  // الطبيعي: LEFT إلى E4.
  // إذا الطوارئ أو دورة المياه خطر، يرجع إلى RIGHT نحو الاستقبال/المدخل.
  // MQ52 - خطر الممر السفلي الأيسر يوجه نفس توجيه الطوارئ/دورة المياه
  if (dTawari || dDawraGas || dLowerLeftGas) {
    screenDir[7] = DIR_RIGHT;
    screenBlink[7] = true;
  }
 
  // SCR8 - الوسط / صالة الانتظار
  // لا يوجد X في SCR8.
  // خطر يمين فقط => LEFT.
  // خطر يسار فقط => RIGHT.
  // خطر في الجهتين => BOTH مع وميض.
  if (rightSideDanger && !leftSideDanger) {
    screenDir[8] = DIR_LEFT;
    screenBlink[8] = true;
  } else if (leftSideDanger && !rightSideDanger) {
    screenDir[8] = DIR_RIGHT;
    screenBlink[8] = true;
  } else if (rightSideDanger && leftSideDanger) {
    screenDir[8] = DIR_BOTH;
    screenBlink[8] = true;
  } else {
    screenDir[8] = DIR_BOTH;
  }
 
  // إذا صالة الانتظار نفسها فيها خطر، يجب أن يكون توجيه SCR8 واضحاً ووامضاً.
  if (dEntizar) {
    screenBlink[8] = true;
    if (rightSideDanger && !leftSideDanger) screenDir[8] = DIR_LEFT;
    else if (leftSideDanger && !rightSideDanger) screenDir[8] = DIR_RIGHT;
    else screenDir[8] = DIR_BOTH;
  }
}
 
// =====================
// إرسال الحالة إلى ESP32
// =====================
 
char directionCode(Direction d) {
  if (d == DIR_RIGHT) return 'R';
  if (d == DIR_LEFT) return 'L';
  if (d == DIR_UP) return 'U';
  if (d == DIR_DOWN) return 'D';
  if (d == DIR_BOTH) return 'B';
  if (d == DIR_WARNING) return 'X';
  return 'N';
}
 
void appendSensor(String &sensors, const char *code) {
  if (sensors.length() > 0) sensors += ",";
  sensors += code;
}
 
String activeSensorList() {
  String sensors = "";
 
  if (dEyadah2) appendSensor(sensors, "IR22");
  if (dTamreedFire) appendSensor(sensors, "IR23");
  if (dAmaliyat) appendSensor(sensors, "IR24");
  if (dMakhzanFire) appendSensor(sensors, "IR25");
  if (dIdara) appendSensor(sensors, "IR26");
  if (dEntizar) appendSensor(sensors, "IR27");
  if (dAshea) appendSensor(sensors, "IR28");
  if (dTawari) appendSensor(sensors, "IR29");
  if (dIstiqbal) appendSensor(sensors, "IR30");
  if (dMukhtabarFire) appendSensor(sensors, "IR31");
  if (dSaydaliyaFire) appendSensor(sensors, "IR32");
 
  if (dEyadah1Gas) appendSensor(sensors, "MQ233");
  if (dTamreedGas) appendSensor(sensors, "MQ234");
  if (dMakhzanGas) appendSensor(sensors, "MQ235");
  if (dMatbakhMQ2) appendSensor(sensors, "MQ236");
  if (dDawraGas) appendSensor(sensors, "MQ237");
  if (dSaydaliyaGas) appendSensor(sensors, "MQ238");
  if (dCorrLeft) appendSensor(sensors, "MQ239");
  if (dCorrRight) appendSensor(sensors, "MQ240");
  if (dLowerLeftGas) appendSensor(sensors, "MQ252");
  if (dLowerRightGas) appendSensor(sensors, "MQ253");
 
  if (dMatbakhMQ5) appendSensor(sensors, "MQ541");
  if (dMukhtabarMQ5) appendSensor(sensors, "MQ542");
 
  return sensors;
}
 
void sendStateToESP32() {
  if (millis() - lastStateSendMs < STATE_SEND_INTERVAL_MS) return;
  lastStateSendMs = millis();
 
  String msg = "STATE:";
  msg += "VIB=";
  msg += anyVibration ? "1" : "0";
  msg += ";S=";
  msg += activeSensorList();
  msg += ";SCR=";
 
  for (byte i = 1; i <= 8; i++) {
    if (i > 1) msg += ",";
    msg += String(i);
    msg += ":";
    msg += directionCode(screenDir[i]);
  }
 
  Serial1.println(msg);
  Serial.println(msg);
}
 
 
// =====================
// شاشة LCD 16x4
// =====================
 
void lcdPrintLine(byte row, String text) {
  if (text.length() > LCD_COLS) {
    text = text.substring(0, LCD_COLS);
  }
  while (text.length() < LCD_COLS) {
    text += " ";
  }
  lcd.setCursor(0, row);
  lcd.print(text);
}
 
byte activeAlertCount() {
  byte c = 0;
 
  if (dEyadah2) c++;
  if (dTamreedFire) c++;
  if (dAmaliyat) c++;
  if (dMakhzanFire) c++;
  if (dIdara) c++;
  if (dEntizar) c++;
  if (dAshea) c++;
  if (dTawari) c++;
  if (dIstiqbal) c++;
  if (dMukhtabarFire) c++;
  if (dSaydaliyaFire) c++;
 
  if (dEyadah1Gas) c++;
  if (dTamreedGas) c++;
  if (dMakhzanGas) c++;
  if (dMatbakhMQ2) c++;
  if (dDawraGas) c++;
  if (dSaydaliyaGas) c++;
  if (dCorrLeft) c++;
  if (dCorrRight) c++;
  if (dLowerLeftGas) c++;
  if (dLowerRightGas) c++;
 
  if (dMatbakhMQ5) c++;
  if (dMukhtabarMQ5) c++;
 
  if (anyVibration) c++;
  return c;
}
 
void firstActiveAlert(const char*& sensor, const char*& location, const char*& exits) {
  // الترتيب حسب أولوية العرض على LCD
  sensor = "NONE";
  location = "NONE";
  exits = "ALL SAFE";
 
  if (dAshea) { sensor = "IR28"; location = "ASHEA"; exits = "E2,E1"; return; }
  if (dMatbakhMQ2) { sensor = "MQ2-36"; location = "MATBAKH"; exits = "E3,E4"; return; }
  if (dMatbakhMQ5) { sensor = "MQ5-41"; location = "MATBAKH"; exits = "E3,E4"; return; }
  if (dCorrRight) { sensor = "MQ2-40"; location = "TOP RIGHT"; exits = "E1,MAIN"; return; }
  if (dCorrLeft) { sensor = "MQ2-39"; location = "TOP LEFT"; exits = "E4,MAIN"; return; }
  if (dLowerRightGas) { sensor = "MQ2-53"; location = "LOWER RIGHT"; exits = "E1"; return; }
  if (dLowerLeftGas) { sensor = "MQ2-52"; location = "LOWER LEFT"; exits = "E4"; return; }
 
  if (dEyadah2) { sensor = "IR22"; location = "EYADAH2"; exits = "E3,E4"; return; }
  if (dEyadah1Gas) { sensor = "MQ2-33"; location = "EYADAH1"; exits = "E3,E4"; return; }
  if (dTamreedFire) { sensor = "IR23"; location = "TAMREED"; exits = "E3,E2"; return; }
  if (dTamreedGas) { sensor = "MQ2-34"; location = "TAMREED"; exits = "E3,E2"; return; }
  if (dAmaliyat) { sensor = "IR24"; location = "AMALIYAT"; exits = "E2,E1"; return; }
  if (dMakhzanFire) { sensor = "IR25"; location = "MAKHZAN"; exits = "E2,E1"; return; }
  if (dMakhzanGas) { sensor = "MQ2-35"; location = "MAKHZAN"; exits = "E2,E1"; return; }
  if (dIdara) { sensor = "IR26"; location = "IDARA"; exits = "E2,E1"; return; }
  if (dEntizar) { sensor = "IR27"; location = "ENTIZAR"; exits = "FOLLOW ARW"; return; }
  if (dTawari) { sensor = "IR29"; location = "TAWARI"; exits = "E4,MAIN"; return; }
  if (dIstiqbal) { sensor = "IR30"; location = "ISTIQBAL"; exits = "E1,E4"; return; }
  if (dMukhtabarFire) { sensor = "IR31"; location = "MOKHTABAR"; exits = "E1,E2"; return; }
  if (dMukhtabarMQ5) { sensor = "MQ5-42"; location = "MOKHTABAR"; exits = "E1,E2"; return; }
  if (dSaydaliyaFire) { sensor = "IR32"; location = "SAYDALIYA"; exits = "E1,E2"; return; }
  if (dSaydaliyaGas) { sensor = "MQ2-38"; location = "SAYDALIYA"; exits = "E1,E2"; return; }
  if (dDawraGas) { sensor = "MQ2-37"; location = "DAWRA"; exits = "E4,MAIN"; return; }
  if (anyVibration) { sensor = "VIB"; location = "BUILDING"; exits = "FOLLOW ARW"; return; }
}
 
String lcdSystemState(byte count) {
  if (count == 0) return "NORMAL";
  if (count > 1) return "MULTI";
  if (anyVibration) return "VIBRATION";
  if (anyFire) return "FIRE";
  if (anyGas) return "GAS";
  return "NORMAL";
}
 
void updateLCD() {
  if (millis() - lastLcdUpdateMs < LCD_UPDATE_INTERVAL_MS) return;
  lastLcdUpdateMs = millis();
 
  byte count = activeAlertCount();
  String sys = lcdSystemState(count);
 
  if (count == 0) {
    lcdPrintLine(0, "SYS: NORMAL");
    lcdPrintLine(1, "SEN: NONE");
    lcdPrintLine(2, "LOC: NONE");
    lcdPrintLine(3, "EXIT: ALL SAFE");
    return;
  }
 
  if (count > 1) {
    lcdPrintLine(0, "SYS: MULTI");
    lcdPrintLine(1, "SEN: MANY");
    lcdPrintLine(2, "LOC: MULTI ZONE");
    lcdPrintLine(3, "EXIT: FOLLOW");
    return;
  }
 
  const char* sensor;
  const char* location;
  const char* exits;
  firstActiveAlert(sensor, location, exits);
 
  lcdPrintLine(0, "SYS: " + sys);
  lcdPrintLine(1, String("SEN: ") + sensor);
  lcdPrintLine(2, String("LOC: ") + location);
  lcdPrintLine(3, String("EXIT: ") + exits);
}
 
// =====================
// زر التصفير (Reset Button)
// =====================
 
void checkResetButton() {
  bool raw = digitalRead(RESET_BUTTON_PIN); // LOW = مضغوط (INPUT_PULLUP)
 
  if (raw != resetButtonRawLastState) {
    resetButtonLastChangeMs = millis();
    resetButtonRawLastState = raw;
  }
 
  if ((millis() - resetButtonLastChangeMs) > RESET_BUTTON_DEBOUNCE_MS) {
    if (raw != resetButtonStableState) {
      resetButtonStableState = raw;
      if (resetButtonStableState == LOW) {
        // ضغطة جديدة مؤكدة: صفّر النظام (الميجا + الموقع مع بعض)
        clearManualTests();
        resetDirectionsToNormal();
        clearAllScreens();
        noTone(BUZZER_PIN);
        digitalWrite(RED_LED_RELAY_PIN, LOW);
        Serial1.println("NORMAL");   // يخبر ESP32 يصفّر الموقع بنفس اللحظة
        Serial.println("RESET BUTTON PRESSED -> system + website reset");
      }
    }
  }
}
 
// =====================
// الجرس
// =====================
 
void updateBuzzer() {
  bool ledOn = false;
  if (anyFire) {
    bool phase = (millis() / 180) % 2;
    ledOn = phase;
    if (phase) tone(BUZZER_PIN, 2500);
    else noTone(BUZZER_PIN);
  } else if (anyGas) {
    bool phase = (millis() / 350) % 2;
    ledOn = phase;
    if (phase) tone(BUZZER_PIN, 1200);
    else noTone(BUZZER_PIN);
  } else if (anyVibration) {
    bool phase = (millis() / 500) % 2;
    ledOn = phase;
    if (phase) tone(BUZZER_PIN, 800);
    else noTone(BUZZER_PIN);
  } else {
    noTone(BUZZER_PIN);
    ledOn = false;
  }
  digitalWrite(RED_LED_RELAY_PIN, ledOn ? HIGH : LOW);
}
 
// =====================
// اختبار ذاتي
// =====================
 
void selfTest() {
  for (byte s = 1; s <= 8; s++) {
    writeScreen(s, DIR_RIGHT, true);
  }
  tone(BUZZER_PIN, 1800);
  delay(400);
 
  for (byte s = 1; s <= 8; s++) {
    writeScreen(s, DIR_LEFT, true);
  }
  delay(400);
 
  for (byte s = 1; s <= 8; s++) {
    writeScreen(s, DIR_DOWN, true);
  }
  delay(400);
 
  noTone(BUZZER_PIN);
  clearAllScreens();
}
 
// =====================
// SETUP
// =====================
 
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
 
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcdPrintLine(0, "SMART EVAC SYS");
  lcdPrintLine(1, "LCD 1604 READY");
  lcdPrintLine(2, "MEGA STARTING");
  lcdPrintLine(3, "PLEASE WAIT...");
 
  pinMode(BUZZER_PIN, OUTPUT);
 
  byte firePins[11] = {
    F_EYADAH2, F_TAMREED, F_AMALIYAT, F_MAKHZAN, F_IDARA,
    F_ENTIZAR, F_ASHEA, F_TAWARI, F_ISTIQBAL, F_MUKHTABAR, F_SAYDALIYA
  };
 
  byte mq2Pins[10] = {
    G_EYADAH1, G_TAMREED, G_MAKHZAN, G_MATBAKH, G_DAWRA,
    G_SAYDALIYA, G_CORRIDOR_LEFT, G_CORRIDOR_RIGHT,
    G_LOWER_LEFT, G_LOWER_RIGHT
  };
 
  byte mq5Pins[2] = {
    M_MATBAKH, M_MUKHTABAR
  };
 
  for (byte i = 0; i < 11; i++) {
    pinMode(firePins[i], INPUT_PULLUP);
    initMonitor(fireMon[i], firePins[i], HAZARD_ACTIVE_STATE, FIRE_GAS_HOLD_MS);
  }
 
  for (byte i = 0; i < 10; i++) {
    pinMode(mq2Pins[i], INPUT_PULLUP);
    initMonitor(mq2Mon[i], mq2Pins[i], HAZARD_ACTIVE_STATE, FIRE_GAS_HOLD_MS);
  }
 
  for (byte i = 0; i < 2; i++) {
    pinMode(mq5Pins[i], INPUT_PULLUP);
    initMonitor(mq5Mon[i], mq5Pins[i], HAZARD_ACTIVE_STATE, FIRE_GAS_HOLD_MS);
  }
 
  pinMode(VIB_RIGHT, INPUT);
  pinMode(VIB_LEFT, INPUT);
 
  initMonitor(vibMon[0], VIB_RIGHT, VIBRATION_ACTIVE_STATE, VIBRATION_HOLD_MS);
  initMonitor(vibMon[1], VIB_LEFT, VIBRATION_ACTIVE_STATE, VIBRATION_HOLD_MS);
 
  for (byte i = 0; i < 4; i++) {
    lc1.shutdown(i, false);
    lc1.setIntensity(i, 8);
    lc1.clearDisplay(i);
 
    lc2.shutdown(i, false);
    lc2.setIntensity(i, 8);
    lc2.clearDisplay(i);
  }
 
  clearManualTests();
  resetDirectionsToNormal();
  selfTest();

  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_LED_RELAY_PIN, OUTPUT);
  digitalWrite(RED_LED_RELAY_PIN, LOW);
}
 
// =====================
// LOOP
// =====================
 
void loop() {
  readESP32Commands();
  updateInputs();
  updateDisplayLogic();
  displayScreens();
  checkResetButton();
  updateBuzzer();
  updateLCD();
  sendStateToESP32();
}
