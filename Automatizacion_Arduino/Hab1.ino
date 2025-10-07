/***************************************************************************************
 * Hab1.ino — Habitación 1 (DHT11 + LDR digital + 2 relés + SERVO)
 * - Sensores: DHT11 (temperatura/humedad) en D1 + LDR digital en D0
 * - Actuadores: 2 relés (RELE1, RELE2) y un servo (posicional) para la persiana
 * - Conectividad: Wi-Fi STA (TCP:8080) para App Qt + UART (Serial.swap) hacia Tiva
 * - Pantalla: SSD1306 I2C opcional (pinout más abajo)
 ***************************************************************************************/

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <DHT.h>

/*==============================================================================
 *  CONFIGURACIÓN DE HARDWARE
 *============================================================================*/
// ---- OLED (SSD1306 por I2C) ----
#define OLED_W     128
#define OLED_H      64
#define OLED_SDA    D6          // GPIO12
#define OLED_SCL    D5          // GPIO14
#define OLED_ADDR   0x3C
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);
bool display_ok = false;

// ---- Sensor temperatura DHT11 ----
#define DHTPIN      D1          // ⚠️ Cambiado de D5→D1 para no chocar con I2C
#define DHTTYPE     DHT11
DHT dht(DHTPIN, DHTTYPE);
static float g_lastT = NAN, g_lastH = NAN;
static uint32_t g_lastSampleMs = 0;
const uint32_t DHT_INTERVAL_MS = 2200; // DHT11 ≳2 s

// Temperatura: umbrales + histéresis
enum TempLevel { TEMP_NORMAL, TEMP_BAJA, TEMP_ALTA };
TempLevel g_tempLevel = TEMP_NORMAL;
float TEMP_LOW_ENTER  = 18.0;  // < 18°C -> BAJA
float TEMP_HIGH_ENTER = 27.0;  // > 27°C -> ALTA
float TEMP_LOW_EXIT   = 19.0;  // salir de BAJA cuando suba a >= 19°C
float TEMP_HIGH_EXIT  = 26.0;  // salir de ALTA cuando baje a <= 26°C

// ---- Sensor de Luz ----
#define LDR_DIG_PIN      D0     // GPIO16
#define LDR_ACTIVE_HIGH  1      // 1: HIGH = luz alta; 0: invertido

// ---- Relés (2 canales) ----
const int RELE1 = D2;           // GPIO4
const int RELE2 = D3;           // GPIO0
#define RELAY_ACTIVE_HIGH 0     // la mayoría de módulos son active-LOW
bool g_relay1 = false, g_relay2 = false;

// ---- Servo (persiana/puerta) ----
#define SERVO_PIN        D4     // ajusta si prefieres otro
#define SERVO_MIN_US     500
#define SERVO_MAX_US    2500
#define BLIND_STEP_PCT     5    // tamaño del paso en % para STEP_UP/DOWN
Servo servoBlind;               // ← ÚNICA instancia del servo
int   blindPct = 0;             // posición 0..100%

// ---- Wi-Fi (STA simple) ----
const char* ssid     = "Wifi123";
const char* password = "korv6680";
WiFiServer wifiServer(8080);
WiFiClient client;

/*==============================================================================
 *  ADELANTOS / PROTOTIPOS
 *============================================================================*/
void displayInit();
void disp(const String& l1, const String& l2=String(), const String& l3=String());
void setupWiFiSimple();

inline void sendLineToTiva(const String& s) { Serial.print(s); Serial.print('\n'); }
inline void sendLineToClient(const String& s) { if (client && client.connected()) { client.print(s); client.print('\n'); } }

// Relés
inline void writeRelayHW(int pin, bool on) { bool level = RELAY_ACTIVE_HIGH ? on : !on; pinMode(pin, OUTPUT); digitalWrite(pin, level); }
void setRelay1(bool on, bool announce = true);
void setRelay2(bool on, bool announce = true);

// Servo
inline int pctToUs(int p){ if(p<0) p=0; if(p>100) p=100; long span = SERVO_MAX_US - SERVO_MIN_US; return SERVO_MIN_US + (int)(span * p / 100L); }
void servoInit();
void servoGotoPercent(int p);
void servoOpen();
void servoClose();
void servoStop();
void blindStepUp();
void blindStepDown();

