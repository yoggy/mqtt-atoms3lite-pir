#include <M5Unified.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "config.h"

WiFiClient wifi_client;
PubSubClient mqtt_client(mqtt_host, mqtt_port, NULL, wifi_client);

#include <Adafruit_NeoPixel.h>

#define PIN 1  // G1 pin is the outer pin of AtomS3Lite's Grove connector.
#define LED_NUM 8

Adafruit_NeoPixel button_led(1, 35, NEO_GRB + NEO_KHZ800);

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);

  // Wifi
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.setSleep(false);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    switch (count % 4) {
      case 0:
        Serial.println("|");
        button_led.setPixelColor(0, button_led.Color(10, 10, 0));
        button_led.show();
        break;
      case 1:
        Serial.println("/");
        break;
      case 2:
        button_led.setPixelColor(0, button_led.Color(0, 0, 0));
        button_led.show();
        Serial.println("-");
        break;
      case 3:
        Serial.println("\\");
        break;
    }
    count++;
    if (count >= 240) reboot();  // 240 / 4 = 60sec
  }
  button_led.setPixelColor(0, button_led.Color(10, 10, 0));
  button_led.show();
  Serial.println("WiFi connected!");
  delay(1000);

  // MQTT
  bool rv = false;
  if (mqtt_use_auth == true) {
    rv = mqtt_client.connect(mqtt_client_id, mqtt_username, mqtt_password);
  } else {
    rv = mqtt_client.connect(mqtt_client_id);
  }
  if (rv == false) {
    Serial.println("mqtt connecting failed...");
    reboot();
  }
  Serial.println("MQTT connected!");

  button_led.setPixelColor(0, button_led.Color(0, 0, 10));
  button_led.show();

  // Serial.print("Subscribe : topic=");
  // Serial.println(mqtt_subscribe_topic);
  // mqtt_client.subscribe(mqtt_subscribe_topic);

  delay(1000);

  // configTime
  configTime(9 * 3600, 0, "ntp.nict.jp");
  struct tm t;
  if (!getLocalTime(&t)) {
    Serial.println("getLocalTime() failed...");
    delay(1000);
    reboot();
  }
  Serial.println("configTime() success!");
  button_led.setPixelColor(0, button_led.Color(0, 10, 0));
  button_led.show();

  delay(1000);

  //
  pinMode(1, INPUT); 
}

void reboot() {
  Serial.println("REBOOT!!!!!");
  for (int i = 0; i < 30; ++i) {
    button_led.setPixelColor(0, button_led.Color(255, 0, 255));
    delay(100);
    button_led.setPixelColor(0, button_led.Color(0, 0, 0));
    delay(100);
  }

  ESP.restart();
}

int last_val = 0;
char buf[256];

void loop() {
  mqtt_client.loop();
  if (!mqtt_client.connected()) {
    Serial.println("MQTT disconnected...");
    reboot();
  }

  M5.update();

  // sensing PIR
  int val = digitalRead(1);
  if (last_val != val && val == 1) {
    struct tm t;
    getLocalTime(&t);

    snprintf(buf, 256, "{\"time\":\"%04d-%02d-%02d %02d:%02d:%02d\"}", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    Serial.println(buf);
    mqtt_client.publish(mqtt_publish_topic, buf);
  }
  last_val = val;

}
