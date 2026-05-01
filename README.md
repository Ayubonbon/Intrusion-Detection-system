# Intrusion Detection System (ESP32 + Android)

## Overview

This project implements a simple intrusion detection system using an ESP32 and an Android application.

The ESP32 handles sensor input and detection logic. It exposes a local HTTP server that provides system state and accepts control commands. The Android application connects to the ESP32 over the same network and provides a user interface to monitor and control the system.

The two components are designed to work together over a local Wi-Fi network without requiring any external services.

---

## System Architecture

The system is divided into two parts:

### ESP32 (Firmware)

* Reads data from ultrasonic sensors and a touch input
* Maintains baseline values and detects anomalies
* Runs a local HTTP server on port 80
* Exposes endpoints for status and control

### Android Application

* Connects to the ESP32 using its local IP address
* Periodically requests system status
* Displays sensor readings and alarm states
* Sends control commands to the ESP32

---

## Communication Model

All communication happens over HTTP within a local network.

```text id="arch1"
Android App  →  HTTP Requests  →  ESP32 Server
Android App  ←  JSON Responses ←  ESP32 Server
```

Endpoints exposed by the ESP32:

* `GET /status` — returns current system state and sensor data
* `GET /toggle` — toggles the system ON/OFF
* `GET /calibrate` — resets alarms and updates baseline values

---

## Project Structure

```text id="arch2"
Intrusion-Detection-system/
│
├── esp32/
│   └── Firmware and hardware-related code
│
├── android-app/
│   └── Android application (client)
```

---

## Setup Instructions

### 1. ESP32 Setup

* Open the code in the `esp32/` directory using Arduino IDE
* Update Wi-Fi credentials (router or hotspot)
* Upload the firmware to the ESP32
* Open Serial Monitor and note the assigned IP address

---

### 2. Android App Setup

* Open the `android-app/` project in Android Studio
* Update the base URL to match the ESP32 IP:

```java id="rootbase"
String BASE_URL = "http://<ESP32_IP>/";
```

* Build and run the app on a device

---

### 3. Network Requirements

* The ESP32 and Android device must be connected to the same Wi-Fi network
* A mobile hotspot can be used for demonstration:

  * One device provides hotspot
  * ESP32 connects to it
  * Android device connects to the same hotspot

---

## How the System Operates

1. The ESP32 initializes and connects to the configured Wi-Fi network
2. After startup, it calibrates baseline distances for sensors
3. The Android app sends periodic requests to `/status`
4. The ESP32 responds with current readings and alarm flags
5. The app updates the UI based on this data
6. User actions in the app trigger `/toggle` or `/calibrate`

All detection logic is handled on the ESP32. The app only reflects and controls the system state.

---

## Notes

This project focuses on local communication between an embedded device and a mobile client. It is intended to demonstrate integration between hardware, networking, and a user interface without relying on external infrastructure.
