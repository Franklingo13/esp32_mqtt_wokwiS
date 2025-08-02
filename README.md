# Sistema IoT de Monitoreo y Control con ESP32 y MQTT

| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

## Descripción del Proyecto

Este proyecto implementa un sistema completo de Internet de las Cosas (IoT) utilizando un microcontrolador ESP32 para el monitoreo y control automático de un **vivero** y una **pecera**. El sistema integra múltiples sensores y actuadores comunicándose a través del protocolo MQTT, permitiendo control remoto y visualización de datos en tiempo real mediante Node-RED.

### Características Principales

- **Conectividad WiFi** con reconexión automática
- **Comunicación MQTT** para control remoto y telemetría
- **Monitoreo de sensores** en tiempo real
- **Control automático de actuadores**
- **Interfaz web** con Node-RED para visualización y control
- **Integración con ThingSpeak** para almacenamiento de datos en la nube

### Sensores Implementados

1. **DHT22** - Medición de temperatura y humedad del vivero
2. **HC-SR04** - Sensor ultrasónico para nivel de agua
3. **NTC** - Sensor de temperatura para la pecera
4. **Pulsador** - Control manual local

### Actuadores Controlados

1. **Relé** - Control de sistema de riego automático
2. **Servo Motor** - Dispensador automático de alimento para peces
3. **LEDs indicadores** - Estado del sistema y conexiones

### Topics MQTT

- `patio/vivero/temperatura` - Temperatura del vivero
- `patio/vivero/humedad` - Humedad del vivero
- `patio/vivero/nivel` - Nivel de agua del tanque
- `patio/vivero/riego` - Control del sistema de riego
- `patio/pecera/temperatura` - Temperatura de la pecera
- `patio/pecera/alimento` - Control del dispensador de alimento
- `channels/3017431/publish` - Datos para ThingSpeak

## Estructura del Proyecto

```
esp32_mqtt_wokwiS/
├── CMakeLists.txt                 # Configuración principal de CMake
├── main/
│   ├── CMakeLists.txt            # Configuración de CMake para main
│   └── main.c                    # Código principal de la aplicación
├── components/                   # Componentes personalizados
│   ├── DHT22/                   # Driver para sensor DHT22
│   │   ├── CMakeLists.txt
│   │   ├── dht.c
│   │   └── include/dht.h
│   ├── servo_motor/             # Driver para control de servo
│   │   ├── CMakeLists.txt
│   │   ├── servo_motor.c
│   │   └── include/servo_motor.h
│   └── dc_motor/                # Driver para motores DC (no usado)
│       ├── CMakeLists.txt
│       ├── dc_motor.c
│       └── include/dc_motor.h
├── node-red/
│   └── flows.json               # Flujos de Node-RED para interfaz web
├── .devcontainer/               # Configuración para desarrollo en contenedor
│   ├── devcontainer.json
│   └── Dockerfile
├── .vscode/                     # Configuración de Visual Studio Code
│   ├── c_cpp_properties.json
│   ├── launch.json
│   └── settings.json
├── build/                       # Archivos compilados (generados automáticamente)
├── diagram.json                 # Esquema del circuito para Wokwi
├── wokwi.toml                  # Configuración de simulación Wokwi
├── .gitignore                  # Archivos ignorados por Git
└── README.md                   # Este archivo
```

## Configuración de Hardware

### Conexiones GPIO

| Componente | Pin GPIO | Descripción |
|------------|----------|-------------|
| LED Estado | GPIO 12  | Indicador de conexión WiFi |
| Pulsador   | GPIO 35  | Control manual |
| Relé       | GPIO 32  | Control de bomba de riego |
| NTC        | GPIO 34  | Sensor de temperatura (ADC) |
| HC-SR04 TRIG | GPIO 26 | Trigger del sensor ultrasónico |
| HC-SR04 ECHO | GPIO 25 | Echo del sensor ultrasónico |
| Servo Motor | GPIO 33  | PWM para control del servo |
| DHT22      | GPIO 13  | Sensor de temperatura/humedad |

## Cómo usar el proyecto

### Prerrequisitos

- ESP-IDF v4.4 o superior
- Visual Studio Code con extensión ESP-IDF
- Node-RED para la interfaz web (opcional)
- Cuenta en ThingSpeak para almacenamiento en la nube (opcional)

### Compilación y Flasheo

1. **Configurar el entorno ESP-IDF:**
   ```bash
   idf.py set-target esp32
   ```

2. **Configurar parámetros WiFi y MQTT** en [`main/main.c`](main/main.c):
   ```c
   #define WIFI_SSID "tu_red_wifi"
   #define WIFI_PASSWORD "tu_contraseña"
   #define CONFIG_BROKER_URL "tu_broker_mqtt"
   ```

3. **Compilar el proyecto:**
   ```bash
   idf.py build
   ```

4. **Flashear al ESP32:**
   ```bash
   idf.py flash monitor
   ```

### Simulación en Wokwi

El proyecto incluye configuración para simulación en Wokwi:

1. Abrir [Wokwi](https://wokwi.com/)
2. Importar el archivo [`diagram.json`](diagram.json)
3. Cargar el firmware compilado
4. Ejecutar la simulación

### Interfaz Node-RED

1. Importar los flujos desde [`node-red/flows.json`](node-red/flows.json)
2. Configurar la conexión MQTT con tu broker
3. Acceder al dashboard para control y monitoreo

## Funcionalidades

### Monitoreo Automático
- Lectura periódica de sensores cada 16 segundos
- Publicación automática de datos vía MQTT
- Alertas por nivel de agua bajo

### Control Remoto
- Activación/desactivación del sistema de riego
- Control manual del dispensador de alimento
- Visualización en tiempo real del estado del sistema

### Almacenamiento de Datos
- Envío automático a ThingSpeak para análisis histórico
- Logs detallados en consola para depuración

## Autor

**Franklin Gómez**  
Universidad de Cuenca - Curso EmbeIoT 2024

## Licencia

Este proyecto está bajo la Licencia MIT. Ver el archivo de licencia para más detalles.
