# AIR-WATCH  
## Sistema IoT de Monitoreo Ambiental vía LoRa, Red Celular y MQTT

AIR-WATCH es un sistema de monitoreo ambiental basado en tecnología IoT diseñado para medir variables relacionadas con la calidad del aire en zonas cercanas a canales de aguas residuales, permitiendo la captura, transmisión, visualización y almacenamiento histórico de datos ambientales en tiempo real.

El sistema utiliza sensores especializados conectados a una estación Heltec transmisora, comunicación LoRa para el envío inalámbrico local, una segunda estación Heltec receptora y una tarjeta LILYGO T-SIM7000G para el envío de información mediante red celular hacia plataformas de monitoreo en la nube usando MQTT.

---

# Introducción

En diversas regiones del estado de Hidalgo, particularmente en el Valle del Mezquital, existe una problemática ambiental asociada a la presencia de canales de aguas residuales a cielo abierto que atraviesan zonas agrícolas, habitacionales y cercanas a centros escolares. Estas aguas contienen materia orgánica en descomposición, residuos industriales y diversos contaminantes que, bajo condiciones ambientales como la temperatura, la radiación solar y procesos fisicoquímicos naturales, pueden favorecer la liberación de gases y compuestos volátiles hacia la atmósfera.

En municipios como Mixquiahuala de Juárez, específicamente en zonas cercanas a la colonia El Bondho, esta situación representa una preocupación ambiental y de salud pública, ya que la población puede estar expuesta constantemente a olores intensos, emisiones gaseosas y condiciones de contaminación atmosférica sin contar con sistemas locales de monitoreo que permitan conocer objetivamente el comportamiento de estas emisiones.

Actualmente, en muchas comunidades la percepción de contaminación se basa únicamente en la experiencia sensorial de los habitantes, especialmente en olores fuertes o cambios visibles en el ambiente, lo cual no proporciona datos cuantificables ni históricos que permitan analizar tendencias o detectar puntos críticos de contaminación.

Ante esta problemática, surge la necesidad de desarrollar herramientas tecnológicas accesibles que permitan monitorear variables ambientales en tiempo real y almacenar información histórica para su posterior análisis.

El presente proyecto propone el desarrollo de un prototipo de monitoreo ambiental basado en tecnología IoT (Internet de las Cosas), diseñado para medir indicadores relacionados con la calidad del aire mediante sensores especializados. El sistema es capaz de capturar variables como compuestos orgánicos volátiles totales (TVOC), CO2 equivalente (eCO2), gases combustibles o hidrocarburos detectados mediante sensor MiCS, así como temperatura y humedad ambiental.

La información recolectada es transmitida mediante comunicación LoRa hacia una estación receptora y posteriormente enviada mediante red celular a un broker MQTT, permitiendo su visualización en tiempo real en plataformas como Node-RED y su almacenamiento histórico en bases de datos para análisis posterior mediante Grafana.

De esta manera, el prototipo no solo permite el monitoreo ambiental en tiempo real, sino también la generación de históricos y tendencias que pueden servir como base para futuros análisis técnicos, científicos o de toma de decisiones en materia ambiental.

---

# Justificación

En la colonia El Bondho, ubicada en el municipio de Mixquiahuala de Juárez, Hidalgo, los canales de aguas residuales representan una problemática ambiental importante debido a la presencia constante de olores intensos, residuos orgánicos, contaminantes industriales y posibles emisiones gaseosas asociadas a procesos de descomposición y volatilización.

Factores como la temperatura ambiental, la exposición solar, la materia orgánica en descomposición y la presencia de residuos contaminantes favorecen la liberación de compuestos volátiles y gases que pueden afectar la calidad del aire en zonas cercanas a áreas habitacionales, agrícolas y escolares.

A pesar de que esta situación es percibida diariamente por los habitantes a través de olores fuertes y molestias ambientales, actualmente no existe una infraestructura local de monitoreo continuo que permita obtener información objetiva, cuantificable e histórica sobre el comportamiento de estas condiciones ambientales.

En muchos casos, la percepción de contaminación se basa únicamente en la experiencia sensorial de la población, lo cual no permite medir tendencias, detectar anomalías o generar evidencia técnica que apoye la toma de decisiones.

Ante esta problemática, surge la necesidad de desarrollar herramientas tecnológicas accesibles que permitan monitorear de manera continua variables relacionadas con la calidad del aire y el entorno ambiental.

