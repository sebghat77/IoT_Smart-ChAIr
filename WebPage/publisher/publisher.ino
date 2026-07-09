#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define DEBUGGING   // Comment when not debugging


// ---------- Seat FSR Sensors ----------
#define SEAT_FSR_FL 34   // Front Left
#define SEAT_FSR_FR 35   // Front Right
#define SEAT_FSR_BL 32   // Back Left
#define SEAT_FSR_BR 33   // Back Right

// ---------- Backrest FSR Sensors ----------
#define BACK_FSR_UP 36   // Backrest Upper
#define BACK_FSR_LOW 39   // Backrest Lower

// ---------- FSR Configuration ----------
// TODO: These values need to be calibrated when the sensors are installed in their final positions.
#define FSR_THRESHOLD 500   // Minimum ADC value to count as pressure
#define FSR_BALANCE_TOLERANCE 20  // Allowed difference between paired sensors in percentage

// --------- PIR Sensor ----------
// DONE: Its FoV needs to be calibrated when the sensor is installed in its final position.
#define PIR_PIN 13
#define WARMUP_TIME 15000UL // Time in milliseconds for sensors to stabilize after powering up

// --------- Buzzer ----------
#define BUZZER_PIN 18


// --------- General Alert System ----------

/**
 * @brief Enum representing the state of an alert.
 * 
 * OK_STATE: No alert is active.
 * WARNING_STATE: An alert condition has been detected, but it has not yet been active for the minimum duration required to be considered an alert.
 * ALERT_STATE: An alert condition has been detected and has been active for the minimum duration required to be considered an alert.
 */
enum alertState {
  OK_STATE = 0,
  WARNING_STATE = 1,
  ALERT_STATE = 2
};

#define MIN_WARNING_DURATION 5*1000UL // Minimum duration in milliseconds for a warning to be considered an alert
unsigned long startSeatWarningTime      = 0;    // Time when the seat warning started
unsigned long startBackWarningTime      = 0;    // Time when the backrest warning started
unsigned long startMovementWarningTime  = 0;    // Time when the movement warning started


// ------- MQTT Configuration -------
const char* ssid = "Arturo’s iPhone";
const char* password = "12345678";
const char* mqtt_broker = "172.20.10.4"; 
const int mqtt_port = 1883;

const char* seat_topic = "sensors/seat/data";
const char* back_topic = "sensors/back/data";
const char* movement_topic = "sensors/movement/data";

WiFiClient espClient;
PubSubClient client(espClient);
const unsigned long MQTT_PUBLISH_INTERVAL = 2000UL; // Send MQTT messages every 2 seconds



// -------- RAW Data to Send to MQTT Broker --------
alertState seatState     = OK_STATE;
alertState backState     = OK_STATE;
alertState movementState = OK_STATE;

int seat_FL_val = 0;
int seat_FR_val = 0;
int seat_BL_val = 0;
int seat_BR_val = 0;
int back_UP_val = 0;
int back_LOW_val = 0;
bool pir_val = false;


// ----------------------------------------------------


/**
 * @brief Sets up the WiFi connection.
 */
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n¡WiFi Connected!");
  Serial.print("IP of the ESP32: ");
  Serial.println(WiFi.localIP());
}

/**
 * @brief Connects (or reconnects) to the MQTT Broker.
 */
void reconnect() {
  while (!client.connected()) {
    Serial.print("Trying MQTT connection...");
    if (client.connect("ESP32_Chair_Client")) {
      Serial.println("Connected to Broker!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Trying again in 5 seconds...");
      delay(5000);
    }
  }
}




/**
 * @brief Checks if two FSR sensor values are balanced.
 * 
 * Two sensors are balanced if the difference between their values is within a specified tolerance percentage of their average value.
 * @param value1 First FSR sensor value.
 * @param value2 Second FSR sensor value.
 * @return true if the values are balanced, false otherwise.
 */
bool areFSRBalanced(int value1, int value2) {

  if (value1 < FSR_THRESHOLD || value2 < FSR_THRESHOLD)
    return false;

  int average = (value1 + value2) / 2;

  if (average < FSR_THRESHOLD) {
    return false;
  }

  int difference = abs(value1 - value2);
  int allowedDifference = average * FSR_BALANCE_TOLERANCE / 100.00;

  return difference <= allowedDifference;
}


/**
 * @brief Evaluates the posture of the seat based on the readings from four FSR sensors.
 * 
 * The function checks:
 * - FL (Front Left) and FR (Front Right) sensors for balance.
 * - BL (Back Left) and BR (Back Right) sensors for balance.
 * - Front and Back sensors are not compared with each other, as they naturally have different pressure distributions.
 * 
 * @return true if both front and back sensors are balanced, false otherwise.
 */
