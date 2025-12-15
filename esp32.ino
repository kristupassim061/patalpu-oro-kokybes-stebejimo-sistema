#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ===== WIFI NUSTATYMAI =====
const char* WIFI_SSID = "Tavo_WIFI pavadinimas";
const char* WIFI_PASS = "WIFI_SLAPTAZODIS";

// ===== MQTT NUSTATYMAI =====
const char* MQTT_HOST  = "IP adresas";   // TAVO PC IP
const int   MQTT_PORT  = 1883;
const char* MQTT_TOPIC = "sensors/airjson";

// ===== DHT22 =====
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ===== MQ-135 =====
#define MQ135_PIN 34

WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long lastSend = 0;

// ===== WIFI CONNECT =====
void connectWiFi() {
  Serial.print("Jungiuosi prie WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi prisijungta!");
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

// ===== MQTT CONNECT =====
void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Jungiuosi prie MQTT...");
    if (mqtt.connect("esp32-air-quality")) {
      Serial.println(" OK");
    } else {
      Serial.print(" klaida, rc=");
      Serial.println(mqtt.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  dht.begin();
  analogReadResolution(12);                  // 0–4095
  analogSetPinAttenuation(MQ135_PIN, ADC_11db);

  connectWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  connectMQTT();

  Serial.println("Sistema paleista – siunčiu duomenis į MQTT");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  if (millis() - lastSend < 2000) return;
  lastSend = millis();

  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  int mq     = analogRead(MQ135_PIN);

  if (isnan(temp) || isnan(hum)) {
    Serial.println("DHT22 klaida – praleidžiu matavimą");
    return;
  }

  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"temp\":%.2f,\"hum\":%.2f,\"mq135\":%d}",
           temp, hum, mq);

  mqtt.publish(MQTT_TOPIC, payload);

  Serial.print("Išsiųsta: ");
  Serial.println(payload);
}
