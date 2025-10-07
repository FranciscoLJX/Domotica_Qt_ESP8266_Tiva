/***************************************************************************************
 * Hab2.ino — Habitación 2 (estructura gemela a Hab1, limpia y ordenada)
 * - Sensor luz VEML7700 (I2C) → envía SOLO comandos: LUMINOSIDAD_BAJA/MEDIA/ALTA a la Tiva
 * - Motor DC con driver (AIN1/AIN2/PWM/STBY) + encoder 1 canal → topes 0..MAX_TURNS + failsafe
 * - Wi-Fi STA (TCP:8080) para App Qt + UART (Serial.swap) con la Tiva
 * - OLED SSD1306 (I2C) opcional (mismo bus que el VEML)
 ***************************************************************************************/

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_VEML7700.h>
#include <math.h>

/*==============================================================================
 *  PINES Y PARÁMETROS (AJUSTA A TU CABLEADO)
 *============================================================================*/
// ---- OLED (SSD1306 por I2C) ----
#define OLED_W     128
#define OLED_H      64
#define OLED_SDA    D6          // GPIO12
#define OLED_SCL    D5          // GPIO14
#define OLED_ADDR   0x3C
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool display_ok = false;


// --- Wi-Fi ---
const char* ssid     = "Wifi123";
const char* password = "korv6680";
WiFiServer wifiServer(8080);
WiFiClient client;


// --- Relé (si usas uno en Hab2) ---
/*#define RELE1_PIN        D4          // CAMBIA si tu relé está en otro pin
#define RELAY_ACTIVE_HIGH 0          // 0 = activo a nivel bajo (lo más común)*/

// --- Driver motor DC (TB6612 u otro similar) ---
#define PIN_PWMA         D1          // PWM del motor  (CAMBIA si lo tienes en otro)
#define PIN_AIN1         D0          // dirección
#define PIN_AIN2         D4          // dirección
#define PIN_STBY         D3         // standby/enable

// --- Encoder (1 canal). Si no tienes índice, deja ENC_I_PIN = -1 ---
#define ENC_A_PIN        D2
#define ENC_I_PIN        -1

// --- PWM y movimiento ---
#define PWM_RUN          650         // duty (0..1023)
#define MOVE_MAX_MS      30000UL     // failsafe tiempo máximo movimiento

// --- Encoder/Topes (vueltas totales = 0..100%) ---
#define ENC_PULSES_PER_REV  11
#define ENC_MIN_US          300      // antirrebote ISR
#define MAX_TURNS           100      // tope “alto” en vueltas

// --- LUZ: umbrales e histéresis (trabaja en % relativo 0..100) ---
enum LuxLevel { LUX_MUY_BAJA=0, LUX_BAJA=1, LUX_MEDIA=2, LUX_ALTA=3, LUX_MUY_ALTA=4 };
const uint8_t LUX_UP_THRESH[4]   = { 10, 25, 50, 75 }; // para SUBIR de nivel
const uint8_t LUX_DOWN_THRESH[4] = {  8, 20, 45, 70 }; // para BAJAR de nivel
const float   LUX_FILTER_ALPHA   = 0.20f;              // suavizado EMA
float         LUX_DARK_REF       = 10.0f;              // “oscuro”
float         LUX_BRIGHT_REF     = 500.0f;             // “muy claro”

/*==============================================================================
 *  OBJETOS Y ESTADO GLOBAL
 *============================================================================*/

// TCP / UART Tiva
static inline void sendLineToTiva(const String& s){ Serial.print(s); Serial.print('\n'); }
static inline void sendLineToClient(const String& s){ if (client && client.connected()) { client.print(s); client.print('\n'); } }

// Relé
bool g_rele1 = false;

// Luz
Adafruit_VEML7700 veml;
float    g_luxFilt   = -1.0f;
LuxLevel g_luxLevel  = LUX_MEDIA;

// Persiana + encoder
enum { BLIND_IDLE, BLIND_OPENING, BLIND_CLOSING };
volatile long   g_encCount      = 0;
volatile int8_t g_encDir        = 0;         // +1 abrir, −1 cerrar
long            g_encZero       = 0;
long            g_encCountsFull = (long)ENC_PULSES_PER_REV * (long)MAX_TURNS;
int             g_blindPct      = 0;         // 0..100
int             g_blindTarget   = 0;
int             g_blindState    = BLIND_IDLE;
unsigned long   g_moveStartedMs = 0;