bool isSeatPostureOK() {
  seat_BL_val = analogRead(SEAT_FSR_BL);
  seat_BR_val = analogRead(SEAT_FSR_BR);
  seat_FL_val = analogRead(SEAT_FSR_FL);
  seat_FR_val = analogRead(SEAT_FSR_FR);

  bool frontBalanced = areFSRBalanced(seat_FL_val, seat_FR_val);
  bool backBalanced  = areFSRBalanced(seat_BL_val, seat_BR_val);

  bool seatBalanced = frontBalanced && backBalanced;

  #ifdef DEBUGGING

    Serial.println("Seat FSR values:");
    Serial.print("\t- Front Left: ");
    Serial.println(seat_FL_val);
    Serial.print("\t- Front Right: ");
    Serial.println(seat_FR_val);
    Serial.print("\t- Back Left: ");
    Serial.println(seat_BL_val);
    Serial.print("\t- Back Right: ");
    Serial.println(seat_BR_val);

    Serial.println("Seat FSR balance check:");
    Serial.print("\t- Front balanced: ");
    Serial.println(frontBalanced ? "TRUE" : "FALSE");
    Serial.print("\t- Back balanced: ");
    Serial.println(backBalanced ? "TRUE" : "FALSE");

    Serial.print("\t- Overall seat balanced: ");
    Serial.println(seatBalanced ? "TRUE" : "FALSE");

  #endif 

  return seatBalanced;
}

/**
 * @brief Evaluates the posture of the backrest based on the readings from two FSR sensors.
 * 
 * Logic:
 * - The upper and lower backrest sensors must both detect pressure to consider the backrest posture balanced.
 * 
 * @return true if both backrest sensors are pressed, false otherwise.
 */
bool isBackPostureOK() {
  back_UP_val = analogRead(BACK_FSR_UP);
  back_LOW_val = analogRead(BACK_FSR_LOW);

  bool LOW_pressed = back_LOW_val > FSR_THRESHOLD;
  bool UP_pressed = back_UP_val > FSR_THRESHOLD;

  bool back_pressed = LOW_pressed && UP_pressed;

  #ifdef DEBUGGING

    Serial.println("Backrest FSR values:");
    Serial.print("\t- Back Upper: ");
    Serial.println(back_UP_val);
    Serial.print("\t- Back Lower: ");
    Serial.println(back_LOW_val);

    Serial.print("\t- Back pressed: ");
    Serial.println(back_pressed ? "TRUE" : "FALSE");

  #endif 

  return back_pressed;
}

/**
 * @brief Evaluates the movement status based on the PIR sensor reading.
 * 
 * Logic:
 * - If the PIR sensor detects movement (nervous state), it returns false, indicating that the movement is not OK.
 * - If the PIR sensor does not detect movement (calm state), it returns true, indicating that the movement is OK.
 * 
 * @return true if the PIR sensor does not detect movement (calm state), false otherwise (nervous state).
 */
bool isMovementOK() {
  pir_val = digitalRead(PIR_PIN);
  bool movementDetected = pir_val == HIGH;

  #ifdef DEBUGGING

    Serial.print("Calm state (no movement detected): ");
    Serial.println(movementDetected ? "FALSE" : "TRUE");
  #endif

  return !movementDetected;
}


/**
 * @brief Updates the alert state based on the current condition and the time it has been active.
 * 
 * @param isOK True if the condition is OK, false if it is not OK.
 * @param startWarningTime Reference to the variable that stores the time when the warning started.
 * @param currentTime The current time in milliseconds, typically obtained from millis().
 * 
 * @note It is really important to pass the startWarningTime variable by reference, as it needs to be updated.
 * 
 * @see alertState
 * 
 * @return The updated alert state (OK_STATE, WARNING_STATE, or ALERT_STATE).
 */
alertState updateAlertState(bool isOK, unsigned long &startWarningTime, unsigned long currentTime) {
  if (isOK) {
    startWarningTime = 0;
    return OK_STATE;
  } else {
    if (startWarningTime == 0) {  
      startWarningTime = currentTime; // The warning has just started, we record the start time
    }
    
    if (currentTime - startWarningTime >= MIN_WARNING_DURATION) {
      return ALERT_STATE;
    } else {
      return WARNING_STATE;
    }
  }
}


String alertStateToString(alertState state) {
  switch (state) {
    case OK_STATE:
      return "OK";
    case WARNING_STATE:
      return "WARNING";
    case ALERT_STATE:
      return "ALERT";
    default:
      return "UNKNOWN STATE";
  }
}



/**
 * @brief Detects and counts the number of alerts based on:
 * - Seat posture (Seat FSR)
 * - Backrest posture (Back FSR)
 * - Movement status (PIR sensor)
 * 
 * @return int Number of alerts detected.
 */
