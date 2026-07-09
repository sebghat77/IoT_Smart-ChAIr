#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <vl53l4cd_class.h>

#define DEBUGGING   // Comment when not debugging


// ---------- Seat FSR Sensors ----------
#define SEAT_FSR_FL 34
#define SEAT_FSR_FR 35
#define SEAT_FSR_BL 32
#define SEAT_FSR_BR 33

// ---------- Backrest FSR Sensors ----------
#define BACK_FSR_UP 36
#define BACK_FSR_LOW 39

// ---------- FSR Configuration ----------
// TODO: These values need to be calibrated when the sensors are installed in their final positions.
#define FSR_THRESHOLD 0             // Minimum ADC value to count as pressure
#define FSR_BALANCE_TOLERANCE 20    // Allowed difference between paired sensors in percentage

// --------- PIR Sensor ----------
// DONE: Its FoV needs to be calibrated when the sensor is installed in its final position.
#define PIR_PIN 13
#define WARMUP_TIME 15000UL // Time in milliseconds for sensors to stabilize after powering up

// --------- Buzzer ----------
#define BUZZER_PIN 18

// --------- ToF Sensors ----------
#define SCL_PIN 22
#define SDA_PIN 21
#define XSHUT_SENSOR_UP 17
#define XSHUT_SENSOR_LOW 16

#define NO_OBJECT -1
#define OBJECT_TOO_CLOSE -2

// TODO: these thresholds needs to be calibrated when the sensors are installed in their final positions.
#define TOO_CLOSE_THRESHOLD 23  // Minimum distance in mm to consider an object "too close" to the sensor
#define ToF_BALANCE_TOLERANCE 15 // Allowed difference between the two ToF sensors in percentage

// Initialize with the Wire instance. The library handles XSHUT if passed, 
// but doing it manually in setup() is much safer for multi-sensor setups.
VL53L4CD UP_ToF(&Wire, -1); 
VL53L4CD LOW_ToF(&Wire, -1);





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
unsigned long startDistanceWarningTime  = 0;    // Time when the distance warning started


// ------- MQTT Configuration -------
const char* ssid = "Arturo’s iPhone";
const char* password = "X";
const char* mqtt_broker = "172.20.10.4"; 
const int mqtt_port = 1883;

const char* seat_topic = "sensors/seat/data";
const char* back_topic = "sensors/back/data";
const char* movement_topic = "sensors/movement/data";
const char* distance_topic = "sensors/distance/data";

WiFiClient espClient;
PubSubClient client(espClient);
const unsigned long MQTT_PUBLISH_INTERVAL = 2000UL; // Send MQTT messages every 2 seconds



// -------- RAW Data to Send to MQTT Broker --------
alertState seatState     = OK_STATE;
alertState backState     = OK_STATE;
alertState movementState = OK_STATE;
alertState distanceState = OK_STATE;

int seat_FL_val = 0;
int seat_FR_val = 0;
int seat_BL_val = 0;
int seat_BR_val = 0;
int back_UP_val = 0;
int back_LOW_val = 0;
float distance_UP_val = 0.0;    // Distance in centimeters
float distance_LOW_val = 0.0;   // Distance in centimeters
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
 * @brief Scans the I2C bus for connected devices and prints their addresses to the Serial Monitor.
 */
void scanI2C() {
  Serial.println("--- Scanning I2C bus ---");
  byte found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
      found++;
    }
  }
  if (found == 0) Serial.println("No I2C devices found!");
  Serial.println("--- Scan done ---");
}


#define DEBUGGING_SETUP_ToF
/**
 * @brief Initializes the Time-of-Flight (ToF) sensors.
 */
