# Air-Watch

# Sistema IoT de Monitoreo de Calidad del Aire usando LoRa, MQTT, Node-RED, InfluxDB y Grafana

Air-Watch es un sistema IoT distribuido para el monitoreo remoto de variables asociadas a la calidad del aire y presencia de compuestos contaminantes en zonas cercanas a cuerpos de agua.

El sistema fue diseñado como una solución de monitoreo ambiental en tiempo real, orientada a comunidades y organismos que requieren observar tendencias de contaminación atmosférica en áreas con posible presencia de compuestos orgánicos volátiles, gases combustibles y condiciones ambientales críticas.

La arquitectura integra sensores, comunicación LoRa, transmisión celular mediante red móvil y visualización en la nube mediante dashboards históricos y en tiempo real.

---

# Arquitectura General del Sistema

```text
Heltec 1 (Sensores)
↓
LoRa 915 MHz
↓
Heltec 2 (Receptor LoRa)
↓
UART
↓
LilyGO T-SIM7000G (Red móvil LTE)
↓
MQTT Broker
↓
Node-RED Cloud
↓
InfluxDB Cloud
↓
Grafana Cloud
```

---

# Variables Monitoreadas

## SGP41 – Calidad del Aire

### TVOC (Total Volatile Organic Compounds)
**Sensor:** SGP41  
**Unidad:** ppb (partes por billón)

Mide la presencia relativa de:

- vapores orgánicos
- solventes
- compuestos químicos volátiles
- emisiones derivadas de combustibles
- contaminación ambiental asociada a VOCs

**Uso en el proyecto:**  
Permite detectar cambios en la calidad del aire cercanos al río o canal, asociados a emisiones contaminantes.

---

### eCO2 (Equivalent Carbon Dioxide)
**Sensor:** SGP41  
**Unidad:** ppm (partes por millón)

No mide CO2 real directamente, sino un:

**indicador equivalente de calidad del aire**

Estimado en función de:

- compuestos orgánicos presentes
- concentración relativa de contaminantes
- comportamiento del aire ambiental

**Uso en el proyecto:**  
Permite observar deterioro de la calidad del aire en tiempo real.

---

## MiCS-5524 – Indicador de Gases Combustibles

### Gases combustibles / vapores asociados
**Sensor:** MiCS-5524  
**Unidad:** RAW (lectura relativa)

Detecta cambios relativos asociados a:

- vapores de gasolina
- hidrocarburos ligeros
- gases combustibles
- compuestos orgánicos volátiles
- humo o presencia química anormal


**Uso en el proyecto:**  
Permite identificar cambios asociados a emisiones combustibles o contaminantes cercanos al cuerpo de agua.

---

## BME680 – Condiciones Ambientales

### Temperatura
**Sensor:** BME680  
**Unidad:** °C

Mide la temperatura ambiental del entorno.

**Uso en el proyecto:**  
Ayuda a contextualizar el comportamiento de los demás sensores, ya que la temperatura puede influir en dispersión de gases.

---

### Humedad Relativa
**Sensor:** BME680  
**Unidad:** %

Mide la humedad del aire.

**Uso en el proyecto:**  
Permite complementar el análisis ambiental y mejorar interpretación de condiciones atmosféricas.

---

# Hardware Utilizado

## Nodo Sensor (Heltec 1)
- Heltec WiFi LoRa 32 V3 (ESP32-S3)
- Sensor SGP41 (TVOC + eCO2)
- Sensor MiCS-5524
- Sensor BME680
- Protoboard y cableado

## Nodo Receptor (Heltec 2)
- Heltec WiFi LoRa 32 V3
- Comunicación LoRa 915 MHz
- Comunicación UART hacia gateway

## Gateway Celular
- LilyGO T-SIM7000G
- SIM Card Telcel
- Antena LTE
- UART desde Heltec receptora

## Alimentación
- Power Bank 10,000 mAh
- Cables USB independientes para cada nodo

---

# Requerimientos de Software

## Arduino IDE
Versión 2.0 o superior

## Gestor de Tarjetas
Instalar:

### ESP32 by Espressif Systems

Compatible con:
- Heltec WiFi LoRa 32 V3
- LilyGO T-SIM7000G

---

# Librerías Necesarias

## Para Heltec 1 y Heltec 2
- LoRaWan_APP
- Arduino
- Wire
- Sensirion I2C SGP41
- Adafruit Sensor
- Adafruit BME680

## Para LilyGO
- TinyGSM
- PubSubClient
- ArduinoJson
- LittleFS

---

# Configuración de Red Celular

Código preconfigurado para Telcel México:

APN:

```text
internet.itelcel.com
```

Usuario:

```text
(vacío)
```

Password:

```text
(vacío)
```

---

# Configuración MQTT

Topic utilizado:

```text
itics/heltec/datos
```


---

# Servicios Cloud Utilizados

## Node-RED Cloud
Recepción y procesamiento de mensajes MQTT en tiempo real.

## InfluxDB Cloud
Almacenamiento histórico de datos de sensores.

Measurement:

```text
calidad_aire
```
```

## Grafana Cloud
Visualización profesional mediante dashboards históricos y tiempo real.

---

# Mecanismos de Robustez Implementados

## Reconexión automática
Si se pierde señal celular o MQTT:

- reconexión automática
- múltiples reintentos
- reinicio del módem

## Respaldo temporal en memoria flash
Si el envío falla:

- datos se almacenan temporalmente
- reintento posterior

## Validación de datos
Antes de publicar:

- verificación de JSON
- verificación de campos numéricos
- descarte de datos corruptos

---

# Configuración Heltec LoRa

Frecuencia:

```text
915000000 Hz
```

Parámetros:

- BW: 125 kHz
- SF: 7
- CR: 4/5
- TX Power: 14 dBm

---

# Instalación y Uso

1. Clonar repositorio
2. Abrir los archivos `.ino`
3. Configurar credenciales MQTT si aplica
4. Cargar código a cada tarjeta
5. Energizar el sistema
6. Verificar Monitor Serie
7. Observar datos en Node-RED
8. Consultar históricos en InfluxDB / Grafana

---

# Archivos del Repositorio

```text
README.md
Heltec_1_Transmisor.ino
Heltec_2_Receptor.ino
LilyGO_SIM7000_MQTT.ino
```

---

# Aplicación del Proyecto

Este sistema fue diseñado como prototipo de monitoreo ambiental para:

- comunidades cercanas a ríos o canales
- organismos de monitoreo ambiental
- protección civil
- análisis histórico de tendencias de contaminación
- generación de alertas tempranas

---

# Autores

Proyecto Integrador  
Ingeniería en Tecnologías de la Información y Comunicaciones  
ITSOEH