/*==============================================================================
 *  PROTOTIPOS
 *============================================================================*/
//Wi-Fi
void setupWiFiSimple();

//oled
void displayInit();
void disp(const String& l1, const String& l2=String(), const String& l3=String());

// Luz
float    luxToPct(float lux);
LuxLevel classifyLuxPct(float pct, LuxLevel prev);
void enviarEstadosYLecturas(bool forceSend=false, bool notifyApp=false);


// Relé
/*void writeRelayHW(int pin, bool on);
void setRelay1(bool on, bool announce=true);*/

// Motor + encoder + persiana
void motorPinsInit();
void motorRunOpen();
void motorRunClose();
void motorCoastStop();
void encoderInit();
void IRAM_ATTR encA_ISR();
void IRAM_ATTR encI_ISR();

int   countsToPct(long c);
long  pctToCounts(int pct);
void  blindStop();
void  blindStartToTarget();
void  blindOpen();
void  blindClose();
void  blindGotoPercent(int p);
void  blindStepUp();
void  blindStepDown();
void  blindLoop();

// Parser
bool   handleBlindCommand(const String& cmd);
void manejarComando(const String& raw, bool fromApp);



/*==============================================================================
 *  OLED 
 *============================================================================*/
void displayInit() {
  Wire.begin(OLED_SDA, OLED_SCL);
  delay(50);
  display_ok = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!display_ok) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.display();
}

void disp(const String& l1, const String& l2, const String& l3) {
  if (!display_ok) return;
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(l1);
  if (l2.length()) display.println(l2);
  if (l3.length()) display.println(l3);
  display.display();
}

/*==============================================================================
 *  Wi-Fi STA (TCP:8080)
 *============================================================================*/
void setupWiFiSimple() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) delay(150);
  wifiServer.begin();
}

/*==============================================================================
 *  LUZ (VEML7700 → niveles con histéresis)
 *============================================================================*/
float luxToPct(float lux){
  if (lux < 0) return -1;
  float den = fmaxf(5.0f, LUX_BRIGHT_REF - LUX_DARK_REF);
  float pct = 100.0f * (lux - LUX_DARK_REF) / den;
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  return pct;
}
LuxLevel classifyLuxPct(float pct, LuxLevel prev){
  LuxLevel lvl = prev;
  if (pct < 0) return prev;
  switch (prev){
    case LUX_MUY_BAJA: if (pct >= LUX_UP_THRESH[0]) lvl = LUX_BAJA; break;
    case LUX_BAJA:
      if (pct >= LUX_UP_THRESH[1]) lvl = LUX_MEDIA;
      else if (pct <= LUX_DOWN_THRESH[0]) lvl = LUX_MUY_BAJA;
      break;
    case LUX_MEDIA:
      if (pct >= LUX_UP_THRESH[2]) lvl = LUX_ALTA;
      else if (pct <= LUX_DOWN_THRESH[1]) lvl = LUX_BAJA;
      break;
    case LUX_ALTA:
      if (pct >= LUX_UP_THRESH[3]) lvl = LUX_MUY_ALTA;
      else if (pct <= LUX_DOWN_THRESH[2]) lvl = LUX_MEDIA;
      break;
    case LUX_MUY_ALTA:
      if (pct <= LUX_DOWN_THRESH[3]) lvl = LUX_ALTA;
      break;
  }
  return lvl;
}

void enviarEstadosYLecturas(bool forceSend, bool notifyApp) {
  float lux = veml.readLux();
  if (isnan(lux)) lux = -1.0f;

  // filtro
  if (lux >= 0) {
    if (g_luxFilt < 0) g_luxFilt = lux;
    else g_luxFilt = g_luxFilt + LUX_FILTER_ALPHA * (lux - g_luxFilt);
  }

  float pct = (lux >= 0) ? luxToPct(g_luxFilt) : -1.0f;
  LuxLevel newLvl = (pct >= 0) ? classifyLuxPct(pct, g_luxLevel) : g_luxLevel;
  bool levelChanged = (newLvl != g_luxLevel);
  g_luxLevel = newLvl;
  String s = "Ventana_pos=" + String(g_blindPct);

  // A la Tiva SOLO comandos de nivel
  if (pct >= 0) {
    if (forceSend || levelChanged) {
      if (g_luxLevel >= LUX_ALTA)       sendLineToTiva("LUMINOSIDAD_ALTA");
      else if (g_luxLevel <= LUX_BAJA)  sendLineToTiva("LUMINOSIDAD_BAJA");
      else                              sendLineToTiva("LUMINOSIDAD_MEDIA");
    }
  } else {
    sendLineToTiva("LUX_ERROR");
  }

  if (notifyApp) { 
      sendLineToClient(String(" Lectura_Luz_Cr=") + lux);
      sendLineToClient(String(" Nivel_Luz_Fil=") + g_luxFilt);
      sendLineToClient(String(" Nivel_Luz_Rel=") + (pct<0?-1:(int)round(pct)));
      sendLineToClient(s);
      disp( "Datos___" + String("\nLectura_Luz_Cr=")+lux+"/nNivel_Luz_Fil="+g_luxFilt+"/nNivel_Luz_Rel="+(pct<0?-1:(int)round(pct)) + "\n" + s);
   }

}