int countAlerts() {
  unsigned long currentTime = millis();
  int totalAlerts = 0;

  backState     = updateAlertState(isBackPostureOK(), startBackWarningTime,      currentTime);
  seatState     = updateAlertState(isSeatPostureOK(), startSeatWarningTime,      currentTime);
  movementState = updateAlertState(isMovementOK(),    startMovementWarningTime,  currentTime);

  if (backState == ALERT_STATE)     totalAlerts++;
  if (seatState == ALERT_STATE)     totalAlerts++;
  if (movementState == ALERT_STATE) totalAlerts++;

  #ifdef DEBUGGING
    Serial.println("Current alert states: ");
    Serial.print("\t- Backrest: ");
    Serial.println(alertStateToString(backState));
    Serial.print("\t- Seat: ");
    Serial.println(alertStateToString(seatState));
    Serial.print("\t- Movement: ");
    Serial.println(alertStateToString(movementState));
    Serial.print("\t- Total valid alerts: ");
    Serial.println(totalAlerts);
  #endif

  return totalAlerts;
}



/**
 * @brief It generates a buzzer sound based on the number of active alerts.
 * 
 * @param alertas Number of active alerts (0, 1, 2, or 3).
 */
void processNumberOfAlerts(int numAlerts) {
  unsigned long currentTime = millis();
  
  // The previous state of the alerts is stored in a static variable to maintain its value across function calls.
  static int previousAlerts = -1;
  static unsigned long lastBuzzerChange = 0;
  static bool buzzerSounding = false;

  // If the number of alerts has changed, we reset the buzzer state and timing.
  if (numAlerts != previousAlerts) {
    previousAlerts = numAlerts;
    lastBuzzerChange = currentTime;
  }

  if (numAlerts <= 0) {
    noTone(BUZZER_PIN);
    buzzerSounding = false;
    return;
  }

  unsigned int frequency = 0;
  unsigned long noteDuration = 0;
  unsigned long silenceDuration = 0;

  switch (numAlerts) {
    case 1:
      frequency = 1000;
      noteDuration = 150;
      silenceDuration = 3000;
      break;

    case 2:
      frequency = 1500;
      noteDuration = 250;
      silenceDuration = 100; 
      break;

    case 3:
      frequency = ((currentTime / 150) % 2 == 0) ? 1000 : 1500;
      noteDuration = 150;
      silenceDuration = 0;
      break;
  }

  if (silenceDuration == 0) {
    buzzerSounding = true;
    tone(BUZZER_PIN, frequency);
    return;
  }

  unsigned long configuredInterval = buzzerSounding ? noteDuration : silenceDuration;

  if (currentTime - lastBuzzerChange >= configuredInterval) {
    lastBuzzerChange = currentTime; // Guardamos el momento exacto del cambio
    buzzerSounding = !buzzerSounding;

    if (buzzerSounding) {
      tone(BUZZER_PIN, frequency);
    } else {
      noTone(BUZZER_PIN);
    }
  }
}


/**
 * @brief Publishes the current alert states to the MQTT broker.
 */
void publishData() {

  static unsigned long lastPublishTime = 0;

  if (millis() - lastPublishTime >= MQTT_PUBLISH_INTERVAL) {
    lastPublishTime = millis();

    char jsonBuffer[256];

    // Seat Data
    StaticJsonDocument<200> docSeat;
    docSeat["state"] = alertStateToString(seatState);
    docSeat["fl"] = seat_FL_val;
    docSeat["fr"] = seat_FR_val;
    docSeat["bl"] = seat_BL_val;
    docSeat["br"] = seat_BR_val;
    serializeJson(docSeat, jsonBuffer);
    client.publish(seat_topic, jsonBuffer);

    // Backrest Data
    StaticJsonDocument<150> docBack;
    docBack["state"] = alertStateToString(backState);
    docBack["up"] = back_UP_val;
    docBack["low"] = back_LOW_val;
    serializeJson(docBack, jsonBuffer);
    client.publish(back_topic, jsonBuffer);

    // Movement Data
    StaticJsonDocument<100> docMove;
    docMove["state"] = alertStateToString(movementState);
    docMove["movement_detected"] = pir_val;
    serializeJson(docMove, jsonBuffer);
    client.publish(movement_topic, jsonBuffer);
  }
}





void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);

  setup_wifi();
  client.setServer(mqtt_broker, mqtt_port);


  Serial.println("Waiting for sensors to stabilize...");
  delay(WARMUP_TIME);
  Serial.println("Smart ChAIr Logic Started");
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();



  int numAlerts = countAlerts();
  processNumberOfAlerts(numAlerts);
  publishData();
  #ifdef DEBUGGING
    Serial.println("===============================");
  #endif

  delay(10);
}
