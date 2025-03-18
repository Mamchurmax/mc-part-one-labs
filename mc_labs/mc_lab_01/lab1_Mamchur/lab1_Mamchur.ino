#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define LED1 13    // D7 (GPIO13)
#define LED2 12    // D6 (GPIO12)
#define LED3 2     // D4 (GPIO2)
#define BUTTON_PIN 0 // GPIO0

bool reverseSequence = false;
uint32_t lastInterruptTime = 0;
const uint8_t debounceDelay = 50;

const char* ssid = "Ronka 13";
const char* password = "08042001";

ESP8266WebServer server(80);

unsigned long lastMillis = 0;
const uint16_t delayTime = 500;

bool isLedOn = false;
unsigned long ledTimer = 0;

void ICACHE_RAM_ATTR handleButton() {
    static bool buttonPressed = false;
    uint32_t interruptTime = millis();

    if (interruptTime - lastInterruptTime > debounceDelay) {
        if (!buttonPressed) { 
            buttonPressed = true;
        } else { 
            reverseSequence = !reverseSequence;
            buttonPressed = false;
        }
        lastInterruptTime = interruptTime;
    }
}


void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    uint32_t maxTimer = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        
        if (millis() - maxTimer >= 60000) {
            Serial.println("WiFi connection timeout. Restarting...");
            ESP.restart();
        }
    }
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButton, CHANGE);

    server.on("/", HTTP_GET, []() {
        String html = "<html><body><h1>ESP8266 KNOPKA</h1><br><br>";
        html += "<button onclick=\"window.location.href='/toggle'\">";
        html += reverseSequence ? "Reverse" : "Default";
        html += "</button></body></html>";
        server.send(200, "text/html", html);
    });

    server.on("/toggle", HTTP_GET, []() {
        reverseSequence = !reverseSequence;
        server.sendHeader("Location", "/");
        server.send(303);
    });

    server.begin();
}

void loop() {
    server.handleClient();

    if (millis() - lastMillis >= delayTime) {
        lastMillis = millis();
        lightSequence(reverseSequence); // Викликаємо правильну функцію
    }

    handleLedTiming();
}

void lightSequence(bool reverse) {
    static uint8_t currentStep = 0; // Використовуємо 1 байт пам’яті

    const uint8_t sequenceForward[] = { LED1, LED2, LED3 };
    const uint8_t sequenceReverse[] = { LED3, LED2, LED1, LED2 };

    if (reverse) {
        startLight(sequenceReverse[currentStep]);
        currentStep = (currentStep + 1) % 4;
    } else {
        startLight(sequenceForward[currentStep]);
        currentStep = (currentStep + 1) % 3;
    }
}



void startLight(int led) {
    digitalWrite(led, HIGH);
    isLedOn = true;
    ledTimer = millis();
}

void handleLedTiming() {
    if (isLedOn && millis() - ledTimer >= delayTime / 2) {
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, LOW);
        isLedOn = false;
    }
}