El presente proyecto propone el desarrollo de una estación de monitoreo ambiental basada en tecnología IoT, capaz de medir indicadores como compuestos orgánicos volátiles totales (TVOC), CO2 equivalente (eCO2), gases combustibles o hidrocarburos detectados mediante sensor MiCS, así como variables complementarias como temperatura y humedad ambiental.

La importancia de este proyecto radica en que permite transformar una problemática percibida únicamente de forma sensorial en información cuantitativa basada en datos. A través de la captura continua de información y su almacenamiento histórico, es posible identificar patrones de comportamiento, analizar tendencias ambientales y detectar condiciones anómalas en tiempo real.

Desde el enfoque social, el sistema puede beneficiar a comunidades cercanas al proporcionar información objetiva sobre variables relacionadas con la calidad del aire, promoviendo una mayor conciencia ambiental y fortaleciendo la toma de decisiones informadas.

Desde la perspectiva ambiental, el proyecto contribuye al monitoreo de una problemática poco instrumentada a nivel local, generando datos que pueden servir como base para estudios posteriores o estrategias de mitigación.

Desde el punto de vista tecnológico, el sistema demuestra la viabilidad del uso de tecnologías IoT de bajo costo para el monitoreo ambiental en zonas con infraestructura limitada, integrando sensores, comunicación LoRa, red celular, MQTT y plataformas de análisis de datos en la nube.

En el aspecto económico, representa una alternativa accesible frente a sistemas tradicionales de monitoreo ambiental, los cuales suelen ser más costosos y difíciles de implementar en comunidades semiurbanas, permitiendo además su posible escalabilidad y replicabilidad en otras regiones.

Finalmente, el proyecto se alinea con iniciativas de monitoreo ambiental, salud pública y desarrollo sostenible, al promover el acceso a información ambiental objetiva y el uso de tecnología para la generación de evidencia técnica basada en datos.

---

# Objetivos

## Objetivo General

Desarrollar un prototipo de monitoreo ambiental autónomo basado en tecnología IoT, comunicación LoRa y red celular, capaz de medir y transmitir en tiempo real variables relacionadas con la calidad del aire en zonas cercanas a canales de aguas residuales de la colonia El Bondho, ubicada en el municipio de Mixquiahuala de Juárez, Hidalgo, con el propósito de generar información ambiental objetiva, histórica y en tiempo real que permita identificar condiciones de riesgo y analizar tendencias ambientales.

## Objetivos Específicos

1. Analizar los requisitos ambientales y operativos del sistema, identificando las variables críticas a monitorear y diseñando la arquitectura general del nodo IoT mediante la selección de sensores ambientales, módulos de comunicación y componentes electrónicos adecuados.

2. Integrar el hardware del prototipo, ensamblando sensores, tarjetas de procesamiento y módulos de comunicación, garantizando el correcto funcionamiento del sistema para la adquisición de variables ambientales y su transmisión inalámbrica.

3. Configurar el funcionamiento del sistema de monitoreo, implementando rutinas de adquisición de datos, procesamiento local y transmisión inalámbrica mediante tecnología LoRa y red celular hacia plataformas de monitoreo en la nube.

4. Visualizar y almacenar la información recolectada, utilizando protocolos MQTT y herramientas como Node-RED, InfluxDB y Grafana para mostrar datos en tiempo real y generar históricos para su posterior análisis.

5. Generar información ambiental cuantificable, permitiendo identificar patrones de comportamiento, detectar anomalías y contar con evidencia técnica basada en datos sobre condiciones relacionadas con la calidad del aire.

---

# Requerimientos de Hardware

- Heltec WiFi LoRa 32 V3 (Nodo transmisor)
- Heltec WiFi LoRa 32 V3 (Nodo receptor)
- LILYGO T-SIM7000G
- Sensor SGP41
- Sensor BME680
- Sensor MiCS-5524
- Antena LoRa
- Antena LTE
- Nano SIM con datos activos
- Protoboard y cableado
- Fuente de alimentación o batería

---

# Requerimientos de Software

- Arduino IDE
- ESP32 Board Package
- TinyGSM
- PubSubClient
- U8g2
- Librerías Heltec LoRa
- Librerías SGP41
- Librerías BME680
- Node-RED
- InfluxDB
- Grafana

# Tabla de Conexiones

## Heltec 1 – Nodo de Sensores

### Alimentación

| Fuente | Conectado a |
|---|---|
| 3.3V | SGP41 VCC |
| 3.3V | BME680 VCC |
| 5V | MiCS-5524 VCC |
| GND | Todos los sensores |

