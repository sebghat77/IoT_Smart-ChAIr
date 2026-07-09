#include <Wire.h>
#include <vl53l4cd_class.h>

#define SCL_PIN 22
#define SDA_PIN 21
#define XSHUT_SENSOR1 17
#define XSHUT_SENSOR2 16
#define BUZZER_PIN 19
#define DEBUGGING

// Tolerance for ToF comparison 
const int TOLERANCE_MM = 100; // Define what "approximately the same" means (e.g., within 100mm)

// Alarm delay (connect with A&S's code)
const unsigned long ALARM_DELAY_MS = 5000; // 5 seconds in milliseconds

// Variables to track sensor states
uint16_t dist1 = 0, dist2 = 0;
bool valid1 = false, valid2 = false;

// Variables for the stopwatch
unsigned long mismatchStartTime = 0;
bool mismatchActive = false;

// Initialize with the Wire instance. The library handles XSHUT if passed, 
// but doing it manually in setup() is much safer for multi-sensor setups.
VL53L4CD sensor1(&Wire, -1); 
VL53L4CD sensor2(&Wire, -1);

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
void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial monitor time to attach
  
  // Setup Buzzer: 
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure it starts quiet


  // Setup ToF: 
  Wire.begin(SDA_PIN, SCL_PIN);

  // Configure XSHUT pins as outputs
  pinMode(XSHUT_SENSOR1, OUTPUT);
  pinMode(XSHUT_SENSOR2, OUTPUT);

  // STEP 1: Put both sensors in reset state
  digitalWrite(XSHUT_SENSOR1, LOW);
  digitalWrite(XSHUT_SENSOR2, LOW);
  delay(100);

  // ==========================================
  // STEP 2: Initialize Sensor 1
  // ==========================================
  Serial.println("=== Initializing Sensor 1 ===");
  digitalWrite(XSHUT_SENSOR1, HIGH); // Wake up Sensor 1
  delay(10); // Wait for boot

  sensor1.begin(); // Establishes internal tracking at the default 0x29 address
  
  // 1. Tell physical hardware to use 8-bit address 0x60 (which is 7-bit 0x30)
  sensor1.VL53L4CD_SetI2CAddress(0x60); 
  
  // 2. Tell the C++ object to look for 7-bit address 0x30
  sensor1.InitSensor(0x30); 
  
  sensor1.VL53L4CD_SetRangeTiming(200, 0);
  sensor1.VL53L4CD_StartRanging();

  scanI2C(); // Should show 0x30

  // ==========================================
  // STEP 3: Initialize Sensor 2
  // ==========================================
  Serial.println("=== Initializing Sensor 2 ===");
  digitalWrite(XSHUT_SENSOR2, HIGH); // Wake up Sensor 2 (Sensor 1 is safely ignoring 0x29 now)
  delay(10);

  sensor2.begin(); // Establishes internal tracking at default 0x29
  
  // 1. Tell physical hardware to use 8-bit address 0x64 (which is 7-bit 0x32
  sensor2.VL53L4CD_SetI2CAddress(0x64); 
  
  // 2. Tell the C++ object to look for 7-bit address 0x32
  sensor2.InitSensor(0x32); 
  
  sensor2.VL53L4CD_SetRangeTiming(200, 0);
  sensor2.VL53L4CD_StartRanging();

  scanI2C(); // Should show BOTH 0x30 and 0x32

  Serial.println("Both VL53L4CD sensors ready.");
}

void loop() {
  uint8_t dataReady = 0;
  VL53L4CD_Result_t results;

  // --- 1. Update Sensor 1 Data ---
  sensor1.VL53L4CD_CheckForDataReady(&dataReady);
  if (dataReady) {
    sensor1.VL53L4CD_GetResult(&results);
    valid1 = (results.range_status == 0);
    if (valid1) dist1 = results.distance_mm;
    sensor1.VL53L4CD_ClearInterrupt();
  }

  // --- 2. Update Sensor 2 Data ---
  dataReady = 0;
  sensor2.VL53L4CD_CheckForDataReady(&dataReady);
  if (dataReady) {
    sensor2.VL53L4CD_GetResult(&results);
    valid2 = (results.range_status == 0);
    if (valid2) dist2 = results.distance_mm;
    sensor2.VL53L4CD_ClearInterrupt();
  }

  // --- 3. Evaluate the Logic ---
  
  // Rule A: Do neither sense anything? 
  bool neitherSenses = !valid1 && !valid2;
  
  // Rule B: Do both sense an object, and is the distance gap smaller than our tolerance?
  // Note: We cast to (long) to prevent math errors when subtracting unsigned variables
  bool bothSenseApproxSame = valid1 && valid2 && (abs((long)dist1 - (long)dist2) <= TOLERANCE_MM);

  if (neitherSenses || bothSenseApproxSame) {
    // Condition is NORMAL. Reset the stopwatch and silence the buzzer.
    mismatchActive = false;
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    // Condition is ABNORMAL. (One senses and one doesn't, OR they both sense but distances are vastly different).
    if (!mismatchActive) {
      // Start the stopwatch!
      mismatchActive = true;
      mismatchStartTime = millis();
      Serial.println("Mismatch detected! Starting 5-second timer...");
    } else {
      // The stopwatch is running. Has it been 5 seconds?
      if (millis() - mismatchStartTime >= ALARM_DELAY_MS) {
        digitalWrite(BUZZER_PIN, HIGH); // Sound the alarm!
      }
    }
  }

  // --- 4. Debug Output ---
  #ifdef DEBUGGING
    static unsigned long lastDebugTime = 0;
    
    // Only print debug info every 500ms so we don't flood the serial monitor
    if (millis() - lastDebugTime > 500) { 
      lastDebugTime = millis();
      
      Serial.println("ToF Sensor values:");
      // Using a quick inline if/else to print the distance OR an "invalid" warning
      Serial.println("\t- Sensor 1: " + (valid1 ? String(dist1) + " mm" : "INVALID/NO TARGET"));
      Serial.println("\t- Sensor 2: " + (valid2 ? String(dist2) + " mm" : "INVALID/NO TARGET"));

      Serial.println("Logic state check:");
      Serial.print("\t- Sensor 1 Active: ");
      Serial.println(valid1 ? "TRUE" : "FALSE");
      
      Serial.print("\t- Sensor 2 Active: ");
      Serial.println(valid2 ? "TRUE" : "FALSE");

      Serial.print("\t- Distances Approximate Match: ");
      Serial.println(bothSenseApproxSame ? "TRUE" : "FALSE");
      
      Serial.print("\t- Alarm Timer Running: ");
      Serial.println(mismatchActive ? "TRUE" : "FALSE");
      
      Serial.println("---------------------------------"); // Separator for readability
    }
  #endif


  delay (50);
}