void setup_ToF() {
  
  Wire.begin(SDA_PIN, SCL_PIN);


  pinMode(XSHUT_SENSOR_UP, OUTPUT);
  pinMode(XSHUT_SENSOR_LOW, OUTPUT);


  digitalWrite(XSHUT_SENSOR_UP, LOW);
  digitalWrite(XSHUT_SENSOR_LOW, LOW);
  delay(100);

  // -------------- Sensor UP
  digitalWrite(XSHUT_SENSOR_UP, HIGH);
  delay(10);


  UP_ToF.begin();
  
  // Physical hardware. 8-bit address 0x60 (which is 7-bit 0x30)
  UP_ToF.VL53L4CD_SetI2CAddress(0x60); 
  
  // C++ address. 7-bit address 0x30
  UP_ToF.InitSensor(0x30); 
  
  UP_ToF.VL53L4CD_SetRangeTiming(200, 0);
  UP_ToF.VL53L4CD_StartRanging();

  #ifdef DEBUGGING_SETUP_ToF
    Serial.println("Sensor UP initialized at I2C address 0x30. Checking...");
    scanI2C(); // Should show 0x30
  #endif

  // -------------- Sensor LOW
  digitalWrite(XSHUT_SENSOR_LOW, HIGH);
  delay(10);

  LOW_ToF.begin();

  // Physical hardware. 8-bit address 0x64 (which is 7-bit 0x32)
  LOW_ToF.VL53L4CD_SetI2CAddress(0x64); 
  
  // C++ address. 7-bit address 0x32
  LOW_ToF.InitSensor(0x32); 
  
  LOW_ToF.VL53L4CD_SetRangeTiming(200, 0);
  LOW_ToF.VL53L4CD_StartRanging();

  #ifdef DEBUGGING_SETUP_ToF
    Serial.println("Sensor LOW initialized at I2C address 0x32. Checking...");
    scanI2C(); // Should show 0x32
  #endif

  #ifdef DEBUGGING
    Serial.println("Both VL53L4CD sensors should be ready.");
  #endif
}


/**
 * @brief Checks if two float values are balanced within a given tolerance.
 * 
 * @param value1 First float value.
 * @param value2 Second float value.
 * @param tolerance The maximum allowed relative difference between the two values for them to be considered balanced.
 * The tolerance is expressed relative to the average of the two values.
 * 
 * @return true if the values are balanced, false otherwise.
 */
bool areBalanced(float value1, float value2, int tolerance){

  float average = (value1 + value2) / 2.0;
  float difference = abs(value1 - value2);
  float allowedDifference = average * tolerance / 100.0;
  return difference <= allowedDifference;
}


/**
 * @brief Checks if a given FSR sensor is pressed based on its value and a threshold.
 * 
 * @param value The current reading from the FSR sensor.
 * @param threshold The minimum value that indicates the sensor is pressed.
 * @return true if the sensor is pressed, false otherwise.
 */
bool isFSRPressed(int value, int threshold) {
  return value > threshold;
}


/**
 * @brief Evaluates the posture of the seat based on the readings from four FSR sensors.
 * 
 * The function checks:
 * - The four sensors are pressed (indicating a good contact with the seat).
 * - FL (Front Left) and FR (Front Right) sensors for balance.
 * - BL (Back Left) and BR (Back Right) sensors for balance.
 * - Front and Back sensors are not compared with each other, as they naturally have different pressure distributions.
 * 
 * However, if none of the sensors if pressed, it is considered that the person is not sitting on the chair,
 *    and the posture is considered OK.
 * 
 * 
 * @return true if both front and back sensors are balanced, false otherwise.
 */
bool isSeatPostureOK() {
  seat_BL_val = analogRead(SEAT_FSR_BL);
  seat_BR_val = analogRead(SEAT_FSR_BR);
  seat_FL_val = analogRead(SEAT_FSR_FL);
  seat_FR_val = analogRead(SEAT_FSR_FR);

  bool anyPressed =   isFSRPressed(seat_FL_val, FSR_THRESHOLD)
                  ||  isFSRPressed(seat_FR_val, FSR_THRESHOLD)
                  ||  isFSRPressed(seat_BL_val, FSR_THRESHOLD)
                  ||  isFSRPressed(seat_BR_val, FSR_THRESHOLD);

  if (!anyPressed) {
    #ifdef DEBUGGING
      Serial.println("No one is sitting on the chair. Seat posture is considered OK.");
    #endif
    return true;
  }


  bool allPressed = isFSRPressed(seat_FL_val, FSR_THRESHOLD)
                  && isFSRPressed(seat_FR_val, FSR_THRESHOLD)
                  && isFSRPressed(seat_BL_val, FSR_THRESHOLD)
                  && isFSRPressed(seat_BR_val, FSR_THRESHOLD);

  bool frontBalanced = areBalanced(seat_FL_val, seat_FR_val, FSR_BALANCE_TOLERANCE);
  bool backBalanced  = areBalanced(seat_BL_val, seat_BR_val, FSR_BALANCE_TOLERANCE);

  bool seatOK = allPressed && frontBalanced && backBalanced;

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

    Serial.println("Seat FSR check:");
    Serial.print("\t- Front balanced: ");
    Serial.println(frontBalanced ? "TRUE" : "FALSE");
    Serial.print("\t- Back balanced: ");
    Serial.println(backBalanced ? "TRUE" : "FALSE");
    Serial.print("\t- All pressed: ");
    Serial.println(allPressed ? "TRUE" : "FALSE");

    Serial.print("\t- Overall seat ok: ");
    Serial.println(seatOK ? "TRUE" : "FALSE");

  #endif 

  return seatOK;
}

