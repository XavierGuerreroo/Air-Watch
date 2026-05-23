#define TINY_GSM_MODEM_SIM7000

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <LittleFS.h>

#define UART_BAUD      115200
#define PIN_TX         27
#define PIN_RX         26
#define PWR_PIN        4
#define LED_INDICATOR  12

// UART desde Heltec 2
#define HELTEC_RX      13
#define HELTEC_TX      14

// Archivo de respaldo en memoria flash
#define ARCHIVO_CACHE "/cache_tx.log"

HardwareSerial SerialAT(1);
HardwareSerial SerialHeltec(2);

const char apn[]      = "internet.itelcel.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

const char* broker = "broker.hivemq.com";
const int   port   = 1883;
const char* topic  = "itics/heltec/datos";

TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem);
PubSubClient mqtt(gsmClient);

// ===== Control =====
unsigned long ultimoEnvioOK = 0;
unsigned long ultimoIntentoRecuperacion = 0;
unsigned long ultimoLogEstado = 0;
unsigned long ultimoEnvioMQTT = 0;

int fallosConsecutivos = 0;

const unsigned long intervaloRecuperacion = 15000UL;
const unsigned long timeoutSinEnvio       = 120000UL;
const unsigned long intervaloLogEstado    = 30000UL;
const unsigned long intervaloEnvioMQTT    = 2000UL; // 2 segundos

const int maxFallosAntesReinicio = 4;

// ======================================================
// VALIDACIÓN JSON
// ======================================================

bool jsonBasicoValido(String data) {
  data.trim();

  if (!data.startsWith("{") || !data.endsWith("}")) {
    return false;
  }

  if (data.indexOf("\"tvoc\"") == -1) return false;
  if (data.indexOf("\"eco2\"") == -1) return false;
  if (data.indexOf("\"mics\"") == -1) return false;
  if (data.indexOf("\"temp\"") == -1) return false;
  if (data.indexOf("\"hum\"") == -1) return false;

  return true;
}

// ======================================================
// PERSISTENCIA LOCAL CON LittleFS
// ======================================================

void guardarEnCache(String data) {
  File file = LittleFS.open(ARCHIVO_CACHE, FILE_APPEND);

  if (!file) {
    Serial.println("ERROR: no se pudo abrir cache local");
    return;
  }

  file.println(data);
  file.close();

  Serial.println("CACHE: dato guardado en memoria interna");
}

// Se conserva la función, pero NO se llama automáticamente para evitar ráfagas.
void reenviarCache() {
  if (!mqtt.connected()) {
    return;
  }

  File file = LittleFS.open(ARCHIVO_CACHE, FILE_READ);

  if (!file) {
    return;
  }

  String restante = "";

  while (file.available()) {
    String linea = file.readStringUntil('\n');
    linea.trim();

    if (linea.length() > 0) {
      if (mqtt.publish(topic, linea.c_str())) {
        Serial.println("REENVIO OK desde cache:");
        Serial.println(linea);
        delay(500); // pequeña pausa si algún día se usa manualmente
      } else {
        Serial.println("ERROR reenviando dato de cache, se conserva");
        restante += linea + "\n";
      }
    }
  }

  file.close();

  File out = LittleFS.open(ARCHIVO_CACHE, FILE_WRITE);
  if (out) {
    out.print(restante);
    out.close();
  }
}

// ======================================================

void encenderModem() {
  Serial.println("Encendiendo modem...");
  pinMode(PWR_PIN, OUTPUT);

  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(1200);
  digitalWrite(PWR_PIN, LOW);

  delay(6000);
}

void apagarIndicador() {
  digitalWrite(LED_INDICATOR, LOW);
}

void encenderIndicador() {
  digitalWrite(LED_INDICATOR, HIGH);
}

bool inicializarModem() {
  Serial.println("Inicializando modem...");

  while (SerialAT.available()) {
    SerialAT.read();
  }

  if (!modem.init()) {
    Serial.println("ERROR: modem no responde con init()");
    return false;
  }

  Serial.print("Modem: ");
  Serial.println(modem.getModemInfo());

  Serial.print("SIM status: ");
  Serial.println(modem.getSimStatus());

  return true;
}

bool esperarRed() {
  Serial.println("Esperando red...");

  if (!modem.waitForNetwork(120000L)) {
    Serial.println("ERROR: no registro en red");
    return false;
  }

  Serial.println("Red registrada");
  Serial.print("CSQ: ");
  Serial.println(modem.getSignalQuality());

  return true;
}

