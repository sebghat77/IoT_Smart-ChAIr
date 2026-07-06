#define DEBUGGING   // Uncomment when not debugging


// ---------- Seat FSR Sensors ----------
#define SEAT_FSR_FL 34   // Front Left
#define SEAT_FSR_FR 35   // Front Right
#define SEAT_FSR_BL 32   // Back Left
#define SEAT_FSR_BR 33   // Back Right

// ---------- Backrest FSR Sensors ----------
#define BACK_FSR_UP 25   // Backrest Upper
#define BACK_FSR_LOW 26   // Backrest Lower

// ---------- FSR Configuration ----------
// TODO: These values need to be calibrated when the sensors are installed in their final positions.
#define FSR_THRESHOLD 500   // Minimum ADC value to count as pressure
#define FSR_BALANCE_TOLERANCE 20  // Allowed difference between paired sensors in percentage

// --------- PIR Sensor ----------
// TODO: Its FoV needs to be calibrated when the sensor is installed in its final position.
#define PIR_PIN 13
#define WARMUP_TIME 15000UL // Time in milliseconds for sensors to stabilize after powering up

// --------- Buzzer ----------
// TODO: I do not know if that pin works, it needs to be tested
#define BUZZER_PIN 18


// --------- General Alert System ----------
#define MIN_ALERT_DURATION 60*1000UL // Minimum duration in milliseconds for an alert to be considered valid
unsigned long startSeatAlertTime      = 0;    // Time when the seat alert started
unsigned long startBackAlertTime      = 0;    // Time when the backrest alert started
unsigned long startMovementAlertTime  = 0;    // Time when the movement alert started

unsigned long lastBuzzerChange = 0;
bool buzzerSounding = false;


/**
 * @brief Checks if two FSR sensor values are balanced.
 * 
 * Two sensors are balanced if the difference between their values is within a specified tolerance percentage of their average value.
 * @param value1 First FSR sensor value.
 * @param value2 Second FSR sensor value.
 * @return true if the values are balanced, false otherwise.
 */
bool areFSRBalanced(int value1, int value2) {
  int average = (value1 + value2) / 2;

  // If at least one of the values is too low, we do not consider them valid pressure.
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
  int seat_BL_reading = analogRead(SEAT_FSR_BL);
  int seat_BR_reading = analogRead(SEAT_FSR_BR);
  int seat_FL_reading = analogRead(SEAT_FSR_FL);
  int seat_FR_reading = analogRead(SEAT_FSR_FR);

  bool frontBalanced = areFSRBalanced(seat_FL_reading, seat_FR_reading);
  bool backBalanced  = areFSRBalanced(seat_BL_reading, seat_BR_reading);

  bool seatBalanced = frontBalanced && backBalanced;

  #ifdef DEBUGGING

    Serial.println("Seat FSR values:");
    Serial.println("\t- Front Left: " + String(seat_FL_reading));
    Serial.println("\t- Front Right: " + String(seat_FR_reading));
    Serial.println("\t- Back Left: " + String(seat_BL_reading));
    Serial.println("\t- Back Right: " + String(seat_BR_reading));

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
  int back_UP_reading = analogRead(BACK_FSR_UP);
  int back_LOW_reading = analogRead(BACK_FSR_LOW);

  bool LOW_pressed = back_LOW_reading > FSR_THRESHOLD;
  bool UP_pressed = back_UP_reading > FSR_THRESHOLD;

  bool back_pressed = LOW_pressed && UP_pressed;

  #ifdef DEBUGGING

    Serial.println("Backrest FSR values:");
    Serial.println("\t- Back Upper: " + String(back_UP_reading));
    Serial.println("\t- Back Lower: " + String(back_LOW_reading));

    Serial.print("\t- Back balanced: ");
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
  bool movementDetected = digitalRead(PIR_PIN) == HIGH;

  #ifdef DEBUGGING

    Serial.print("Calm state (no movement detected): ");
    Serial.println(movementDetected ? "FALSE" : "TRUE");
  #endif

  return !movementDetected;
}

/**
 * @brief Detects if an alert is valid.
 * 
 * It is valid if the condition has been continuously in a non-OK state for more than the minimum alert duration.
 * 
 * @param isOK True if the condition is OK, false if it is not OK.
 * @param startAlertTime Reference to the variable that stores the time when the alert started.
 * @param currentTime The current time in milliseconds, typically obtained from millis().
 * 
 * @note It is really important to pass the startAlertTime variable by reference, as it needs to be updated.
 * 
 * @return true if the alert has been continuously active for more than the minimum duration.
 */
bool isAlertValid(bool isOK, unsigned long &startAlertTime, unsigned long currentTime) {
  if (!isOK) {
    if (startAlertTime == 0) {  // The alert has just started
      startAlertTime = currentTime;
    }

    return (currentTime - startAlertTime >= MIN_ALERT_DURATION);

  } else {
    startAlertTime = 0;
    return false;
  }
}


/**
 * @brief Detects and counts the number of valid alerts based on:
 * - Seat posture (Seat FSR)
 * - Backrest posture (Back FSR)
 * - Movement status (PIR sensor)
 * 
 * @return int Number of valid alerts detected.
 */
int countValidAlerts() {
  unsigned long currentTime = millis();
  int totalAlerts = 0;

  if (isAlertValid(isSeatPostureOK(), startSeatAlertTime,      currentTime))   totalAlerts++;
  if (isAlertValid(isBackPostureOK(), startBackAlertTime,      currentTime))   totalAlerts++;
  if (isAlertValid(isMovementOK(),    startMovementAlertTime,  currentTime))   totalAlerts++;

  #ifdef DEBUGGING
    Serial.print("Total valid alerts detected: ");
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

  if (numAlerts <= 0) {
    noTone(BUZZZER_PIN);
    buzzerSounding = false;
    return;
  }

  unsigned int frequency = 0;
  unsigned long noteDuration = 0;
  unsigned long silenceDuration = 0;

  switch (numAlerts) {
    case 1:
      frequency = 1500;
      noteDuration = 150;
      silenceDuration = 1000;
      break;

    case 2:
      frequency = 2400;
      noteDuration = 250;
      silenceDuration = 250;
      break;

    case 3:
      // Maximum alert: continuous sound with alternating frequencies
      frequency = ((currentTime / 150) % 2 == 0) ? 3000 : 2000;
      noteDuration = 150;
      silenceDuration = 0;
      break;
  }

  
  // Before changing the buzzer state, the previous state must finish its duration.
  unsigned long configuredInterval = buzzerSounding ? noteDuration : silenceDuration;

  if (currentTime - lastBuzzerChange >= configuredInterval) { // Change can happen
    lastBuzzerChange = currentTime;
    buzzerSounding = !buzzerSounding;

    if (buzzerSounding || silenceDuration == 0) {
      tone(BUZZZER_PIN, frequency);
    } else {
      noTone(BUZZZER_PIN);
    }
  }
}




void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIR_PIN, INPUT);


  Serial.println("Waiting for sensors to stabilize...");
  delay(WARMUP_TIME);
  Serial.println("Smart Chair FSR Logic Started");
}


void loop() {
  int validAlerts = countValidAlerts();

  delay(1000);
}