/*==============================================================================
 *  RELÉ
 *============================================================================*/
/*void writeRelayHW(int pin, bool on){
  bool level = RELAY_ACTIVE_HIGH ? on : !on;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, level);
}
void setRelay1(bool on, bool announce){
  g_rele1 = on;
  writeRelayHW(RELE1_PIN, on);
  if (announce) sendLineToClient(on ? "RELE1_ON" : "RELE1_OFF");
}*?

/*==============================================================================
 *  MOTOR + ENCODER + PERSIANA
 *============================================================================*/
void motorPinsInit(){
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_PWMA, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);
  digitalWrite(PIN_STBY, HIGH);
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  analogWriteRange(1023);
  analogWriteFreq(2000);
}
void motorRunOpen(){  digitalWrite(PIN_STBY,HIGH); digitalWrite(PIN_AIN1,HIGH); digitalWrite(PIN_AIN2,LOW);  analogWrite(PIN_PWMA, PWM_RUN); }
void motorRunClose(){ digitalWrite(PIN_STBY,HIGH); digitalWrite(PIN_AIN1,LOW);  digitalWrite(PIN_AIN2,HIGH); analogWrite(PIN_PWMA, PWM_RUN); }
void motorCoastStop(){ analogWrite(PIN_PWMA,0); digitalWrite(PIN_AIN1,LOW); digitalWrite(PIN_AIN2,LOW); }

void IRAM_ATTR encA_ISR(){
  static uint8_t last=1;
  static uint32_t lastUs=0;
  uint8_t  v   = digitalRead(ENC_A_PIN);
  uint32_t now = micros();
  if (v && !last && (now - lastUs) > ENC_MIN_US) {
    g_encCount += g_encDir;             // cuenta según dir actual
    lastUs = now;
  }
  last = v;
}
void IRAM_ATTR encI_ISR(){
#if (ENC_I_PIN >= 0)
  static uint8_t last=1;
  uint8_t v = digitalRead(ENC_I_PIN);
  if (v && !last) { /* índice por vuelta si lo quieres usar */ }
  last = v;
#endif
}
void encoderInit(){
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), encA_ISR, CHANGE);
#if (ENC_I_PIN >= 0)
  pinMode(ENC_I_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_I_PIN), encI_ISR, CHANGE);
#endif
}

int countsToPct(long c){
  long full  = max(1L, g_encCountsFull);
  long delta = c - g_encZero;
  if (delta < 0)   delta = 0;
  if (delta > full) delta = full;
  long pct = (delta * 100L) / full;
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  return (int)pct;
}
long pctToCounts(int pct){
  pct = constrain(pct, 0, 100);
  long full = max(1L, g_encCountsFull);
  return g_encZero + (long)((pct/100.0f) * (float)full + 0.5f);
}

void blindStop(){
  g_blindState = BLIND_IDLE;
  g_encDir = 0;
  motorCoastStop();
}
void blindStartToTarget(){
  if (g_blindTarget > g_blindPct) { g_blindState = BLIND_OPENING; g_encDir = +1; motorRunOpen(); }
  else if (g_blindTarget < g_blindPct) { g_blindState = BLIND_CLOSING; g_encDir = -1; motorRunClose(); }
  else { blindStop(); return; }
  g_moveStartedMs = millis();
}
void blindOpen(){  g_blindTarget = 100; blindStartToTarget(); }
void blindClose(){ g_blindTarget = 0;   blindStartToTarget(); }
void blindGotoPercent(int p){ g_blindTarget = constrain(p,0,100); blindStartToTarget(); }
void blindStepUp(){   blindGotoPercent(g_blindPct + 5); }
void blindStepDown(){ blindGotoPercent(g_blindPct - 5); }