// Sensores/estados
String buildStatusString(float t, int ldrD, bool ldrActiveHigh, float umbralT = 27.0f);
bool readDHT(float* outT, float* outH);
void enviarEstadosYLecturas(bool forceSend=false, bool notifyApp=false);
bool handleBlindOrServoCommand(const String& cmd);
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
  display.println("Hab1 listo");
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
 *  RELÉS
 *============================================================================*/
void setRelay1(bool on, bool announce) {
  g_relay1 = on;
  writeRelayHW(RELE1, on);
  if (announce) sendLineToClient(on ? "RELE1_ON" : "RELE1_OFF");
}
void setRelay2(bool on, bool announce) {
  g_relay2 = on;
  writeRelayHW(RELE2, on);
  if (announce) sendLineToClient(on ? "RELE2_ON" : "RELE2_OFF");
}

/*==============================================================================
 *  SERVO (posicional 0..100%)
 *============================================================================*/
void servoInit() {
  servoBlind.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
  servoBlind.writeMicroseconds(pctToUs(blindPct));
}
void servoGotoPercent(int p) { if(p<0)p=0; if(p>100)p=100; blindPct = p; servoBlind.writeMicroseconds(pctToUs(blindPct)); }
void servoOpen()            { servoGotoPercent(100); }
void servoClose()           { servoGotoPercent(0); }
void servoStop()            { /* posicional: no hace falta freno */ }
void blindStepUp()          { servoGotoPercent(blindPct + BLIND_STEP_PCT); }
void blindStepDown()        { servoGotoPercent(blindPct - BLIND_STEP_PCT); }

/*==============================================================================
 *  SENSORES Y MENSAJES
 *============================================================================*/
String buildStatusString(float t, int ldrD, bool ldrActiveHigh, float umbralT) {
  String s;
  if (!isnan(t)) s += (t >= umbralT ? "TEMPERATURA_ALTA" : "TEMPERATURA_BAJA");
  if (s.length()) s += ';';
  bool luzAlta = ldrActiveHigh ? (ldrD == HIGH) : (ldrD == LOW);
  s += luzAlta ? "LUMINOSIDAD_ALTA" : "LUMINOSIDAD_BAJA";
  return s;
}


bool readDHT(float* outT, float* outH) {
  uint32_t now = millis();
  if ((now - g_lastSampleMs) < DHT_INTERVAL_MS && !isnan(g_lastT) && !isnan(g_lastH)) { *outT=g_lastT; *outH=g_lastH; return true; }
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) { delay(60); h = dht.readHumidity(); t = dht.readTemperature(); }
  if (!isnan(h) && !isnan(t)) { g_lastH=h; g_lastT=t; g_lastSampleMs=millis(); *outT=t; *outH=h; return true; }
  if (!isnan(g_lastT) && !isnan(g_lastH)) { *outT=g_lastT; *outH=g_lastH; return true; }
  return false;
}

void enviarEstadosYLecturas(bool forceSend, bool notifyApp) {
  float t = NAN, h = NAN;
  bool ok = readDHT(&t, &h);
  int  d = digitalRead(LDR_DIG_PIN);
  String s = "Ventana_pos=" + String(blindPct);   
  // A la Tiva: siempre al menos la LUMINOSIDAD_*; si hay temp válida, añade TEMPERATURA_*
  if (ok) {
    // Histéresis temperatura
    TempLevel next = g_tempLevel;
    switch (g_tempLevel) {
      case TEMP_NORMAL:
        if (t <= TEMP_LOW_ENTER)       next = TEMP_BAJA;
        else if (t >= TEMP_HIGH_ENTER) next = TEMP_ALTA;
        break;
      case TEMP_BAJA: if (t >= TEMP_LOW_EXIT)  next = TEMP_NORMAL; break;
      case TEMP_ALTA: if (t <= TEMP_HIGH_EXIT) next = TEMP_NORMAL; break;
    }
    g_tempLevel = next;
    sendLineToTiva(buildStatusString(t, d, LDR_ACTIVE_HIGH));
  } else {
    bool luzAlta = LDR_ACTIVE_HIGH ? (d == HIGH) : (d == LOW);
    sendLineToTiva(luzAlta ? "LUMINOSIDAD_ALTA" : "LUMINOSIDAD_BAJA");
  }
  // A la App (debug): TEMP/HUM/LDRD
  if (notifyApp){
    sendLineToClient("TEMP=" + String(t,1) + ";HUM=" + String(h,1) + ";LDRD=" + String(d));
    sendLineToClient(s);
    disp("Datos______" "\nTEMP=" + String(t,1) + "\nHUM=" + String(h,1) + "\nLDRD=" + String(d) + "\n" + s );
  }
}

