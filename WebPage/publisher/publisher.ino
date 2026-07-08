#include <WiFi.h>
#include <PubSubClient.h>


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

// 1. Configuración de tu red WiFi
const char* ssid = "Arturo’s iPhone";
const char* password = "X";

// 2. Configuración del Broker MQTT (Tu Mac)
// IMPORTANTE: Aquí NO puedes usar "localhost". Debes poner la IP local de tu Mac 
// (ej. "192.168.1.15"). La puedes averiguar en la terminal de tu Mac con: ipconfig getifaddr en0
const char* mqtt_broker = "172.20.10.4"; 
const int mqtt_port = 1883;
const char* topic = "casa/sala/temperatura";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
int contador = 1;

// Función para conectar al WiFi
void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("¡WiFi conectado!");
  Serial.print("IP de la ESP32: ");
  Serial.println(WiFi.localIP());
}

// Función para conectar (o reconectar) al Broker MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // Intentar conectar usando un ID de cliente único
    if (client.connect("ESP32_Client")) {
      Serial.println("¡Conectado al Broker!");
    } else {
      Serial.print("Falló, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos...");
      delay(5000);
    }
  }
}

void setup() {
  setup_wifi();
  client.setServer(mqtt_broker, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Mantiene la comunicación con el broker viva

  unsigned long now = millis();
  // Enviar un mensaje cada 2000 milisegundos (2 segundos)
  if (now - lastMsg > 2000) {
    lastMsg = now;

    // Crear el mensaje simulando la temperatura
    String mensaje = "Temperatura desde ESP32: 2" + String(contador) + "°C";
    
    Serial.print("Publicando mensaje: ");
    Serial.println(mensaje);
    
    // Publicar el mensaje en el broker
    client.publish(topic, mensaje.c_str());

    contador++;
    if (contador > 9) contador = 1;
  }
}
