#include <WebServer.h>
#include <WiFi.h>

// --- Wi-Fi Credentials ---
const char *ssid = "";
const char *password = "";

WebServer server(80); // HTTP server on port 80

// --- Pin Definitions ---
const int TRIG1 = 27;
const int ECHO1 = 35;
const int TRIG2 = 14;
const int ECHO2 = 32;

const int TOUCH_PIN = 34;

const int S_LED1 = 26;
const int S_LED2 = 25;
const int P_LED = 13;

// --- System State Variables ---
bool isSystemOn = false;
bool s1_Triggered = false;
bool s2_Triggered = false;

// --- Non-blocking Auto-Calibration Timer ---
bool pendingAutoCalibrate = false;
unsigned long autoCalibrateTimer = 0;
const int AUTO_CAL_DELAY_MS = 1500;

// --- Ultrasonic Variables ---
const int MAX_DISTANCE_CM = 300;
const int ALARM_THRESHOLD_CM = 15;
int baseline1 = 0;
int baseline2 = 0;
int currentDist1 = 0;
int currentDist2 = 0;

int bufferCount1 = 0;
int bufferCount2 = 0;
const int BUFFER_REQUIRED = 3;

unsigned long lastPingTime = 0;
const int PING_INTERVAL = 40;
bool pingSensor1Next = true;

// --- Button Timing Variables ---
unsigned long touchStartTime = 0;
bool previousTouchState = LOW;
bool longPressHandled = false;
const int SHORT_PRESS_MS = 50;
const int LONG_PRESS_MS = 1000;

void setup() {
        Serial.begin(115200);

        pinMode(TRIG1, OUTPUT);
        pinMode(ECHO1, INPUT);
        pinMode(TRIG2, OUTPUT);
        pinMode(ECHO2, INPUT);
        pinMode(TOUCH_PIN, INPUT);
        pinMode(S_LED1, OUTPUT);
        pinMode(S_LED2, OUTPUT);
        pinMode(P_LED, OUTPUT);

        digitalWrite(S_LED1, LOW);
        digitalWrite(S_LED2, LOW);
        digitalWrite(P_LED, LOW);

        delay(10000);

        // 1. Start Wi-Fi connection
        Serial.print("Connecting to Wi-Fi: ");
        Serial.println(ssid);
        WiFi.begin(ssid, password);

        // Non-blocking-ish Wi-Fi connect (gives up after 10 seconds to allow offline physical mode)
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                Serial.print(".");
                attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nWi-Fi Connected!");
                Serial.print("IP Address for App/Browser: ");
                Serial.println(WiFi.localIP());
        } else {
                Serial.println("\nWi-Fi Failed. Running in offline physical mode.");
        }

        // 2. Setup Web Server Routing
        server.on("/toggle", HTTP_GET, handleWebToggle);
        server.on("/calibrate", HTTP_GET, handleWebCalibrate);
        server.on("/status", HTTP_GET, handleWebStatus);
        server.begin();

        Serial.println("System Initialized. Awaiting commands...");
}

void loop() {
        server.handleClient();
        handleTouchControl();
        handlePendingCalibration();

        if (isSystemOn && !pendingAutoCalibrate) {
                runUltrasonicSystem();
        }
}

// --- Web Server Endpoints ---

void handleWebToggle() {
        toggleSystemPower();
        // Send immediate response back to phone
        server.send(200, "text/plain", isSystemOn ? "System ON" : "System OFF");
}

void handleWebCalibrate() {
        if (isSystemOn) {
                Serial.println("\n--- App Requested Calibration ---");
                calibrateSensors();
                server.send(200, "text/plain", "Calibrated");
        } else {
                server.send(400, "text/plain", "System is offline. Turn on first.");
        }
}

void handleWebStatus() {
        // Build a JSON payload for the Android Dev to parse
        String json = "{";
        json += "\"systemOn\":" + String(isSystemOn ? "true" : "false") + ",";
        json += "\"calibrating\":" + String(pendingAutoCalibrate ? "true" : "false") + ",";
        json += "\"sensor1_Alarm\":" + String(s1_Triggered ? "true" : "false") + ",";
        json += "\"sensor2_Alarm\":" + String(s2_Triggered ? "true" : "false") + ",";
        json += "\"dist1\":" + String(currentDist1) + ",";
        json += "\"dist2\":" + String(currentDist2);
        json += "}";

        server.send(200, "application/json", json);
}