bool abrirDatos() {
  Serial.println("Abriendo datos moviles...");

  modem.gprsDisconnect();
  delay(1500);

  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println("ERROR: no se pudo abrir datos");
    return false;
  }

  delay(2000);

  if (!modem.isGprsConnected()) {
    Serial.println("ERROR: GPRS no quedo realmente conectado");
    return false;
  }

  Serial.println("Datos moviles conectados");
  Serial.print("IP local: ");
  Serial.println(modem.localIP());

  return true;
}

bool conectarRedMovilCompleta() {
  if (!esperarRed()) return false;
  if (!abrirDatos()) return false;
  return true;
}

bool conectarMQTT() {
  if (mqtt.connected()) {
    return true;
  }

  if (!modem.isNetworkConnected()) {
    Serial.println("No hay red celular registrada");
    return false;
  }

  if (!modem.isGprsConnected()) {
    Serial.println("No hay datos moviles activos");
    return false;
  }

  Serial.println("Conectando a broker MQTT...");
  String clienteID = "Xavier-LilyGO-" + String(millis()) + "-" + String(random(1000, 9999));

  bool ok = mqtt.connect(clienteID.c_str());

  if (ok) {
    Serial.println("MQTT conectado");
    encenderIndicador();

    // Para demo se desactiva para evitar ráfagas al dashboard
    // reenviarCache();

    return true;
  } else {
    Serial.print("ERROR MQTT, state: ");
    Serial.println(mqtt.state());
    apagarIndicador();
    return false;
  }
}

void cerrarSesiones() {
  Serial.println("Cerrando sesiones MQTT/GPRS...");
  mqtt.disconnect();
  delay(500);
  modem.gprsDisconnect();
  delay(1500);
}

bool recuperacionLigera() {
  Serial.println("=== RECUPERACION LIGERA ===");

  bool redOK   = modem.isNetworkConnected();
  bool datosOK = modem.isGprsConnected();

  Serial.print("Red OK: ");
  Serial.println(redOK ? "SI" : "NO");

  Serial.print("Datos OK: ");
  Serial.println(datosOK ? "SI" : "NO");

  if (!redOK || !datosOK) {
    Serial.println("Reconectando red/datos...");
    cerrarSesiones();

    if (!conectarRedMovilCompleta()) {
      Serial.println("ERROR: fallo al restaurar red/datos");
      return false;
    }
  }

  if (!conectarMQTT()) {
    Serial.println("ERROR: fallo al reconectar MQTT");
    return false;
  }

  Serial.println("=== RECUPERACION LIGERA EXITOSA ===");
  fallosConsecutivos = 0;
  return true;
}

bool recuperacionCompleta() {
  Serial.println("=== INICIANDO RECUPERACION COMPLETA ===");

  cerrarSesiones();

  Serial.println("Reiniciando modem fisicamente...");
  encenderModem();

  if (!inicializarModem()) {
    Serial.println("ERROR: init del modem fallo");
    return false;
  }

  if (!conectarRedMovilCompleta()) {
    Serial.println("ERROR: red/datos no se pudieron restaurar");
    return false;
  }

  if (!conectarMQTT()) {
    Serial.println("ERROR: MQTT no se pudo restaurar");
    return false;
  }

  Serial.println("=== RECUPERACION COMPLETA EXITOSA ===");
  fallosConsecutivos = 0;
  ultimoEnvioOK = millis();
  return true;
}

bool asegurarConexionTotal() {
  bool redOK   = modem.isNetworkConnected();
  bool datosOK = modem.isGprsConnected();
  bool mqttOK  = mqtt.connected();

  if (redOK && datosOK && mqttOK) {
    return true;
  }

  Serial.println("Conexion incompleta detectada:");
  Serial.print("  Red: ");
  Serial.println(redOK ? "SI" : "NO");
  Serial.print("  Datos: ");
  Serial.println(datosOK ? "SI" : "NO");
  Serial.print("  MQTT: ");
  Serial.println(mqttOK ? "SI" : "NO");

  if (recuperacionLigera()) {
    return true;
  }

  Serial.println("La recuperacion ligera fallo, probando recuperacion completa...");
  return recuperacionCompleta();
}

bool publicarJSON(const String& data) {
  if (!asegurarConexionTotal()) {
    Serial.println("No se pudo asegurar conexion total antes de publicar");
    Serial.println("Guardando dato en cache local");

    guardarEnCache(data);

    fallosConsecutivos++;
    return false;
  }

  Serial.println("Publicando en MQTT...");

  if (mqtt.publish(topic, data.c_str())) {
    Serial.println("Enviado a MQTT OK");
    encenderIndicador();

    ultimoEnvioOK = millis();
    fallosConsecutivos = 0;

    // Para demo se desactiva para evitar ráfagas al dashboard
    // reenviarCache();

    return true;
  } else {
    Serial.println("ERROR al publicar mensaje");
    Serial.println("Guardando dato en cache local");

    apagarIndicador();
    guardarEnCache(data);

    fallosConsecutivos++;
    return false;
  }
}