void blindLoop(){
  const long cMin = g_encZero;
  const long cMax = g_encZero + g_encCountsFull;

  if (g_encCount < cMin) g_encCount = cMin;
  if (g_encCount > cMax) g_encCount = cMax;

  int pct = countsToPct(g_encCount);
  if (pct != g_blindPct) { g_blindPct = pct; /* si quieres, envía BLIND_POS aquí */ }

  if (g_blindState == BLIND_OPENING && g_blindPct >= g_blindTarget) blindStop();
  else if (g_blindState == BLIND_CLOSING && g_blindPct <= g_blindTarget) blindStop();

  if (g_blindState == BLIND_OPENING && g_encCount >= cMax) blindStop();
  if (g_blindState == BLIND_CLOSING && g_encCount <= cMin) blindStop();

  if (g_blindState != BLIND_IDLE && (millis() - g_moveStartedMs) > MOVE_MAX_MS) blindStop();
}

/*==============================================================================
 *  PARSER
 *============================================================================*/
bool handleBlindCommand(const String& cmd){
  if (cmd == "BLIND_OPEN") { blindOpen(); sendLineToClient("Subir ventana del todo");return true; }
  if (cmd == "BLIND_CLOSE") { blindClose(); sendLineToClient("Bajar ventana del todo"); return true; }
  //if (cmd == "BLIND_STOP") { blindStop(); sendLineToClient("OK"); return true; }
  if (cmd == "BLIND_STEP_UP") { blindStepUp(); sendLineToClient("Subir ventana"); return true; }
  if (cmd == "BLIND_STEP_DOWN") { blindStepDown(); sendLineToClient("Bajar ventana"); return true; }
  if (cmd.startsWith("BLIND_GOTO=")) {
    int v = cmd.substring(sizeof("BLIND_GOTO=")-1).toInt();
    blindGotoPercent(v); return true;
  }
  return false;
}

void manejarComando(const String& raw, bool fromApp){
  String cmd = raw; cmd.trim(); cmd.toUpperCase();
  if (!cmd.length()) return;

  // Persiana
  if (handleBlindCommand(cmd)) return;

  // Relé
  //if (cmd == "RELE1_ON")  { setRelay1(true);  sendLineToClient("Luz 1 encendida"); return; }
  //if (cmd == "RELE1_OFF") { setRelay1(false); sendLineToClient("Luz 1 apagada");   return; }

  // Automático (solo notifica a la Tiva)
  if (cmd == "AUTO_ON")   { sendLineToTiva("AUTO_ON");  sendLineToClient("Modo auto ONK"); disp("Modo auto ON"); return; }
  if (cmd == "AUTO_OFF")  { sendLineToTiva("AUTO_OFF"); sendLineToClient("Modo auto OFF"); disp("Modo auto OFF");return; }

  // Sonda de estados (la Tiva te pide esto cada 1 s)
  if (cmd == "PEDIR_DATOS"){ enviarEstadosYLecturas(true, fromApp); return; }
}


/*==============================================================================
 *  SETUP / LOOP
 *============================================================================*/
void setup(){
  // UART hacia Tiva (D8 TX / D7 RX)
  Serial.begin(9600);
  Serial.swap();

  // I2C (OLED + VEML)
  displayInit();


  // Relé OFF seguro
  /*pinMode(RELE1_PIN, OUTPUT);
  setRelay1(false, false);*/

  // Motor + encoder
  motorPinsInit();
  encoderInit();
  
  // VEML7700
  if (!veml.begin()) {
    sendLineToClient("VEML7700 NOK");
  } else {
    veml.setGain(VEML7700_GAIN_1);
    veml.setIntegrationTime(VEML7700_IT_100MS);   // ← API correcta
  }

  // Wi-Fi
  setupWiFiSimple();

  disp("Hab2 lista", "WiFi OK", "");

}

void loop(){
  // Aceptar cliente TCP
   if (!client || !client.connected()) client = wifiServer.available();

  // Comandos desde App (TCP)
  if (client && client.available()) {
    String line = client.readStringUntil('\n');
    manejarComando(line,true);
  }


  // Comandos desde Tiva (UART)
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    manejarComando(line,false);
  }


  // Control del movimiento por encoder
  blindLoop();

  // Sonda luz cada 1 s (si cambia nivel → manda a Tiva)
  //static unsigned long t0=0;
  //if (millis() - t0 > 1000) { t0 = millis(); enviarEstadosYLecturas(false); }
}
