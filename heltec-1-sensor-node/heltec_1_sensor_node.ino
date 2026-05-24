/*
  AIR-WATCH - Heltec 1 / Nodo de sensores

  Este código lee los sensores conectados a la primera Heltec,
  arma un JSON con los datos y lo manda por LoRa hacia la Heltec 2.

  Sensores:
  - SGP azul: TVOC y eCO2
  - BME680: temperatura, humedad, presión y gas interno
  - MiCS-5524: lectura analógica RAW para gases combustibles/hidrocarburos

  Nota:
  Esta placa solo envía por LoRa. El envío a MQTT lo hace después la LilyGO.
*/

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <Wire.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

// Configuración LoRa. Estos valores deben coincidir con la Heltec receptora.
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

// Pines usados por los sensores
#define SDA_PIN   19
#define SCL_PIN   20
#define MICS_PIN  3

// Objetos de los sensores
// Aunque el sensor usado es el SGP azul, esta librería fue la que funcionó en el montaje.
Adafruit_SGP30 sgp;
Adafruit_BME680 bme;

// Tiempo entre cada lectura y envío
unsigned long lastRead = 0;
const unsigned long intervalMs = 3000;

// Buffer para enviar el JSON por LoRa
char txpacket[256];
String payload = "";

// Se ejecuta cuando LoRa termina de enviar
void OnTxDone(void) {
  Serial.println("TX completada");
  lora_idle = true;
}

// Se ejecuta si LoRa tarda demasiado en enviar
void OnTxTimeout(void) {
  Serial.println("TX timeout");
  Radio.Sleep();
  lora_idle = true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("========================================");
  Serial.println("AIR-WATCH NODE A INITIALIZED");
  Serial.println("Nodo de sensores ambientales");
  Serial.println("========================================");

  // Iniciamos el bus I2C para SGP y BME680
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  // Inicializamos el sensor SGP
  if (!sgp.begin()) {
    Serial.println("ERROR: sensor SGP no detectado");
    while (true) delay(1000);
  }
  Serial.println("Sensor SGP OK");

  // Inicializamos el BME680.
  // Algunos módulos usan dirección 0x77 y otros 0x76, por eso se prueban ambas.
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

  // Ajustes del BME680 para obtener lecturas más estables
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);

  // El MiCS-5524 se lee como valor analógico RAW
  analogReadResolution(12);
  Serial.println("MiCS-5524 ADC OK");

  // Inicializamos la placa y el radio LoRa
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Serial.println("Sistema listo para leer sensores y enviar datos");
  Serial.println();
}

void loop() {
  // Procesa eventos internos del radio LoRa
  Radio.IrqProcess();

  // Evita mandar otro paquete mientras LoRa sigue ocupado
  if (!lora_idle) return;

  // Lee y envía cada 3 segundos
  if (millis() - lastRead < intervalMs) return;
  lastRead = millis();

  // Lectura RAW del MiCS-5524
  int micsRaw = analogRead(MICS_PIN);

  // Lectura del SGP: TVOC y eCO2
  bool sgpOk = sgp.IAQmeasure();
  if (!sgpOk) {
    Serial.println("ERROR leyendo sensor SGP");
    return;
  }

  // Lectura del BME680
  if (!bme.performReading()) {
    Serial.println("ERROR leyendo BME680");
    return;
  }

  float temp  = bme.temperature;
  float hum   = bme.humidity;
  float press = bme.pressure / 100.0;
  float gas   = bme.gas_resistance / 1000.0;

  // Armamos el JSON con las lecturas principales.
  // Se deja esta estructura porque es la que esperan Heltec 2, LilyGO y Node-RED.
  payload = "{";
  payload += "\"tvoc\":" + String(sgp.TVOC) + ",";
  payload += "\"eco2\":" + String(sgp.eCO2) + ",";
  payload += "\"mics\":" + String(micsRaw) + ",";
  payload += "\"temp\":" + String(temp, 1) + ",";
  payload += "\"hum\":" + String(hum, 1);
  payload += "}";

  payload.toCharArray(txpacket, sizeof(txpacket));

  // Datos para verificar en el Monitor Serie
  Serial.println("--------------------------------------------------");

  Serial.print("TVOC: ");
  Serial.print(sgp.TVOC);
  Serial.print(" ppb | eCO2: ");
  Serial.print(sgp.eCO2);
  Serial.println(" ppm");

  Serial.print("MiCS-5524 RAW: ");
  Serial.println(micsRaw);

  Serial.print("Temp: ");
  Serial.print(temp, 1);
  Serial.print(" C | Hum: ");
  Serial.print(hum, 1);
  Serial.println(" %");

  Serial.print("Presion: ");
  Serial.print(press, 1);
  Serial.print(" hPa | Gas BME680: ");
  Serial.print(gas, 1);
  Serial.println(" KOhms");

  Serial.print("Paquete LoRa: ");
  Serial.println(txpacket);

  Serial.println("--------------------------------------------------");
  Serial.println();

  // Enviamos el JSON por LoRa hacia la Heltec 2
  Radio.Send((uint8_t *)txpacket, strlen(txpacket));
  lora_idle = false;
}