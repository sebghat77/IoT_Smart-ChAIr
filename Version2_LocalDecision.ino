// ==================================================
// Smart Chair - FSR Logic
// Two independent decision functions:
// 1. Seat posture decision
// 2. Backrest posture decision
// ==================================================


// ---------- Seat FSR Sensors ----------
const int seatFSR1 = 34;   // Front Left
const int seatFSR2 = 35;   // Front Right
const int seatFSR3 = 32;   // Back Left
const int seatFSR4 = 33;   // Back Right


// ---------- Backrest FSR Sensors ----------
const int backFSR1 = 25;   // Backrest Left / Upper
const int backFSR2 = 26;   // Backrest Right / Lower


// ---------- Configuration ----------
const int pressureThreshold = 500;       // Minimum ADC value to count as pressure
const int balanceTolerancePercent = 20;  // Allowed difference between paired sensors


// ==================================================
// Helper Function
// Checks whether two FSR values are balanced.
// Example:
// If tolerance is 20%, the difference between two sensors
// must be less than or equal to 20% of their average.
// ==================================================
bool areBalanced(int value1, int value2) {
  int average = (value1 + value2) / 2;

  // If both values are too low, we do not consider them valid pressure.
  if (average < pressureThreshold) {
    return false;
  }

  int difference = abs(value1 - value2);
  int allowedDifference = average * balanceTolerancePercent / 100;

  if (difference <= allowedDifference) {
    return true;
  } else {
    return false;
  }
}


// ==================================================
// Function 1: Seat Posture Decision
//
// Seat layout:
// seatFSR1 = Front Left
// seatFSR2 = Front Right
// seatFSR3 = Back Left
// seatFSR4 = Back Right
//
// Logic:
// - Front Left should be balanced with Front Right
// - Back Left should be balanced with Back Right
// - Front sensors are NOT compared with back sensors,
//   because thighs and hips naturally apply different pressure.
// ==================================================
bool evaluateSeatPosture() {
  int s1 = analogRead(seatFSR1);
  int s2 = analogRead(seatFSR2);
  int s3 = analogRead(seatFSR3);
  int s4 = analogRead(seatFSR4);

  bool frontBalanced = areBalanced(s1, s2);
  bool backBalanced  = areBalanced(s3, s4);

  Serial.print("Seat FSR values: [");
  Serial.print(s1); Serial.print(", ");
  Serial.print(s2); Serial.print(", ");
  Serial.print(s3); Serial.print(", ");
  Serial.print(s4); Serial.print("]");

  Serial.print(" | Front balanced: ");
  Serial.print(frontBalanced ? "TRUE" : "FALSE");

  Serial.print(" | Back balanced: ");
  Serial.print(backBalanced ? "TRUE" : "FALSE");

  if (frontBalanced && backBalanced) {
    Serial.println(" | Seat decision: TRUE");
    return true;
  } else {
    Serial.println(" | Seat decision: FALSE");
    return false;
  }
}


// ==================================================
// Function 2: Backrest Posture Decision
//
// Logic:
// - Both backrest FSR sensors must detect pressure.
// - If one sensor is below the threshold, the user is probably
//   not leaning correctly on the backrest.
// ==================================================
bool evaluateBackrestPosture() {
  int b1 = analogRead(backFSR1);
  int b2 = analogRead(backFSR2);

  bool sensor1Pressed = b1 > pressureThreshold;
  bool sensor2Pressed = b2 > pressureThreshold;

  Serial.print("Backrest FSR values: [");
  Serial.print(b1); Serial.print(", ");
  Serial.print(b2); Serial.print("]");

  Serial.print(" | Sensor 1 pressed: ");
  Serial.print(sensor1Pressed ? "TRUE" : "FALSE");

  Serial.print(" | Sensor 2 pressed: ");
  Serial.print(sensor2Pressed ? "TRUE" : "FALSE");

  if (sensor1Pressed && sensor2Pressed) {
    Serial.println(" | Backrest decision: TRUE");
    return true;
  } else {
    Serial.println(" | Backrest decision: FALSE");
    return false;
  }
}


// ==================================================
// Example Final Decision Function
// This is only an example.
// Later, this function can be completed together with the team
// and connected to ToF, PIR, timing logic, and buzzer.
// ==================================================
void exampleFinalDecision() {
  bool seatOK = evaluateSeatPosture();
  bool backrestOK = evaluateBackrestPosture();

  if (seatOK && backrestOK) {
    Serial.println("Example final decision: GOOD POSTURE");
    // digitalWrite(buzzerPin, LOW);
  } else {
    Serial.println("Example final decision: BAD POSTURE / WARNING");
    // digitalWrite(buzzerPin, HIGH);
  }

  Serial.println("--------------------------------------");
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Smart Chair FSR Logic Started");
}


void loop() {
  exampleFinalDecision();

  delay(1000);
}
