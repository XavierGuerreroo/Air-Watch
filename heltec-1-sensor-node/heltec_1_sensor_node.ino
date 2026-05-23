#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

// ===================== LoRa =====================
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
bool lora_idle = true;

// ===================== Sensores =====================
#define SDA_PIN   19
#define SCL_PIN   20
#define MICS_PIN  3

Adafruit_SGP30 sgp;
Adafruit_BME680 bme;   // I2C

// ===================== Tiempo =====================
unsigned long lastRead = 0;
const unsigned long intervalMs = 3000;

// ===================== Buffers =====================
char txpacket[256];
String payload = "";

// ===================== LoRa callbacks =====================
void OnTxDone(void) {
  Serial.println("LoRa TX done");
  lora_idle = true;
}

void OnTxTimeout(void) {
  Serial.println("LoRa TX Timeout");
  Radio.Sleep();
  lora_idle = true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("========================================");
  Serial.println("HELTEC 1 SENDER -> SGP azul + MiCS + BME680");
  Serial.println("Envio por LoRa");
  Serial.println("========================================");

  // -------- I2C --------
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // -------- SGP azul --------
  if (!sgp.begin()) {
    Serial.println("ERROR: SGP azul no detectado");
    while (true) delay(1000);
  }
  Serial.println("SGP azul OK");

  // -------- BME680 --------
  bool bmeOk = false;

  if (bme.begin(0x77, &Wire)) {
    Serial.println("BME680 OK en 0x77");
    bmeOk = true;
  } else if (bme.begin(0x76, &Wire)) {
    Serial.println("BME680 OK en 0x76");
    bmeOk = true;
  }

  if (!bmeOk) {
    Serial.println("ERROR: BME680 no detectado");
    while (true) delay(1000);
  }

  // Ajustes del BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);

  // -------- ADC MiCS --------
  analogReadResolution(12);
  Serial.println("MiCS-5524 ADC OK");

  // -------- LoRa --------
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Serial.println("Sistema listo");
  Serial.println();
}

void loop() {
  Radio.IrqProcess();

  if (!lora_idle) return;
  if (millis() - lastRead < intervalMs) return;
  lastRead = millis();

  // -------- MiCS --------
  int micsRaw = analogRead(MICS_PIN);

  // -------- SGP azul --------
  bool sgpOk = sgp.IAQmeasure();
  if (!sgpOk) {
    Serial.println("ERROR leyendo SGP azul");
    return;
  }

  // -------- BME680 --------
  if (!bme.performReading()) {
    Serial.println("ERROR leyendo BME680");
    return;
  }

  float temp  = bme.temperature;
  float hum   = bme.humidity;
  float press = bme.pressure / 100.0;
  float gas   = bme.gas_resistance / 1000.0;

  // -------- JSON --------
  // Mantenemos la misma estructura base para no romper el receptor
  payload = "{";
  payload += "\"tvoc\":" + String(sgp.TVOC) + ",";
  payload += "\"eco2\":" + String(sgp.eCO2) + ",";
  payload += "\"mics\":" + String(micsRaw) + ",";
  payload += "\"temp\":" + String(temp, 1) + ",";
  payload += "\"hum\":" + String(hum, 1);
  payload += "}";

  payload.toCharArray(txpacket, sizeof(txpacket));

  // -------- Mostrar --------
  Serial.println("--------------------------------------------------");
  Serial.print("SGP azul TVOC: ");
  Serial.print(sgp.TVOC);
  Serial.print(" ppb | eCO2: ");
  Serial.println(sgp.eCO2);

  Serial.print("MiCS-5524 RAW: ");
  Serial.println(micsRaw);

  Serial.print("BME680 Temp: ");
  Serial.print(temp, 1);
  Serial.print(" C | Hum: ");
  Serial.print(hum, 1);
  Serial.println(" %");

  Serial.print("BME680 Press: ");
  Serial.print(press, 1);
  Serial.print(" hPa | Gas: ");
  Serial.print(gas, 1);
  Serial.println(" KOhms");

  Serial.print("Enviando por LoRa: ");
  Serial.println(txpacket);
  Serial.println("--------------------------------------------------");
  Serial.println();

  // -------- Enviar LoRa --------
  Radio.Send((uint8_t *)txpacket, strlen(txpacket));
  lora_idle = false;
}