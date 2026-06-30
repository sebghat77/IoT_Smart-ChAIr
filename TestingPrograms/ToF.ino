


















#include <Wire.h>
#include <VL6180X.h>

#define SCL_PIN 22  // SCL mit D22 verbinden
#define SDA_PIN 21  // SDA mit D21 verbinden

VL6180X sensor;

// Pins
const int BUZZER_PIN = 25;

// Einstellungen
const int DISTANCE_THRESHOLD = 20;       // mm
const unsigned long TIME_THRESHOLD = 5000; // 5 Sekunden

unsigned long alertTimerStart = 0;
bool timerRunning = false;

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);  // Konsistent mit #define

  sensor.init();
  sensor.configureDefault();
  sensor.setTimeout(500);

  Serial.println("Sensor bereit.");
}

void loop() {
  uint8_t range = sensor.readRangeSingleMillimeters();

  if (!sensor.timeoutOccurred()) {
    Serial.print("Abstand: ");
    Serial.print(range);
    Serial.println(" mm");

    if (range > DISTANCE_THRESHOLD) {
      if (!timerRunning) {
        timerRunning = true;
        alertTimerStart = millis();
      }
      if (millis() - alertTimerStart >= TIME_THRESHOLD) {
        digitalWrite(BUZZER_PIN, HIGH);
      }
    } else {
      timerRunning = false;
      alertTimerStart = 0;  // Timer sauber zurücksetzen
      digitalWrite(BUZZER_PIN, LOW);
    }
  } else {
    Serial.println("Messfehler / Timeout");
  }

  delay(100);
}
