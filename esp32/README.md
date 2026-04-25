# ESP32 Intrusion Detection System Documentation

## 1. Hardware Specifications

### 1.1 Components

| Component     | Description             | Operating Voltage        | Key Specifications                                                                                                                        |
| :------------ | :---------------------- | :----------------------- | :---------------------------------------------------------------------------------------------------------------------------------------- |
| **ESP32**     | Main microcontroller    | 3.3V (Logic) / 5V (VIN)  | Dual-core, 2.4 GHz Wi-Fi. Handles non-blocking sensor polling and HTTP server simultaneously.                                             |
| **AJ-SR04M**  | Ultrasonic Sensor 1     | 5V                       | Range: 20cm - 600cm. Waterproof probe. **Note:** Requires a voltage divider on the Echo pin to step down 5V to 3.3V to protect the ESP32. |
| **RCWL-1655** | Ultrasonic Sensor 2     | 3.3V                     | Range: 2cm - 400cm. 3.3V logic compatible, directly connectable to ESP32.                                                                 |
| **TTP223**    | Capacitive Touch Sensor | 2V - 5.5V                | Digital output (HIGH on touch). Response time: ~60ms (fast mode) / ~220ms (low power mode).                                               |
| **LEDs**      | Status Indicators (x3)  | ~2V (Requires Resistors) | Standard 5mm LEDs. Use 220Ω - 330Ω resistors in series to limit current draw.                                                             |

### 1.2 Pin Mapping & Characteristics

| Device Pin   | ESP32 Pin   | Mode   | Special Characteristics / Notes                                                                                       |
| :----------- | :---------- | :----- | :-------------------------------------------------------------------------------------------------------------------- |
| US 1 Trig    | **GPIO 27** | OUTPUT | Standard I/O pin.                                                                                                     |
| US 1 Echo    | **GPIO 35** | INPUT  | **Input-only pin.** No internal pull-up/pull-down resistors. Requires voltage divider if sensor outputs 5V.           |
| US 2 Trig    | **GPIO 14** | OUTPUT | **Strapping pin.** Outputs a faint PWM signal during boot. Normal operation resumes after boot sequence.              |
| US 2 Echo    | **GPIO 32** | INPUT  | Standard I/O pin.                                                                                                     |
| TTP223 I/O   | **GPIO 34** | INPUT  | **Input-only pin.** No internal pull resistors. Driven by external module logic.                                      |
| Sensor 1 LED | **GPIO 26** | OUTPUT | Standard I/O pin.                                                                                                     |
| Sensor 2 LED | **GPIO 25** | OUTPUT | Standard I/O pin.                                                                                                     |
| Power LED    | **GPIO 13** | OUTPUT | **MTDI Strapping pin.** Must be pulled LOW or left floating at boot (naturally handled by the LED circuit to ground). |

---

## 2. Software Configuration & Logic

### 2.1 Core Working Principle

The system utilizes a non-blocking state machine. To prevent acoustic interference (cross-talk) between the two ultrasonic sensors, they are pinged sequentially rather than simultaneously. Background timers manage the Wi-Fi stack, HTTP server, and physical button debouncing without interrupting the sensor polling cycle.

### 2.2 Critical Constants & Structures

- **`MAX_DISTANCE_CM = 300`**: The maximum range (in cm) the system cares about. Prevents `pulseIn()` from blocking indefinitely. If an echo takes longer than this distance implies, it times out and returns `MAX_DISTANCE_CM`.
- **`ALARM_THRESHOLD_CM = 15`**: The delta (in cm). An intruder is detected if the current distance reading is less than `(baseline - 15)`.
- **`BUFFER_REQUIRED = 3`**: False-positive filter. An alarm is only triggered if an anomaly is detected for **3 consecutive polling cycles**.
- **`PING_INTERVAL = 40`**: Time (in milliseconds) between pings. Sensor 1 pings, 40ms later Sensor 2 pings, 40ms later Sensor 1 pings again. Yields an effective update rate of >10Hz per sensor.
- **`AUTO_CAL_DELAY_MS = 1500`**: A 1.5-second non-blocking buffer triggered upon system startup, allowing the user to step away from the device before the baseline distance is locked in.

---

## 3. API Reference

The ESP32 runs an unencrypted HTTP web server on port `80`. Connect the Android device to the configured Wi-Fi hotspot to issue requests.

### 3.1 Toggle Power

- **Endpoint:** `/toggle`
- **Method:** `GET`
- **Description:** Toggles the system state between ON and OFF. Initiates auto-calibration (after 1.5s) if turning ON.
- **Response (Text):** `System ON` or `System OFF`
- **Status Code:** `200 OK`

### 3.2 Calibrate / Acknowledge Alarm

- **Endpoint:** `/calibrate`
- **Method:** `GET`
- **Description:** Instantly captures new baselines for both sensors and resets any active `s1_Triggered` or `s2_Triggered` alarm states. The system must be ON.
- **Response (Text):** `Calibrated` (on success), `System is offline. Turn on first.` (on failure)
- **Status Code:** `200 OK` or `400 Bad Request`

### 3.3 Get System Status

- **Endpoint:** `/status`
- **Method:** `GET`
- **Description:** Returns a real-time snapshot of the system states, including raw distances and alarm flags. Useful for polling at 1Hz in the Android application.
- **Response Format:** `application/json`
- **Status Code:** `200 OK`

**Example JSON Response:**

```json
{
  "systemOn": true,
  "calibrating": false,
  "sensor1_Alarm": false,
  "sensor2_Alarm": true,
  "dist1": 215,
  "dist2": 45
}
```

- `calibrating`: Indicates if the 1.5s startup delay is currently ticking.
- `dist1` / `dist2`: Current raw distance readings in centimeters.
- `sensor[x]_Alarm`: If `true`, the intruder buffer threshold has been met and the physical LED is locked on.

---

## 4. Usage Guide

### 4.1 Physical Operation

1.  **Turn ON:** Perform a **Short Press** (< 1 second) on the TTP223 touch pad. The Power LED will illuminate. You have 1.5 seconds to move away from the sensors before it automatically locks in the room's baseline distances.
2.  **Acknowledge Alarm / Recalibrate:** Perform a **Long Press** (> 1 second) on the touch pad. The Status LEDs will blink once. Active alarms will be cleared, and new baseline distances will be locked in.
3.  **Turn OFF:** Perform a **Short Press** while the system is running. All LEDs will turn off, and sensor polling will stop.

### 4.2 Network / App Operation

1.  Ensure the Android App is connected to the same Wi-Fi network/hotspot as configured in the ESP32 firmware.
2.  Fetch the ESP32's assigned IP address (printed via Serial over USB on boot, or found in the hotspot's connected devices list).
3.  The App should poll `http://<ESP_IP>/status` to sync the UI with the physical state.
4.  The App can send `GET` requests to `/toggle` and `/calibrate` to control the device remotely, mirroring the physical touch sensor's functionality.
