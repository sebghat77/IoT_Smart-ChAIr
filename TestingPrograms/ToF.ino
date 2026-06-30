CONNECTIONS: 

The VL6180X sensor connects to the ESP32 as follows:
VCC  -> 3.3V
GND  -> GND
SCL  -> GPIO 22
SDA  -> GPIO 21
  
Note: The VL6180X runs on 3.3V only - connecting it to 5V will damage the sensor.

ISSUES DURING TESTING: 

- Naming conflict with ESP32 internal functions: 
  Our variable "timerStart" conflicted with the ESP32's internal timerStart() function defined in esp32-hal-timer.h, causing a compilation error. 
  The fix was to rename our variable to "alertTimerStart" to avoid the conflict. 
  When working with ESP32, reserved timer-related names like timerStart, timerStop, timerRead, and timerWrite must be avoided.

- Incorrect sensor initialization:
  The code initially used vl.begin() to initialize the sensor, which does not exist in the VL6180X library. The correct initialization sequence is:
      sensor.init();
      sensor.configureDefault();
      sensor.setTimeout(500);

- Wrong API methods and error checking: 
  readRange() and readRangeStatus() were used, along with the non-existent constant VL6180X_ERROR_NONE. 
  The correct approach is readRangeSingleMillimeters() for distance readings and timeoutOccurred() for error checking.

- I2C pin initialization: 
  On the ESP32, I2C pins are not set automatically - Wire.begin(SDA_PIN, SCL_PIN) must be called explicitly before initializing the sensor, 
  otherwise the sensor will not be detected on the bus.

- * Breadboard to small*: 
  While testing I found out that one of our breadboard is to slim for our ESP32, resulting in only one side being open to put pins in


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