### Bus I2C

| Pin Heltec 1 | Conectado a |
|---|---|
| GPIO 19 (SDA) | SGP41 SDA + BME680 SDA |
| GPIO 20 (SCL) | SGP41 SCL + BME680 SCL |

### Entrada Analógica

| Pin Heltec 1 | Conectado a |
|---|---|
| GPIO 3 (A0) | MiCS-5524 AOUT |

---

## Sensores utilizados

| Sensor | Variables monitoreadas |
|---|---|
| SGP41 | TVOC, eCO2 |
| BME680 | Temperatura, humedad |
| MiCS-5524 | Gases combustibles / hidrocarburos (lectura RAW) |

---

## Heltec 2 – Receptor LoRa

Heltec 2 no utiliza sensores directamente. Su función es recibir los paquetes enviados por Heltec 1 mediante LoRa y reenviarlos por comunicación UART hacia la tarjeta LilyGO.

| Función | Pin |
|---|---|
| LoRa | Interno en Heltec |
| UART TX | GPIO 26 |
| UART RX | GPIO 25 |
| GND | Común con LilyGO |

---

## Heltec 2 → LilyGO (UART)

Esta conexión permite transferir el mensaje JSON desde la Heltec receptora hacia la LilyGO para su posterior envío mediante red celular.

| Heltec 2 | LilyGO |
|---|---|
| GPIO 26 (TX) | GPIO 13 (RX) |
| GPIO 25 (RX) | GPIO 14 (TX) |
| GND | GND |

---

## LilyGO T-SIM7000G

### Comunicación interna

| Función | Pin |
|---|---|
| Modem TX | GPIO 27 |
| Modem RX | GPIO 26 |
| PWR Control | GPIO 4 |
| UART RX (desde Heltec 2) | GPIO 13 |
| UART TX (hacia Heltec 2) | GPIO 14 |

---

# Tabla de Direccionamiento



| Elemento | Configuración |
|---|---|
| Broker MQTT | Broker público compatible con MQTT |
| Puerto | 1883 |
| Topic de ejemplo | airwatch/demo/datos |
| APN Telcel | internet.itelcel.com |
| Protocolo | MQTT |

> **Nota de seguridad:** El tópico real utilizado durante pruebas y competencias no se muestra públicamente para evitar publicaciones no autorizadas o alteración de datos.

---

# Esquema de Funcionamiento

## Flujo General del Sistema

```text
SGP41
BME680
MiCS-5524
   ↓
HELTEC 1
   ↓ LoRa
HELTEC 2
   ↓ UART
LILYGO T-SIM7000G
   ↓ Red celular (Telcel)
MQTT Broker
   ↓
Node-RED
   ↓
InfluxDB
   ↓
Grafanas

# Instalación y Uso

## Clonación del repositorio

```bash
git clone https://github.com/usuario/AIR-WATCH.git
```

Abrir el proyecto en Visual Studio Code o Arduino IDE según el archivo a utilizar.

---

## Estructura del repositorio

```text
AIR-WATCH/
│
├── README.md
│
├── heltec-1-sensor-node/
│   └── heltec_1_sensor_node.ino
│
├── heltec-2-lora-receiver/
│   └── heltec_2_lora_receiver.ino
│
└── lilygo-cellular-mqtt/
    └── lilygo_cellular_mqtt.ino
