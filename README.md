# Sistema domótico distribuido con aplicación Qt/QML, nodos ESP8266 y controlador Tiva con FreeRTOS

Este repositorio contiene el código fuente y la documentación del **Trabajo Fin de Grado (TFG)** del Grado en Ingeniería de Sistemas Electrónicos.  
El proyecto desarrolla un **sistema domótico distribuido** basado en **nodos ESP8266**, un **controlador Tiva C con FreeRTOS** y una **aplicación Qt/QML** para control y supervisión.

---

## 🧩 Estructura del repositorio

- **Automatizacion_App_QT/** → Aplicación de control desarrollada en Qt/QML.  
- **Gateway_Tiva_CCS/** → Código del controlador Tiva C con FreeRTOS.  
- **Nodo_ESP8266/** → Código de los nodos ESP8266 para sensado y comunicación.  


---

## ⚙️ Descripción general

El sistema permite la **monitorización de variables ambientales** (temperatura, humedad, iluminación, etc.) y el **control de actuadores** en distintas habitaciones mediante una interfaz gráfica moderna.La comunicación se realiza mediante **UART y WiFi**.

---

## 🧠 Tecnologías principales

- **ESP8266 (NodeMCU)** → Nodos sensores y actuadores.  
- **Tiva TM4C123GH6PM** → Controlador central ejecutando **FreeRTOS**.  
- **FreeRTOS** → Sistema operativo en tiempo real para multitarea.  
- **Qt 6 / QML** → Desarrollo de la interfaz de usuario multiplataforma.  
- **C++ / Arduino Framework** → Lógica de control y comunicación.   
- **UART / WiFi** → Protocolos de comunicación.  

---

## 📦 Instalación y configuración

1. **Aplicación Qt/QML**  
   - Abre la carpeta `Automatizacion_App_QT` en *Qt Creator 6.8 o superior*.  
   - Compila con MSVC 2019 o MinGW.  

2. **Gateway (Tiva + FreeRTOS)**  
   - Abre el proyecto `Gateway_Tiva_CCS` en *Code Composer Studio (CCS)*.  
   - Asegura tener instalados **TivaWare** y **FreeRTOS**, con rutas configuradas.  

3. **Nodo (ESP8266)**  
   - Abre `Nodo_ESP8266/Hab1.ino` en *Arduino IDE* o *PlatformIO*.  
   - Instala las librerías necesarias: `ESP8266WiFi`, `Adafruit_SSD1306`, `Adafruit_GFX`.  
   - Configura los parámetros de red WiFi antes de compilar.  

---

## 👩‍💻 Autor

**Francisco López Jiménez**
Universidad de Málaga.
Grado en Ingeniería de Sistemas Electrónicos.
Año: 2025.  

---

## 📄 Licencia

Este proyecto se distribuye bajo la licencia  
**Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)**.  
Consulta los términos completos en:  
🔗 [https://creativecommons.org/licenses/by-nc-sa/4.0/](https://creativecommons.org/licenses/by-nc-sa/4.0/)

---

# 🇬🇧 Distributed Home Automation System with Qt/QML Application, ESP8266 Nodes and Tiva Controller running FreeRTOS

This repository contains the source code and documentation of a **Final Degree Project (TFG)** in  Electronic Systems Engineering.  
It implements a **distributed home automation system** composed of **ESP8266 nodes**, a **Tiva C controller with FreeRTOS**, and a **Qt/QML application** for remote control and monitoring.

---

## 🧩 Repository Structure

- **Automatizacion_App_QT/** → Qt/QML control application.  
- **Gateway_Tiva_CCS/** → Tiva C controller with FreeRTOS.  
- **Nodo_ESP8266/** → ESP8266 node firmware.  
 
---

## 👩‍💻 Author

**Francisco López Jiménez**  
University of Málaga.
Degree in Electronic Systems Engineering.  
Year: 2025.  

---

## ⚙️ Overview

The system monitors environmental variables (temperature, humidity, light, etc.) and controls actuators across different rooms through a modern graphical interface.  
Communication is handled via **UART and WiFi**.

---

## 🧠 Key Technologies

- **ESP8266 (NodeMCU)** – Sensor and actuator nodes.  
- **Tiva TM4C123GH6PM** – Central controller running **FreeRTOS**.  
- **FreeRTOS** – Real-Time Operating System.  
- **Qt 6 / QML** – Cross-platform user interface.  
- **C++ / Arduino Framework** – Control and communication logic.   
- **UART / WiFi** – Communication protocols.  

---



## 📄 License

This project is licensed under the  
**Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)**.  
Full text:  
🔗 [https://creativecommons.org/licenses/by-nc-sa/4.0/](https://creativecommons.org/licenses/by-nc-sa/4.0/)
