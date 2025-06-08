#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "Redmi Note 13 Pro+ 5G";
const char* password = "qwertyuiop123";

#define LED_PIN       4   // D2
#define BUTTON_TOGGLE 13  // D7
#define BUTTON_DOWN   12  // D6
#define BUTTON_UP     14  // D5
#define OLED_SDA      2   // D4
#define OLED_SCL      0   // D3

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

ESP8266WebServer server(80);

bool lightState = false;
int brightness = 128;
unsigned long timerEndMillis = 0;

const unsigned long debounceDelay = 100;
unsigned long lastToggleTime = 0;
unsigned long lastUpTime = 0;
unsigned long lastDownTime = 0;

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);

  display.println(lightState ? "Status: ON" : "Status: OFF");
  display.print("Brightness: ");
  display.println(brightness);

  display.display();
}

void applyBrightness() {
  analogWrite(LED_PIN, lightState ? brightness : 0);
  updateDisplay();
}

void handleSetBrightness() {
  if (server.hasArg("value")) {
    brightness = constrain(server.arg("value").toInt(), 0, 255);
    analogWrite(LED_PIN, brightness);
    updateDisplay();
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Missing brightness value");
  }
}

void handleRoot() {
  if (!LittleFS.exists("/index.html")) {
    server.send(500, "text/plain", "index.html not found");
    return;
  }
  File file = LittleFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleStatus() {
  server.send(200, "text/plain", lightState ? "ON" : "OFF");
}

void handleOn() {
  lightState = true;
  timerEndMillis = 0;
  applyBrightness();
  server.send(200, "text/plain", "OK");
}

void handleOff() {
  lightState = false;
  timerEndMillis = 0;
  applyBrightness();
  server.send(200, "text/plain", "OK");
}

void handleSetTimer() {
  if (!server.hasArg("minutes")) {
    server.send(400, "text/plain", "Missing 'minutes' param");
    return;
  }
  int minutes = server.arg("minutes").toInt();
  if (minutes <= 0) {
    server.send(400, "text/plain", "Invalid 'minutes' param");
    return;
  }

  lightState = true;
  timerEndMillis = millis() + minutes * 60UL * 1000UL;
  applyBrightness();

  server.send(200, "text/plain", "Timer set");
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_TOGGLE, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (true);
  }
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("System ready...");
  display.display();

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    while (true);
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected. IP:");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/status", handleStatus);
  server.on("/set_timer", handleSetTimer);
  server.on("/set_brightness", handleSetBrightness);
  server.serveStatic("/script.js", LittleFS, "/script.js");
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.begin();

  applyBrightness();
}

void loop() {
  server.handleClient();
  unsigned long now = millis();

  bool prevToggleState = HIGH;
  unsigned long lastUpTime = 0;
  unsigned long lastDownTime = 0;

  bool toggleState = digitalRead(BUTTON_TOGGLE);
  bool upPressed = digitalRead(BUTTON_UP) == LOW;
  bool downPressed = digitalRead(BUTTON_DOWN) == LOW;

  if (prevToggleState == HIGH && toggleState == LOW) {
    lightState = !lightState;
    applyBrightness();
  }
  prevToggleState = toggleState;

  if (upPressed && (now - lastUpTime > debounceDelay)) {
    brightness = min(brightness + 5, 255);
    applyBrightness();
    lastUpTime = now;
  }

  if (downPressed && (now - lastDownTime > debounceDelay)) {
    brightness = max(brightness - 5, 0);
    applyBrightness();
    lastDownTime = now;
  }

  if (lightState && timerEndMillis && now >= timerEndMillis) {
    lightState = false;
    timerEndMillis = 0;
    applyBrightness();
  }

  delay(20);
}
