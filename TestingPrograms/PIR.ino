/* CONNECTIONS

The PIR sensor has four MALE pins: VCC, GND, OUT, and EN.
The connections with the ESP32 are as follows:
  VCC -> 5V
  GND -> GND
  OUT -> GPIO 13
  EN -> Disconnected. (If I connect it and set it to LOW, the sensor will be disabled)
*/

/*
I have encountered some issues with the PIR sensor.

1. Too wide Field of View (FOV) or detection range.
      The FoV of the PIR sensor is too wide, so it detects almost every movement in the room.
      I have thought some solutions to this problem:
      - Use tape (or cardboard or anything else) to cover the sensor partially, reducing the FoV and making it more directional.
      - I have lowered the sensitivity of the sensor to the minimum

      We will have to calibrate the sensor accordingly when it is installed in the final position.

2. Final position of the sensor.
      When we build the chair itself, we will have to find the best position for the sensor.
      It should be placed under the chair, but we will have to try different positions to find the best one.

3. When it detects movement, it keeps detecting it for 2/3 seconds, even if the movement has stopped.
      I however believe this is not a problem for us, but we should keep it in mind.
      
*/




#define PIR_PIN 13
#define PIR_WARMUP_TIME 15000UL // Time in milliseconds for the PIR sensor to stabilize after powering up

void setup() {
  Serial.begin(9600);       // Speed of the serial communication
  pinMode(PIR_PIN, INPUT);
  
  delay(PIR_WARMUP_TIME);
}

/**
 * @brief Checks if movement is detected by the PIR sensor.
 * 
 * @param pin The GPIO pin connected to the PIR sensor's OUT pin.
 * @return true if movement is detected (HIGH), false otherwise (LOW).
 */
bool movementDetected(int pin) {
  return digitalRead(pin) == HIGH;
}



void loop() {
  // Just a simple test to check if the PIR sensor is working properly.
  // In the real application where we combine the sensors, a more complex logic will be implemented.
  if (movementDetected(PIR_PIN)) {
    Serial.println("Movement Detected!");
  } else {
    Serial.println("-----");
  }
  
  delay(10);
}
