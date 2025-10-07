# Aplicación Qt/QML para el sistema domótico distribuido

Esta aplicación forma parte del **Trabajo Fin de Grado (TFG)** *“Sistema domótico distribuido con aplicación Qt/QML, nodos ESP8266 y controlador Tiva con FreeRTOS”*.  
Está desarrollada con **Qt 6 y QML**, y permite el **control remoto y la visualización de datos** del sistema domótico distribuido.

La aplicación actúa como **interfaz de usuario (UI)**, conectándose por **TCP/IP** a los distintos nodos y al controlador Tiva.  
Permite encender o apagar luces, solicitar datos ambientales y mostrar mensajes de estado (por ejemplo, “Luz encendida”, “Error de conexión”, etc.).  
Además, está diseñada con una arquitectura modular que facilita la ampliación a más habitaciones o dispositivos en el futuro.

---

# 🇬🇧 Qt/QML Application for the Distributed Home Automation System

This application is part of the **Final Degree Project (TFG)** *“Distributed Home Automation System with Qt/QML Application, ESP8266 Nodes and Tiva Controller running FreeRTOS”*.  
Developed using **Qt 6 and QML**, it enables **remote control and real-time monitoring** of the distributed home automation system.

The app serves as the **user interface**, connecting via **TCP/IP** to both the ESP8266 nodes and the Tiva controller.  
It allows users to switch lights on/off, request environmental data, and display system messages such as “Light ON” or “Connection Error”.  
Its modular design makes it easy to extend the system with more rooms or devices.


---


## 🧩 Estructura de archivos

Automatizacion_App_QT/
│
├── main.cpp # Punto de entrada de la aplicación
├── MyController.cpp/.h # Controlador de la lógica y comunicación TCP
├── Main.qml # Ventana principal de la interfaz
├── Hab1Controls.qml # Controles específicos de la Habitación 1
├── Hab2Controls.qml # Controles específicos de la Habitación 2
├── GlobalControls.qml # Controles globales compartidos (modo automático, mensajes, etc.)
├── resources.qrc # Archivo de recursos (íconos, fuentes, imágenes)
├── CMakeLists.txt # Configuración del proyecto Qt
└── README.md # Este archivo

---

## ⚙️ Tecnologías y dependencias

- **Qt 6.8+ / QML**
- **C++17**
- **Qt Creator 16.0 o superior**
- **MSVC 2019 o MinGW 8.1**
- **Sockets TCP/IP**

---

## 🚀 Ejecución del proyecto

1. Abre la carpeta del proyecto en **Qt Creator**.  
2. Selecciona el kit de compilación apropiado (MSVC o MinGW).  
3. Compila y ejecuta.  
4. Asegúrate de que los nodos ESP8266 y la Tiva estén conectados en la misma red WiFi para la comunicación.

---

## 🧠 Funcionalidades principales

- Conexión y control de los distintos nodos del sistema domótico.  
- Visualización en tiempo real del estado de cada habitación.  
- Envío de comandos a través de TCP.  
- Modo automático y manual.  
- Mensajes de estado y alertas visuales en pantalla.  
- Diseño escalable para añadir más habitaciones o dispositivos.

---

## 🎨 Interfaz de usuario

La interfaz se ha desarrollado íntegramente en **QML**, empleando un diseño limpio y minimalista, optimizado para pantallas táctiles.  
El esquema de navegación permite acceder fácilmente a cada habitación y visualizar el estado de sensores y actuadores.

---

## 📄 Licencia

Esta aplicación se distribuye bajo la licencia  
**Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)**.  
Consulta los términos completos en:  
🔗 [https://creativecommons.org/licenses/by-nc-sa/4.0/](https://creativecommons.org/licenses/by-nc-sa/4.0/)

---


