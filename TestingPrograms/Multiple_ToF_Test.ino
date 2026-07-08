#include <Wire.h>
#include <vl53l4cd_class.h>

#define SCL_PIN 22
#define SDA_PIN 21
#define XSHUT_SENSOR1 25
#define XSHUT_SENSOR2 26

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

  // Read Sensor 1
  sensor1.VL53L4CD_CheckForDataReady(&dataReady);
  if (dataReady) {
    sensor1.VL53L4CD_GetResult(&results);
    if (results.range_status == 0) {
      Serial.print("Sensor 1: ");
      Serial.print(results.distance_mm);
      Serial.println(" mm");
    }
    sensor1.VL53L4CD_ClearInterrupt();
  }

  // Read Sensor 2
  dataReady = 0;
  sensor2.VL53L4CD_CheckForDataReady(&dataReady);
  if (dataReady) {
    sensor2.VL53L4CD_GetResult(&results);
    if (results.range_status == 0) {
      Serial.print("Sensor 2: ");
      Serial.print(results.distance_mm);
      Serial.println(" mm");
    }
    sensor2.VL53L4CD_ClearInterrupt();
  }

  delay(500);
}