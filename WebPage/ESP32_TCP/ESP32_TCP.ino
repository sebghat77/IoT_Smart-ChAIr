#include <WiFi.h>

const char* ssid = "Arturo’s iPhone";
const char* password = "X";

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
