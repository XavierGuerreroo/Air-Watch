#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include <U8g2lib.h>

// ===================== OLED =====================
#define OLED_SDA 17
#define OLED_SCL 18
#define VEXT_PIN 36

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ===================== UART hacia LilyGO =====================
#define TX_TO_LILYGO 26
HardwareSerial SerialLily(1);

// ===================== Control de frecuencia hacia LilyGO =====================
unsigned long ultimoEnvioLily = 0;
const unsigned long intervaloEnvioLily = 2000; // 2 segundos

// ===================== LoRa =====================
#define RF_FREQUENCY              915000000
#define TX_OUTPUT_POWER           14
#define LORA_BANDWIDTH            0
#define LORA_SPREADING_FACTOR     7
#define LORA_CODINGRATE           1
#define LORA_PREAMBLE_LENGTH      8
#define LORA_SYMBOL_TIMEOUT       0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON      false

static RadioEvents_t RadioEvents;

char rxpacket[256];
bool lora_idle = false;
int16_t rssi, rxSize;

// ===================== Variables parseadas =====================
String tvocStr = "---";
String eco2Str = "---";
String micsStr = "---";
String tempStr = "--.-";
String humStr  = "--.-";

// ===================== Funciones =====================
String extraerValor(String json, String clave) {
  int inicio = json.indexOf("\"" + clave + "\":");
  if (inicio == -1) return "";

  inicio += clave.length() + 3;

  int fin = json.indexOf(",", inicio);
  if (fin == -1) fin = json.indexOf("}", inicio);
  if (fin == -1) return "";

  return json.substring(inicio, fin);
}

bool jsonBasicoValido(String json) {
  json.trim();

  if (!json.startsWith("{") || !json.endsWith("}")) {
    return false;
  }

  if (json.indexOf("\"tvoc\"") == -1) return false;
  if (json.indexOf("\"eco2\"") == -1) return false;
  if (json.indexOf("\"mics\"") == -1) return false;
  if (json.indexOf("\"temp\"") == -1) return false;
  if (json.indexOf("\"hum\"") == -1) return false;

  return true;
}

void mostrarOLED() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);

  u8g2.drawStr(0, 10, "LoRa RX OK");

  String l1 = "TVOC:" + tvocStr;
  String l2 = "eCO2:" + eco2Str;
  String l3 = "MiCS:" + micsStr;
  String l4 = "T:" + tempStr + "C";
  String l5 = "H:" + humStr + "%";
  String l6 = "RSSI:" + String(rssi);

  u8g2.drawStr(0, 22, l1.c_str());
  u8g2.drawStr(0, 32, l2.c_str());
  u8g2.drawStr(0, 42, l3.c_str());
  u8g2.drawStr(0, 52, l4.c_str());
  u8g2.drawStr(64, 52, l5.c_str());
  u8g2.drawStr(0, 62, l6.c_str());

  u8g2.sendBuffer();
}

// ===================== LoRa callbacks =====================
void OnRxDone(uint8_t *payload, uint16_t size, int16_t _rssi, int8_t snr) {
  rssi = _rssi;
  rxSize = size;

  if (size >= sizeof(rxpacket)) {
    size = sizeof(rxpacket) - 1;
  }

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  String json = String(rxpacket);
  json.trim();

  tvocStr = extraerValor(json, "tvoc");
  eco2Str = extraerValor(json, "eco2");
  micsStr = extraerValor(json, "mics");
  tempStr = extraerValor(json, "temp");
  humStr  = extraerValor(json, "hum");

  if (tvocStr == "") tvocStr = "---";
  if (eco2Str == "") eco2Str = "---";
  if (micsStr == "") micsStr = "---";
  if (tempStr == "") tempStr = "--.-";
  if (humStr  == "") humStr  = "--.-";

  Radio.Sleep();
  lora_idle = true;
}

void OnRxTimeout(void) {
  Serial.println("LoRa RX Timeout");
  Radio.Sleep();
  lora_idle = true;
}

void OnRxError(void) {
  Serial.println("LoRa RX Error");
  Radio.Sleep();
  lora_idle = true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("========================================");
  Serial.println("HELTEC 2 RECEIVER + OLED + UART CONTROLADO");
  Serial.println("Recibe LoRa y envia a LilyGO cada 2 segundos");
  Serial.println("========================================");

  // ===================== UART a LilyGO =====================
  SerialLily.begin(115200, SERIAL_8N1, -1, TX_TO_LILYGO);
  Serial.println("UART hacia LilyGO lista");

  // ===================== OLED =====================
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  delay(200);

  Wire.begin(OLED_SDA, OLED_SCL);
  delay(200);

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 15, "Receiver iniciando...");
  u8g2.sendBuffer();

  // ===================== LoRa =====================
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.RxDone = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  Serial.println("Receiver listo, esperando paquetes...");

  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "Receiver listo");
  u8g2.drawStr(0, 30, "Esperando LoRa...");
  u8g2.sendBuffer();

  Radio.Rx(0);
}

void loop() {
  Radio.IrqProcess();

  if (lora_idle) {
    lora_idle = false;

    String json = String(rxpacket);
    json.trim();

    Serial.println("--------------------------------------------------");
    Serial.println("Paquete LoRa recibido:");
    Serial.println(json);
    Serial.print("RSSI: ");
    Serial.println(rssi);

    // Mostrar en OLED siempre que llegue paquete
    mostrarOLED();

    // Validar JSON antes de enviarlo a LilyGO
    if (!jsonBasicoValido(json)) {
      Serial.println("JSON invalido o incompleto. No se envia a LilyGO.");
    } 
    else if (millis() - ultimoEnvioLily >= intervaloEnvioLily) {
      SerialLily.println(json);
      ultimoEnvioLily = millis();

      Serial.println("Enviado a LilyGO:");
      Serial.println(json);
    } 
    else {
      Serial.println("Paquete omitido para no saturar LilyGO.");
    }

    Serial.println("--------------------------------------------------");
    Serial.println();

    // Seguir escuchando LoRa
    Radio.Rx(0);
  }
}