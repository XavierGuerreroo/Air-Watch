/*
  AIR-WATCH - Heltec 2 / Nodo receptor

  Este código recibe por LoRa el JSON enviado por la Heltec 1,
  muestra las lecturas principales en la pantalla OLED y reenvía
  el mismo JSON por UART hacia la LilyGO.

  Esta placa funciona como puente:
  LoRa -> UART -> LilyGO
*/

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include <U8g2lib.h>

// Pines de la pantalla OLED
#define OLED_SDA 17
#define OLED_SCL 18
#define VEXT_PIN 36

// Objeto para manejar la pantalla OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// UART hacia LilyGO
#define TX_TO_LILYGO 26
HardwareSerial SerialLily(1);

// Control para no saturar la LilyGO con demasiados envíos seguidos
unsigned long ultimoEnvioLily = 0;
const unsigned long intervaloEnvioLily = 2000; // 2 segundos

// Configuración LoRa. Debe coincidir con la Heltec 1.
#define RF_FREQUENCY              915000000
#define TX_OUTPUT_POWER           14
#define LORA_BANDWIDTH            0       // 125 kHz
#define LORA_SPREADING_FACTOR     7
#define LORA_CODINGRATE           1       // 4/5
#define LORA_PREAMBLE_LENGTH      8
#define LORA_SYMBOL_TIMEOUT       0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON      false

static RadioEvents_t RadioEvents;

// Buffer donde se guarda el paquete recibido por LoRa
char rxpacket[256];

bool lora_idle = false;
int16_t rssi, rxSize;

// Variables que se muestran en la OLED
String tvocStr = "---";
String eco2Str = "---";
String micsStr = "---";
String tempStr = "--.-";
String humStr  = "--.-";

// Extrae un valor simple desde el JSON recibido.
// No usa librería JSON para hacerlo más ligero en la placa.
String extraerValor(String json, String clave) {
  int inicio = json.indexOf("\"" + clave + "\":");
  if (inicio == -1) return "";

  inicio += clave.length() + 3;

  int fin = json.indexOf(",", inicio);
  if (fin == -1) fin = json.indexOf("}", inicio);
  if (fin == -1) return "";

  return json.substring(inicio, fin);
}

// Revisa que el paquete tenga la estructura básica esperada.
// Esto evita enviar basura o paquetes incompletos hacia la LilyGO.
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

// Muestra en pantalla las últimas lecturas recibidas.
// Sirve para verificar el sistema sin depender de la computadora.
void mostrarOLED() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);

  u8g2.drawStr(0, 10, "AIR-WATCH RX");

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

// Se ejecuta cuando llega un paquete LoRa
void OnRxDone(uint8_t *payload, uint16_t size, int16_t _rssi, int8_t snr) {
  rssi = _rssi;
  rxSize = size;

  // Evita que el paquete recibido sobrepase el tamaño del buffer
  if (size >= sizeof(rxpacket)) {
    size = sizeof(rxpacket) - 1;
  }

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  String json = String(rxpacket);
  json.trim();

  // Separamos los valores para poder mostrarlos en la OLED
  tvocStr = extraerValor(json, "tvoc");
  eco2Str = extraerValor(json, "eco2");
  micsStr = extraerValor(json, "mics");
  tempStr = extraerValor(json, "temp");
  humStr  = extraerValor(json, "hum");

  // Si algún dato no llegó bien, se muestra un valor de espera
  if (tvocStr == "") tvocStr = "---";
  if (eco2Str == "") eco2Str = "---";
  if (micsStr == "") micsStr = "---";
  if (tempStr == "") tempStr = "--.-";
  if (humStr  == "") humStr  = "--.-";

  Radio.Sleep();
  lora_idle = true;
}

// Se ejecuta si no se recibe nada en el tiempo esperado
void OnRxTimeout(void) {
  Serial.println("LoRa RX Timeout");
  Radio.Sleep();
  lora_idle = true;
}

// Se ejecuta si hubo error al recibir
void OnRxError(void) {
  Serial.println("LoRa RX Error");
  Radio.Sleep();
  lora_idle = true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("========================================");
  Serial.println("AIR-WATCH NODE B INITIALIZED");
  Serial.println("Nodo receptor LoRa / puente UART");
  Serial.println("========================================");

  // UART hacia LilyGO.
  // Solo se usa TX porque aquí principalmente enviamos datos a la LilyGO.
  SerialLily.begin(115200, SERIAL_8N1, -1, TX_TO_LILYGO);
  Serial.println("UART hacia gateway lista");

  // Encendemos alimentación externa de la OLED en la Heltec
  pinMode(VEXT_PIN, OUTPUT);
  digitalWrite(VEXT_PIN, LOW);
  delay(200);

  // Inicializamos la pantalla OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  delay(200);

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 15, "Nodo B iniciando...");
  u8g2.sendBuffer();

  // Inicializamos LoRa
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

  Serial.println("Receptor listo, esperando paquetes LoRa...");

  u8g2.clearBuffer();
  u8g2.drawStr(0, 15, "Nodo B listo");
  u8g2.drawStr(0, 30, "Esperando datos...");
  u8g2.sendBuffer();

  // Se deja el radio escuchando de forma continua
  Radio.Rx(0);
}

void loop() {
  // Procesa eventos internos del radio LoRa
  Radio.IrqProcess();

  // Cuando llega un paquete, OnRxDone cambia lora_idle a true
  if (lora_idle) {
    lora_idle = false;

    String json = String(rxpacket);
    json.trim();

    Serial.println("--------------------------------------------------");
    Serial.println("Paquete recibido:");
    Serial.println(json);
    Serial.print("RSSI: ");
    Serial.println(rssi);

    // Actualizamos la OLED con los últimos datos recibidos
    mostrarOLED();

    // Antes de mandar a LilyGO se valida que el JSON venga completo
    if (!jsonBasicoValido(json)) {
      Serial.println("JSON invalido o incompleto. No se envia al gateway.");
    } 
    else if (millis() - ultimoEnvioLily >= intervaloEnvioLily) {
      // Enviamos el JSON por UART a la LilyGO
      SerialLily.println(json);
      ultimoEnvioLily = millis();

      Serial.println("Enviado al gateway:");
      Serial.println(json);
    } 
    else {
      // Evita mandar paquetes demasiado rápido y saturar la etapa celular
      Serial.println("Paquete omitido para controlar la frecuencia de envio.");
    }

    Serial.println("--------------------------------------------------");
    Serial.println();

    // Volvemos a escuchar paquetes LoRa
    Radio.Rx(0);
  }
}