/*==============================================================================
 *  PARSER DE COMANDOS (Qt + Tiva)
 *============================================================================*/
bool handleBlindOrServoCommand(const String& cmd) {
  if (cmd == "BLIND_OPEN")  { servoOpen();  sendLineToClient("Subir ventana del todo"); return true; }
  if (cmd == "BLIND_CLOSE")  { servoClose(); sendLineToClient("Bajar ventana del todo"); return true; }
  if (cmd == "BLIND_STOP")  { servoStop();  sendLineToClient("OK"); return true; }
  if (cmd == "BLIND_STEP_UP")   { blindStepUp();   sendLineToClient("Subir ventana"); return true; }
  if (cmd == "BLIND_STEP_DOWN") { blindStepDown(); sendLineToClient("Bajar ventana"); return true; }
  if (cmd.startsWith("BLIND_GOTO=") || cmd.startsWith("SERVO_GOTO=")) {
    int p = cmd.substring(cmd.indexOf('=')+1).toInt();
    servoGotoPercent(p); sendLineToClient(String("OK GOTO ")+p); return true;
  }
  return false;
}

void manejarComando(const String& raw, bool fromApp) {
  String cmd = raw; cmd.trim(); cmd.toUpperCase();
  if (!cmd.length()) return;

  // Relés
  if (cmd == "RELE1_ON")  { setRelay1(true);  disp("Luz 1 encendida"); return; }
  if (cmd == "RELE1_OFF") { setRelay1(false); disp("Luz 1 apagada");   return; }
  if (cmd == "RELE2_ON")  { setRelay2(true);  disp("Luz 2 encendida"); return; }
  if (cmd == "RELE2_OFF") { setRelay2(false); disp("Luz 2 apagada");   return; }

  // Automático (la Tiva se entera)
  if (cmd == "AUTO_ON")   { sendLineToTiva("AUTO_ON");  sendLineToClient("Modo auto ON"); disp("Modo auto ON");  return; }
  if (cmd == "AUTO_OFF")  { sendLineToTiva("AUTO_OFF"); sendLineToClient("Modo auto OFF"); disp("Modo auto OFF"); return; }

  // Persiana / servo
  if (handleBlindOrServoCommand(cmd)) return;
  // Lecturas / estados
  if (cmd == "PEDIR_DATOS") { enviarEstadosYLecturas(true, fromApp); return; }
}

/*==============================================================================
 *  SETUP / LOOP
 *============================================================================*/
void setup() {
  // UART a Tiva en pines alternativos (D8 TX / D7 RX)
  Serial.begin(9600);
  Serial.swap();

  // OLED
  displayInit();

  // Sensores
  dht.begin();
  delay(1500);
  pinMode(LDR_DIG_PIN, INPUT);   // o INPUT_PULLUP según tu módulo

  // Relés: OFF seguros al arranque
  pinMode(RELE1, OUTPUT); pinMode(RELE2, OUTPUT);
  setRelay1(false, false);
  setRelay2(false, false);

  // Servo
  servoInit();

  // Wi-Fi
  setupWiFiSimple();

  disp("Hab1 lista", "WiFi OK", "");
}

void loop() {
  // Aceptar un cliente TCP si llega (uno a la vez)
  if (!client || !client.connected()) client = wifiServer.available();

  // Comandos desde App TCP
  if (client && client.available()) {
    String line = client.readStringUntil('\n');
    manejarComando(line,true);
  }

  // Comandos desde Tiva (UART)
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    manejarComando(line,false);
  }

}






