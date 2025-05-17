#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#define SSID "ESP8266-AP"
#define PASSWORD "012345678900"
#define BLINK_INTERVAL 1000
#define HOLD_INTERVAL 500


enum class Color {
  RED,
  YELLOW,
  GREEN
};

typedef struct led_s {
  const uint8_t pin;
  bool state;
  led_s *next;
  led_s *prev;
  Color color;
} led_t;

typedef struct button_s {
  uint8_t pin;
  bool state;
  bool wasPressed;
  uint32_t pressStartTime;
  bool hardIsHeld;
  bool webIsHeld;
  bool serialIsHeld;
} button_t;

led_t redLED = {13, LOW, nullptr, nullptr, Color::RED};     // GPIO2
led_t yellowLED = {14, LOW, nullptr, nullptr, Color::YELLOW}; // GPIO14
led_t greenLED = {2, LOW, nullptr, nullptr, Color::GREEN};    // GPIO13

button_t button = {12, LOW, false, 0, false, false, false};   // GPIO12

led_t *currentLED = &redLED;
uint32_t currentTime;
uint32_t previousBlinkTime = 0;
uint8_t serialData;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void handleHold(AsyncWebServerRequest *request) {
  button.webIsHeld = true;
  request->send_P(200, "text/html", "ok");
}

void handleReleased(AsyncWebServerRequest *request) {
  button.webIsHeld = false;
  request->send_P(200, "text/html", "ok");
}

void sendStartSignal(AsyncWebServerRequest *request) {
  Serial.write(0x01);  
  request->send_P(200, "text/html", "ok");
}

void sendStopSignal(AsyncWebServerRequest *request) {
  Serial.write(0x02);  
  request->send_P(200, "text/html", "ok");
}

void sendCurrentLEDtoWEB() {
  if (currentLED == nullptr) {
    return;
  }
  switch (currentLED->color) {
    case Color::RED:
      ws.textAll("red");
      break;
    case Color::YELLOW:
      ws.textAll("yellow");
      break;
    case Color::GREEN:
      ws.textAll("green");
      break;
  }
}


void sendHeldLEDtoWEB(led_t* led1, led_t* led2) {
  String message = "held:";
  switch (led1->color) {
    case Color::RED:
      message += "red";
      break;
    case Color::YELLOW:
      message += "yellow";
      break;
    case Color::GREEN:
      message += "green";
      break;
  }
  message += ",";
  switch (led2->color) {
    case Color::RED:
      message += "red";
      break;
    case Color::YELLOW:
      message += "yellow";
      break;
    case Color::GREEN:
      message += "green";
      break;
  }
  ws.textAll(message);
}

void setupLEDOrder() {
  redLED.next = &yellowLED;
  redLED.prev = &greenLED;
  
  yellowLED.next = &greenLED;
  yellowLED.prev = &redLED;
  
  greenLED.next = &redLED;
  greenLED.prev = &yellowLED;
}

void pinSetup() {
  pinMode(redLED.pin, OUTPUT);
  pinMode(yellowLED.pin, OUTPUT);
  pinMode(greenLED.pin, OUTPUT);
  pinMode(button.pin, INPUT_PULLUP);
}

void serverSetup() {
  WiFi.begin(SSID, PASSWORD);
  
  
  LittleFS.begin();
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.on("/hold", HTTP_GET, handleHold);
  server.on("/release", HTTP_GET, handleReleased);
  server.on("/start", HTTP_GET, sendStartSignal);
  server.on("/stop", HTTP_GET, sendStopSignal);
  server.addHandler(&ws);
  server.begin();
}

void lightLEDs() {
  digitalWrite(redLED.pin, redLED.state);
  digitalWrite(yellowLED.pin, yellowLED.state);
  digitalWrite(greenLED.pin, greenLED.state);
}

void lightNextLED() {
  currentTime = millis();
  
  bool effectivePressed = button.state || button.webIsHeld || button.serialIsHeld;
  
  static bool lastButtonPressed = false;
  static uint32_t buttonPressTime = 0;
  static bool normalLEDOn = false;   
  static uint32_t lastStepTime = 0;    
  
  if (effectivePressed != lastButtonPressed) {
    if (effectivePressed) {
      buttonPressTime = currentTime;
    } else {
      currentLED = currentLED->prev;
      normalLEDOn = false;
      lastStepTime = currentTime;
    }
    lastButtonPressed = effectivePressed;
  }
  
  if (effectivePressed) {
    if (currentTime - buttonPressTime < 500) {
      if (currentTime - lastStepTime >= 500) {
        lastStepTime = currentTime;
        if (!normalLEDOn) {
          redLED.state = LOW;
          yellowLED.state = LOW;
          greenLED.state = LOW;
          currentLED->state = HIGH;
          normalLEDOn = true;
        } else {
          currentLED->state = LOW;
          normalLEDOn = false;
          currentLED = currentLED->next;
        }
        sendCurrentLEDtoWEB();
        lightLEDs();
      }
    } else {
      if (currentTime - lastStepTime >= 500) {
        lastStepTime = currentTime;
        redLED.state = LOW;
        yellowLED.state = LOW;
        greenLED.state = LOW;
        led_t* ledA = currentLED->next;
        led_t* ledB = currentLED->prev;
        ledA->state = HIGH;
        ledB->state = HIGH;
        sendHeldLEDtoWEB(ledA, ledB);
        currentLED = currentLED->next;
        lightLEDs();
      }
    }
  } else {
    if (currentTime - lastStepTime >= 500) {
      lastStepTime = currentTime;
      if (!normalLEDOn) {
        redLED.state = LOW;
        yellowLED.state = LOW;
        greenLED.state = LOW;
        currentLED->state = HIGH;
        normalLEDOn = true;
      } else {
        currentLED->state = LOW;
        normalLEDOn = false;
        currentLED = currentLED->next;
      }
      sendCurrentLEDtoWEB();
      lightLEDs();
    }
  }
}

void handleButtonHold() {
  button.state = digitalRead(button.pin) == LOW;
  
  if (button.state) {
    if (!button.wasPressed) {
      button.wasPressed = true;
      button.pressStartTime = millis();
    } else if (millis() - button.pressStartTime >= HOLD_INTERVAL) {
      button.hardIsHeld = true;
    }
  } else {
    if (button.wasPressed) {
      button.hardIsHeld = false;
    }
    button.wasPressed = false;
  }
}

void checkSerial() {
  if (Serial.available() > 0) {
    serialData = Serial.read() & 0b00111111; // 6 bits mask
    switch (serialData) {
      case 0x01: 
        button.serialIsHeld = true;
        break;
      case 0x02:  
        button.serialIsHeld = false;
        break;
    }
  }
}

void setup() {
  Serial.begin(115200, SERIAL_8O2);
  setupLEDOrder();
  pinSetup();
  serverSetup();
}

void loop() {
  handleButtonHold();
  checkSerial();
  lightNextLED();
  ws.cleanupClients();
}