// --- Core Logic Functions ---

void handleTouchControl() {
        bool currentTouchState = digitalRead(TOUCH_PIN);

        if (currentTouchState == HIGH && previousTouchState == LOW) {
                touchStartTime = millis();
                longPressHandled = false;
        }

        if (currentTouchState == HIGH && (millis() - touchStartTime > LONG_PRESS_MS)) {
                if (!longPressHandled && isSystemOn) {
                        Serial.println("\n--- Long Press Detected ---");
                        calibrateSensors();
                        longPressHandled = true;
                }
        }

        if (currentTouchState == LOW && previousTouchState == HIGH) {
                unsigned long pressDuration = millis() - touchStartTime;
                if (pressDuration > SHORT_PRESS_MS && !longPressHandled) {
                        toggleSystemPower();
                }
        }
        previousTouchState = currentTouchState;
}

void toggleSystemPower() {
        isSystemOn = !isSystemOn;
        digitalWrite(P_LED, isSystemOn ? HIGH : LOW);

        if (isSystemOn) {
                Serial.println("\n--- System turned ON ---");
                Serial.println("Stepping back... auto-calibrating in 1.5 seconds.");

                // Start the non-blocking timer
                pendingAutoCalibrate = true;
                autoCalibrateTimer = millis();
        } else {
                Serial.println("\n--- System turned OFF ---");
                pendingAutoCalibrate = false;
                digitalWrite(S_LED1, LOW);
                digitalWrite(S_LED2, LOW);
        }
}

void handlePendingCalibration() {
        if (pendingAutoCalibrate && (millis() - autoCalibrateTimer >= AUTO_CAL_DELAY_MS)) {
                calibrateSensors();
                pendingAutoCalibrate = false;
        }
}

void calibrateSensors() {
        Serial.println("Calibrating...");
        baseline1 = getDistance(TRIG1, ECHO1);
        delay(30);
        baseline2 = getDistance(TRIG2, ECHO2);

        Serial.print("Baseline S1: ");
        Serial.print(baseline1);
        Serial.println(" cm");
        Serial.print("Baseline S2: ");
        Serial.print(baseline2);
        Serial.println(" cm");

        bufferCount1 = 0;
        bufferCount2 = 0;
        s1_Triggered = false;
        s2_Triggered = false;

        digitalWrite(S_LED1, HIGH);
        digitalWrite(S_LED2, HIGH);
        delay(300);
        digitalWrite(S_LED1, LOW);
        digitalWrite(S_LED2, LOW);

        Serial.println("Calibration complete. Monitoring active.");
}

void runUltrasonicSystem() {
        unsigned long currentMillis = millis();

        if (currentMillis - lastPingTime >= PING_INTERVAL) {
                lastPingTime = currentMillis;

                if (pingSensor1Next) {
                        if (!s1_Triggered && baseline1 > 0) {
                                currentDist1 = getDistance(TRIG1, ECHO1);
                                processDistance(1, currentDist1, baseline1, bufferCount1,
                                                s1_Triggered, S_LED1);
                        }
                        pingSensor1Next = false;
                } else {
                        if (!s2_Triggered && baseline2 > 0) {
                                currentDist2 = getDistance(TRIG2, ECHO2);
                                processDistance(2, currentDist2, baseline2, bufferCount2,
                                                s2_Triggered, S_LED2);
                        }
                        pingSensor1Next = true;
                }
        }
}

int getDistance(int trigPin, int echoPin) {
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);

        unsigned long duration = pulseIn(echoPin, HIGH, MAX_DISTANCE_CM * 60);
        if (duration == 0)
                return MAX_DISTANCE_CM;
        return duration / 58;
}

void processDistance(int sensorNum, int currentDist, int baseline, int &bufferCount,
                     bool &triggerFlag, int ledPin) {
        if (currentDist < (baseline - ALARM_THRESHOLD_CM)) {
                bufferCount++;
        } else {
                bufferCount = 0;
        }

        if (bufferCount >= BUFFER_REQUIRED) {
                triggerFlag = true;
                digitalWrite(ledPin, HIGH);
                Serial.print("!!! INTRUDER DETECTED ON SENSOR ");
                Serial.print(sensorNum);
                Serial.print(" !!! Distance: ");
                Serial.print(currentDist);
                Serial.println(" cm");
        }
}
