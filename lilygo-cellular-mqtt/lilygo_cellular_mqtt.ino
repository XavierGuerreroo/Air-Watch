/*
  AIR-WATCH - LilyGO / Gateway celular MQTT

  Esta tarjeta recibe por UART el JSON que manda la Heltec 2,
  se conecta a la red celular con la SIM y publica los datos por MQTT.

  Flujo:
  Heltec 2 -> UART -> LilyGO -> Red celular -> MQTT

  Nota de seguridad:
  El topic real del equipo no se deja público en el repositorio.
  Aquí se usa un topic de ejemplo para documentación.
*/

#define TINY_GSM_MODEM_SIM7000

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <LittleFS.h>

// Pines del modem SIM7000
#define UART_BAUD      115200
#define PIN_TX         27
#define PIN_RX         26
#define PWR_PIN        4
#define LED_INDICATOR  12

// UART que recibe datos desde Heltec 2
#define HELTEC_RX      13
#define HELTEC_TX      14

// Archivo local para guardar datos si falla la conexión
#define ARCHIVO_CACHE "/cache_tx.log"

HardwareSerial SerialAT(1);
HardwareSerial SerialHeltec(2);

// Configuración celular Telcel
const char apn[]      = "internet.itelcel.com";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Configuración MQTT
const char* broker = "broker.hivemq.com";
const int   port   = 1883;

// Topic de ejemplo. Cambiar por el topic real antes de usar en pruebas privadas.
const char* topic  = "airwatch/demo/datos";

TinyGsm modem(SerialAT);
TinyGsmClient gsmClient(modem);
PubSubClient mqtt(gsmClient);

// Variables para control de conexión y tiempos
unsigned long ultimoEnvioOK = 0;
unsigned long ultimoIntentoRecuperacion = 0;
unsigned long ultimoLogEstado = 0;
unsigned long ultimoEnvioMQTT = 0;

int fallosConsecutivos = 0;

const unsigned long intervaloRecuperacion = 15000UL;
const unsigned long timeoutSinEnvio       = 120000UL;
const unsigned long intervaloLogEstado    = 30000UL;
const unsigned long intervaloEnvioMQTT    = 2000UL;

const int maxFallosAntesReinicio = 4;

// Valida que el JSON tenga la estructura mínima esperada
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

// Guarda un dato en memoria local si no se pudo publicar
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

// Función disponible para reenviar cache si se requiere.
// No se llama automáticamente para evitar ráfagas en el dashboard.
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
        delay(500);
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

// Enciende físicamente el modem SIM7000 con el pin PWR
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

// Inicializa el modem y muestra información básica
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

// Espera registro en red celular
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

// Abre conexión de datos móviles usando APN
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

// Conexión al broker MQTT
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

  // ID genérico para no dejar datos personales en el repositorio
  String clienteID = "AIRWATCH-GW-" + String(millis()) + "-" + String(random(1000, 9999));

  bool ok = mqtt.connect(clienteID.c_str());

  if (ok) {
    Serial.println("MQTT conectado");
    encenderIndicador();

    // Se deja apagado para evitar ráfagas en demo
    // reenviarCache();

    return true;
  } else {
    Serial.print("ERROR MQTT, state: ");
    Serial.println(mqtt.state());
    apagarIndicador();
    return false;
  }
}

// Cierra MQTT y datos para intentar reconectar limpio
void cerrarSesiones() {
  Serial.println("Cerrando sesiones MQTT/GPRS...");
  mqtt.disconnect();
  delay(500);
  modem.gprsDisconnect();
  delay(1500);
}

// Recuperación rápida sin reiniciar físicamente el modem
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

// Recuperación más fuerte: cierra todo, reinicia modem y reconecta
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

// Revisa red, datos y MQTT antes de publicar
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

// Publica el JSON en MQTT o lo guarda en cache si falla
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

    // Se deja desactivado para evitar ráfagas durante pruebas
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

// Intenta recuperar conexión de forma periódica si algo cae
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

// Muestra estado cada cierto tiempo para diagnóstico en Monitor Serie
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
  Serial.println("AIR-WATCH GATEWAY INITIALIZED");
  Serial.println("UART -> Cellular MQTT");
  Serial.println("Cache local activo");
  Serial.println("========================================");

  // Inicia memoria interna para guardar datos cuando falla el envío
  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: LittleFS no se pudo iniciar");
  } else {
    Serial.println("LittleFS listo");
  }

  randomSeed(millis());

  pinMode(LED_INDICATOR, OUTPUT);
  apagarIndicador();

  // UART hacia el modem SIM7000
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(500);

  // UART que recibe el JSON desde Heltec 2
  SerialHeltec.begin(115200, SERIAL_8N1, HELTEC_RX, HELTEC_TX);
  Serial.println("UART desde nodo receptor lista");

  encenderModem();

  // Intento inicial hasta que el modem y la red queden listos
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

  // Conecta a MQTT antes de comenzar a publicar
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

  Serial.println("Esperando JSON desde nodo receptor...");
}

void loop() {
  mqtt.loop();

  intentarRecuperacionPeriodica();
  imprimirEstadoPeriodico();

  // Si llega información desde Heltec 2, se procesa
  if (SerialHeltec.available()) {
    String data = SerialHeltec.readStringUntil('\n');
    data.trim();

    if (data.length() > 0) {
      Serial.println("Recibido desde nodo receptor:");
      Serial.println(data);

      // Evita publicar datos incompletos o basura
      if (!jsonBasicoValido(data)) {
        Serial.println("JSON incompleto o sin campos completos. No se publica.");
        Serial.println("----------------------------------------");
        return;
      }

      // Limita la frecuencia de publicación para no saturar MQTT/Node-RED
      if (millis() - ultimoEnvioMQTT < intervaloEnvioMQTT) {
        Serial.println("Dato omitido para controlar frecuencia MQTT.");
        Serial.println("----------------------------------------");
        return;
      }

      ultimoEnvioMQTT = millis();

      Serial.println("Publicando dato controlado a MQTT...");
      publicarJSON(data);

      Serial.println("----------------------------------------");
    }
  }

  // Si pasa mucho tiempo sin publicar correctamente, se fuerza recuperación
  if (millis() - ultimoEnvioOK > timeoutSinEnvio) {
    Serial.println("Mucho tiempo sin envio exitoso. Forzando recuperacion completa...");
    recuperacionCompleta();
    ultimoEnvioOK = millis();
  }
}