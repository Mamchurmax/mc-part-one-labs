#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Ronka 13";
const char* password = "08042001";

const char* mqtt_server = "192.168.0.103"; 

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("recieved [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("MQTT:");
  display.setCursor(0, 16);
  display.println(message);
  display.display();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("connecting to MQTT...");
    if (client.connect("ESP8266Client")) {
      Serial.println("OK");
      client.subscribe("oled/text");
    } else {
      Serial.print("error 5 с. Код: ");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200); 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Wi-Fi...");
  display.display();

  setup_wifi();  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Wi-Fi OK, MQTT...");
  display.println(WiFi.localIP());
  display.display();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
