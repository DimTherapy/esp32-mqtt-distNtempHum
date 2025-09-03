#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "secrets.h" // holt WIFI_SSID, WIFI_PASSWORD, MQTT_HOST, MQTT_PORT
#define TRIG_PIN 5 
#define ECHO_PIN 18

// ====== ANPASSEN ======
const char* TOPIC_DISTANCE    = "esp32/distance";
const char* TOPIC_ENV         = "esp32/tempHum";
// ======================

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

float lastTempC = 20.0f;
float lastHumidityH= 0.0f;
unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL_MS = 5000; // alle 5s senden

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WLAN verbinden");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWLAN ok, IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  client.setServer(MQTT_HOST, MQTT_PORT);
  while (!client.connected()) {
    Serial.print("MQTT verbinden...");
    // Client-ID sollte einzigartig sein:
    if (client.connect("esp32-dht22-pub")) {
      Serial.println("ok");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" -> retry in 2s");
      delay(2000);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(2000);
  pinMode(DHTPIN, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("\nStart HC-SR04, DHT22→ MQTT");
  dht.begin();
  connectWiFi();
  connectMQTT();
}

float readDistance(float tempC){
  digitalWrite(TRIG_PIN,LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long d =pulseIn(ECHO_PIN, HIGH, 30000UL); //µs
  if (d==0) return NAN; //kein Echo

  //v(T) in cm/µs
  float cm_per_us= 0.03313f + 0.0000606f * tempC;
  return 0.5f * d * cm_per_us;     // Hin- und Rückweg
}

void loop() {
  // put your main code here, to run repeatedly:
  //DHT nur alle ~2 s lesen
  if (!client.connected()) connectMQTT();
  client.loop();

  char payload[100];
  if(millis() - lastSend >= SEND_INTERVAL_MS) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if(!isnan(t)) lastTempC= t; 
    if(!isnan(h)) lastHumidityH= h; //nur bei gültiger messungen aktualisieren
    lastSend = millis();
  }

  float cm = readDistance(lastTempC);
  if(isnan(cm)){
    Serial.println("Kein Echo (Ultraschall)");
  } else {
    // Topic 1: nur Entfernung (als Zahl)
    snprintf(payload, sizeof(payload), "%.2f", cm);
    bool ok1 = client.publish(TOPIC_DISTANCE, payload);

    // Topic 2: Temperatur & Feuchtigkeit (Text)
    snprintf(payload, sizeof(payload),
             "Temperatur: %.2f °C, Feuchtigkeit: %.2f",
             lastTempC, lastHumidityH);
    bool ok2 = client.publish(TOPIC_ENV, payload);

    Serial.print("Publish -> "); Serial.print(TOPIC_DISTANCE);
    Serial.print(" : "); Serial.print(cm); Serial.print("  [");
    Serial.print(ok1 ? "ok" : "fail"); Serial.println("]");

    Serial.print("Publish -> "); Serial.print(TOPIC_ENV);
    Serial.print(" : "); Serial.print(payload); Serial.print("  [");
    Serial.print(ok2 ? "ok" : "fail"); Serial.println("]");
  }

  delay(2000);
}