```

---

## Carga de códigos

Cada archivo debe cargarse en su tarjeta correspondiente:

| Archivo | Tarjeta |
|---|---|
| heltec_1_sensor_node.ino | Heltec 1 |
| heltec_2_lora_receiver.ino | Heltec 2 |
| lilygo_cellular_mqtt.ino | LilyGO T-SIM7000G |

---

## Secuencia de operación recomendada

1. Encender Heltec 1 y verificar lectura de sensores.
2. Encender Heltec 2 y verificar recepción LoRa.
3. Encender LilyGO y verificar conexión celular.
4. Confirmar publicación MQTT.
5. Verificar recepción en Node-RED.
6. Confirmar almacenamiento en InfluxDB.
7. Revisar dashboards en Grafana.

---

# Alimentación del Sistema

## Alimentación fija (Laboratorio / pruebas)

Durante pruebas de escritorio o laboratorio:

| Dispositivo | Alimentación |
|---|---|
| Heltec 1 | USB 5V |
| Heltec 2 | USB 5V |
| LilyGO | USB 5V o batería |

---

## Alimentación móvil (Campo)

Para pruebas en campo:

| Dispositivo | Alimentación recomendada |
|---|---|
| Heltec 1 | Power bank / batería |
| Heltec 2 | Power bank / batería |
| LilyGO | Batería LiPo 3.7V |

---

## Consideraciones de alimentación

- La LILYGO puede presentar picos altos de consumo al conectarse a red celular.
- Se recomienda batería o fuente estable.
- El uso exclusivo de USB puede ocasionar reinicios en condiciones de baja corriente.
- Verificar polaridades antes de energizar.

---

# Precauciones y Recomendaciones

## Sensores

- Verificar alimentación correcta (3.3V / 5V según sensor).
- No invertir polaridad.
- Revisar conexiones I2C antes de energizar.
- Permitir calentamiento del MiCS-5524 antes de lectura estable.

---

## Comunicación LoRa

- Ambas Heltec deben usar la misma frecuencia.
- Verificar antenas correctamente conectadas.
- Evitar obstáculos metálicos durante pruebas de alcance.

---

## Red Celular

- Verificar SIM activa con datos.
- Confirmar APN correcto.
- Revisar cobertura Telcel en zona de prueba.

---

## Seguridad MQTT

- No publicar tópicos reales en documentación pública.
- Se recomienda usar tópicos privados en pruebas reales.
- Verificar integridad del JSON antes de publicar.

---

# Explicación de los Códigos del Proyecto

## Heltec 1 – Nodo de Sensores

Archivo:

```text
heltec_1_sensor_node.ino
```

### Funciones principales

- Inicializa sensores SGP41
- Inicializa sensor BME680
- Lee MiCS-5524
- Obtiene temperatura y humedad
- Construye mensaje JSON
- Envía datos mediante LoRa
- Muestra información local en pantalla OLED

---

## Heltec 2 – Receptor LoRa

Archivo:

```text
heltec_2_lora_receiver.ino
```

### Funciones principales

- Espera paquetes LoRa
- Recibe mensajes JSON
- Valida recepción
- Muestra información en OLED
- Reenvía mensaje mediante UART hacia LilyGO

---

## LilyGO – Gateway Celular MQTT

Archivo:

```text
lilygo_cellular_mqtt.ino
```

### Funciones principales

- Recibe JSON desde Heltec 2
- Inicializa modem SIM7000
- Se conecta a red celular
- Se conecta a broker MQTT
- Publica mensaje recibido
- Mantiene reconexión automática en caso de fallo

---

# Posibles Mejoras Futuras

- Uso de broker MQTT privado
- Implementación de autenticación segura
- Integración de GPS
- Incorporación de gabinete IP65
- Alimentación solar
- Sensores especializados adicionales
- Alarmas automáticas por umbrales
- Modelos predictivos con análisis histórico

---

# Limitaciones Actuales

- El sistema es un prototipo académico.
- Algunos sensores trabajan como indicadores indirectos.
- El broker MQTT público no es ideal para producción.
- La cobertura celular depende del operador.
- El sistema requiere calibración para aplicaciones críticas.

---

# Autores

## Proyecto Integrador  
Ingeniería en Tecnologías de la Información y Comunicaciones  
ITSOEH  

| Matrícula | Nombre |
|---|---|
| 230110063 | Gustavo Barrera Martínez |
| 230110166 | Leilany Aislin Sanchez Reyes |
| 230110581 | Bryan Fuentes Perez |
| 230110530 | Diego Lozano Camargo |
| 230110579 | Xavier Amed Guerrero Hernandez |

---

# Tecnologías Utilizadas

- IoT (Internet of Things)
- LoRa
- ESP32
- MQTT
- Red Celular LTE
- Node-RED
- InfluxDB
- Grafana
- JSON
- Arduino IDE

---

# Nota Final

Este repositorio contiene el código fuente y documentación técnica del prototipo AIR-WATCH desarrollado con fines académicos y de investigación, enfocado en el monitoreo ambiental de bajo costo mediante tecnologías IoT, comunicación inalámbrica y análisis de datos en tiempo real.

# Evidencias del Prototipo

## Prototipo en pruebas (casa al lado del río)

![Prototipo AIR-WATCH](./img/PROTOTIPO%20CERCA.png)

## Prototipo ensamblado

![AIR-WATCH montaje](./img/EN%20GALLETA.png)

## Plática con directores de CONAGUA

![Reunión CONAGUA](./img/REUNION%20CONAGUA.png)