/**
 * @brief Evaluates the posture of the backrest based on the readings from two FSR sensors.
 * 
 * A good posture is one of these two options:
 * - Both sensors are pressed (indicating a good contact with the backrest).
 * - None of the sensors is pressed (indicating that the person is not leaning on the backrest).
 * 
 * If only one of the sensors is pressed, it indicates that the person is leaning on the backrest in a bad way.
 * 
 * @return true if the person is leaning on the backrest in a good way, false otherwise.
 */
bool isBackPostureOK() {
  back_UP_val = analogRead(BACK_FSR_UP);
  back_LOW_val = analogRead(BACK_FSR_LOW);

  bool LOW_pressed  = isFSRPressed(back_LOW_val, FSR_THRESHOLD);
  bool UP_pressed   = isFSRPressed(back_UP_val, FSR_THRESHOLD);

  bool onlyOnePressed = (LOW_pressed && !UP_pressed) || (!LOW_pressed && UP_pressed);

  #ifdef DEBUGGING

    Serial.println("Backrest FSR values:");
    Serial.print("\t- Back Upper: ");
    Serial.println(back_UP_val);
    Serial.print("\t- Back Lower: ");
    Serial.println(back_LOW_val);

    Serial.print("\t- Bad posture (only one sensor pressed): ");
    Serial.println(onlyOnePressed ? "TRUE" : "FALSE");

  #endif 

  return !onlyOnePressed;
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
 * @brief Checks if the distance measurement from a VL53L4CD sensor is valid.
 * 
 * @param results The result structure from the VL53L4CD sensor
 * @see <vl53l4cd_class.h> for the definition of VL53L4CD_Result_t
 */
bool isDistanceMeasurementValid(const VL53L4CD_Result_t &results) {
  return results.range_status == 0; // 0 indicates a valid measurement
}

/**
 * @brief Checks if the distance measurements from both ToF sensors are valid and balanced.
 * 
 * @return true if the distance measurement is valid.
 */
bool isObjectDetected(const VL53L4CD_Result_t &results) {
  return isDistanceMeasurementValid(results);
}

/**
 * @brief Checks if an object is too close to the sensor based on the distance measurement.
 * 
 * @param float distance The distance measurement in millimeters.
 * @return true if the object is too close, false otherwise.
 */
bool isObjectTooClose(float distance) {
  return distance < TOO_CLOSE_THRESHOLD;
}


/**
 * @brief Retrieves the distance measurement from a VL53L4CD sensor in centimeters.
 * 
 * @param sensor The VL53L4CD sensor object.
 * @return The distance in centimeters if an object is detected and not too close
 * - OBJECT_TOO_CLOSE if the object is too close to the sensor
 * - NO_OBJECT if no object is detected
 * 
 * @see OBJECT_TOO_CLOSE and NO_OBJECT constants for special return values.
 */
float getDistanceInCm(VL53L4CD &sensor) {
  uint8_t dataReady = 0;
  VL53L4CD_Result_t results;

  sensor.VL53L4CD_CheckForDataReady(&dataReady);
  if (dataReady) {
    sensor.VL53L4CD_GetResult(&results);

    float distance_cm = 0.0;
    if (isObjectDetected(results)) {
      distance_cm = (float)results.distance_mm / 10.0; // Convert mm to cm
      if (isObjectTooClose(distance_cm)) distance_cm = OBJECT_TOO_CLOSE;
    }

    sensor.VL53L4CD_ClearInterrupt();
    return distance_cm;
  }
  else return NO_OBJECT;
}





/**
 * @brief Checks if the distance measurements from both ToF sensors should generate a warning or not
 * 
 * A warning is generated if:
 * - Both sensors detect an object (if no object is detected, no person is sitting in the chair, so no warning is needed)
 * - The object detected is not too close (if it is too close, the person is leaning on the backrest, FSR sensors will handle it)
 * - The distance measurements from both sensors are not balanced within the defined tolerance
 * 
 * @return true if the distance measurements are OK (no warning needed), false if a warning should be generated
 */
bool isDistanceOK() {
  uint8_t dataReady = 0;
  VL53L4CD_Result_t results;

  distance_UP_val = getDistanceInCm(UP_ToF);
  distance_LOW_val = getDistanceInCm(LOW_ToF);

  bool bothDetectObject = (distance_UP_val != NO_OBJECT) && (distance_LOW_val != NO_OBJECT);
  bool noObjectIsTooClose = (distance_UP_val != OBJECT_TOO_CLOSE) && (distance_LOW_val != OBJECT_TOO_CLOSE);
  bool distancesNotBalanced = !areBalanced(distance_UP_val, distance_LOW_val, ToF_BALANCE_TOLERANCE);

  bool generateDistanceWarning = bothDetectObject && noObjectIsTooClose && distancesNotBalanced;

  #ifdef DEBUGGING
    Serial.println("ToF Sensor values:");
    Serial.print("\t- Sensor UP: ");
    Serial.print(distance_UP_val);
    Serial.println(" cm");
    Serial.print("\t- Sensor LOW: ");
    Serial.print(distance_LOW_val);
    Serial.println(" cm");

    Serial.println("Distance check:");
    Serial.print("\t- Both detect object: ");
    Serial.println(bothDetectObject ? "TRUE" : "FALSE");
    Serial.print("\t- No object too close: ");
    Serial.println(noObjectIsTooClose ? "TRUE" : "FALSE");
    Serial.print("\t- Distances not balanced: ");
    Serial.println(distancesNotBalanced ? "TRUE" : "FALSE");
    Serial.print("\t- Generate distance warning: ");
    Serial.println(generateDistanceWarning ? "TRUE" : "FALSE");
  #endif


  return !generateDistanceWarning;
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


/**
 * @brief Converts an alertState enum value to its corresponding string representation.
 * @param state The alertState enum value to convert.
 * @return The string representation of the alertState.
 */
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
 * - Distance status (ToF sensors)
 * 
 * @return int Number of alerts detected.
 */
int countAlerts() {
  unsigned long currentTime = millis();
  int totalAlerts = 0;

  backState     = updateAlertState(isBackPostureOK(), startBackWarningTime,      currentTime);
  seatState     = updateAlertState(isSeatPostureOK(), startSeatWarningTime,      currentTime);
  movementState = updateAlertState(isMovementOK(),    startMovementWarningTime,  currentTime);
  distanceState = updateAlertState(isDistanceOK(),    startDistanceWarningTime,  currentTime);


  if (backState == ALERT_STATE)     totalAlerts++;
  if (seatState == ALERT_STATE)     totalAlerts++;
  if (movementState == ALERT_STATE) totalAlerts++;
  if (distanceState == ALERT_STATE) totalAlerts++;

  #ifdef DEBUGGING
    Serial.println("Current alert states: ");
    Serial.print("\t- Backrest: ");
    Serial.println(alertStateToString(backState));
    Serial.print("\t- Seat: ");
    Serial.println(alertStateToString(seatState));
    Serial.print("\t- Movement: ");
    Serial.println(alertStateToString(movementState));
    Serial.print("\t- Distance: ");
    Serial.println(alertStateToString(distanceState));
    Serial.print("\t- Total valid alerts: ");
    Serial.println(totalAlerts);
  #endif

  if (totalAlerts > 3){
    // Since distance & backrest can't be an alert at the same time, this should never happen.
    // If it does, it indicates a logical error in the alert detection system.
    Serial.println("Warning: More than 3 alerts detected. This should not happen.");
  }

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
      noteDuration = 250;
      silenceDuration = 3000;
      break;

    case 2:
      frequency = 1500;
      noteDuration = 250;
      silenceDuration = 300; 
      break;

    case 3:
      frequency = ((currentTime / 300) % 2 == 0) ? 1000 : 1500;
      noteDuration = 300;
      silenceDuration = 0;
      break;
    
    default:  // This case should never happen, but if it does, we will generate a warning sound.
      frequency = ((currentTime / 250) % 2 == 0) ? 1000 : 1500;
      noteDuration = 250;
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

    // Distance Data
    StaticJsonDocument<150> docDistance;
    docDistance["state"] = alertStateToString(distanceState);
    docDistance["up"] = distance_UP_val;
    docDistance["low"] = distance_LOW_val;
    serializeJson(docDistance, jsonBuffer);
    client.publish(distance_topic, jsonBuffer);
  }
}







void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  setup_wifi();
  client.setServer(mqtt_broker, mqtt_port);

  setup_ToF();


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

  delay(200);
}
