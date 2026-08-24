#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

#include "config.h"

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  delay(500);
  Serial.println("\nstart");

  dht.begin();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("[ERROR] reading DHT22 data error, going to sleep");
    goToSleep();
  }

  if (temperature < -90.0 || temperature > 90.0 || humidity < 0.0 || humidity > 100.0) {
    Serial.println("[ERROR] value(s) is(are) not in allowed interval, you lowkey shoulda check your sensor");
    goToSleep();
  }

  Serial.printf("[LOCAL] T: %.2f °C | H: %.2f %%\n", temperature, humidity);



  Serial.print("[WIFI] connecting to ");
  Serial.println(SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempt++;
    if (attempt > 30) {
      Serial.println("\n[ERROR] unable to connect to WIFI, going to sleep");
      goToSleep();
    }
  }
  Serial.println("\n[LOG] connected, local IP: " + WiFi.localIP().toString());



  client.setServer(MQTT_BROKER, MQTT_PORT);
  Serial.print("[LOG] connecting to MQTT broker");

  if (client.connect(DEVICE_ID)) {
    char topic[64];
    snprintf(topic, sizeof(topic), "thermometrum/%s/reading", DEVICE_ID);

    JsonDocument doc;
    doc["temperature"] = round(temperature * 100) / 100.0;
    doc["humidity"]    = round(humidity * 100) / 100.0;

    char payload[128];
    size_t n = serializeJson(doc, payload);

    if (client.publish(topic, (const uint8_t*)payload, n, false)) {
      Serial.print("[LOG] sent: ");
      Serial.println(payload);
    } else {
      Serial.println("[ERROR] unable to publish MQTT message");
    }

    client.disconnect();
  } else {
    Serial.print("[ERROR] error occured while using MQTT, status: ");
    Serial.println(client.state());
  }



  goToSleep();
}

void loop() {
  // useless
}

void goToSleep() {
  Serial.println("[LOG] take a nap");
  digitalWrite(LED_BUILTIN, HIGH);
  ESP.deepSleep(SLEEP_TIME);
}
