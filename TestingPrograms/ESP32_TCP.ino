#include <WiFi.h>

const char* ssid = "Arturo’s iPhone";
const char* password = "X";

/**
 * Steps:
 * 1. SSID and password of your WiFi network (Arturo’s iPhone)
      - In personal AP (from phone), maximize compatibility
 * 2. MQTT Broker IP adress (ifconfig)
 * 3. See the assigned IP address of your ESP32 in the Serial Monitor
 * 4. sudo ufw allow from X to any port 1883 proto tcp
 * 5. sudo ufw reload
 * 6. sudo iptables -F
sudo iptables -X
sudo iptables -t nat -F
sudo iptables -t nat -X
sudo nft flush ruleset 2>/dev/null
 */

const char* host = "172.20.10.4"; 
const int port = 1883;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println(WiFi.localIP());

  WiFiClient client;

  Serial.println("Intentando TCP...");

  if (client.connect(host, port)) {
    Serial.println("TCP OK");
  } else {
    Serial.println("TCP FAIL");
  }
}

void loop() {}