void intentarRecuperacionPeriodica() {
  if (millis() - ultimoIntentoRecuperacion < intervaloRecuperacion) {
    return;
  }

  ultimoIntentoRecuperacion = millis();

  bool redOK   = modem.isNetworkConnected();
  bool datosOK = modem.isGprsConnected();
  bool mqttOK  = mqtt.connected();

  if (redOK && datosOK && mqttOK) {
    return;
  }

  Serial.println("Se detecto fallo de conectividad. Intentando recuperar...");
  fallosConsecutivos++;

  if (!recuperacionLigera()) {
    Serial.print("Fallos consecutivos: ");
    Serial.println(fallosConsecutivos);
  }

  if (fallosConsecutivos >= maxFallosAntesReinicio) {
    Serial.println("Demasiados fallos. Forzando recuperacion completa...");
    recuperacionCompleta();
  }
}

void imprimirEstadoPeriodico() {
  if (millis() - ultimoLogEstado < intervaloLogEstado) {
    return;
  }

  ultimoLogEstado = millis();

  Serial.println("----- ESTADO ACTUAL -----");
  Serial.print("Red: ");
  Serial.println(modem.isNetworkConnected() ? "SI" : "NO");
  Serial.print("Datos: ");
  Serial.println(modem.isGprsConnected() ? "SI" : "NO");
  Serial.print("MQTT: ");
  Serial.println(mqtt.connected() ? "SI" : "NO");
  Serial.print("CSQ: ");
  Serial.println(modem.getSignalQuality());
  Serial.print("Fallos consecutivos: ");
  Serial.println(fallosConsecutivos);
  Serial.println("-------------------------");
}

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println("========================================");
  Serial.println("LILYGO SIM7000 -> MQTT + UART CONTROLADO");
  Serial.println("Recibe JSON desde Heltec 2");
  Serial.println("Publica maximo cada 2 segundos");
  Serial.println("Cache activo, reenvio automatico desactivado");
  Serial.println("========================================");

  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: LittleFS no se pudo iniciar");
  } else {
    Serial.println("LittleFS listo");
  }

  randomSeed(millis());

  pinMode(LED_INDICATOR, OUTPUT);
  apagarIndicador();

  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(500);

  SerialHeltec.begin(115200, SERIAL_8N1, HELTEC_RX, HELTEC_TX);
  Serial.println("UART desde Heltec 2 lista");

  encenderModem();

  while (true) {
    if (inicializarModem() && conectarRedMovilCompleta()) {
      break;
    }

    Serial.println("Fallo de arranque. Reintentando inicializacion completa en 8 segundos...");
    delay(8000);
    encenderModem();
  }

  mqtt.setServer(broker, port);
  mqtt.setKeepAlive(45);
  mqtt.setSocketTimeout(20);

  while (!conectarMQTT()) {
    Serial.println("MQTT no conecto en setup. Reintentando en 5 segundos...");
    delay(5000);

    if (!modem.isNetworkConnected() || !modem.isGprsConnected()) {
      cerrarSesiones();
      conectarRedMovilCompleta();
    }
  }

  ultimoEnvioOK = millis();
  ultimoIntentoRecuperacion = millis();
  ultimoLogEstado = millis();
  ultimoEnvioMQTT = 0;

  Serial.println("Esperando JSON desde Heltec 2...");
}

void loop() {
  mqtt.loop();

  intentarRecuperacionPeriodica();
  imprimirEstadoPeriodico();

  if (SerialHeltec.available()) {
    String data = SerialHeltec.readStringUntil('\n');
    data.trim();

    if (data.length() > 0) {
      Serial.println("Recibido desde Heltec 2:");
      Serial.println(data);

      // Validar JSON antes de publicar
      if (!jsonBasicoValido(data)) {
        Serial.println("JSON incompleto o sin campos completos. No se publica.");
        Serial.println("----------------------------------------");
        return;
      }

      // Limitar frecuencia de publicación MQTT
      if (millis() - ultimoEnvioMQTT < intervaloEnvioMQTT) {
        Serial.println("Dato omitido para no saturar MQTT/Node-RED.");
        Serial.println("----------------------------------------");
        return;
      }

      ultimoEnvioMQTT = millis();

      Serial.println("Publicando dato controlado a MQTT...");
      publicarJSON(data);

      Serial.println("----------------------------------------");
    }
  }

  if (millis() - ultimoEnvioOK > timeoutSinEnvio) {
    Serial.println("Mucho tiempo sin envio exitoso. Forzando recuperacion completa...");
    recuperacionCompleta();
    ultimoEnvioOK = millis();
  }
}