# IoT\_Smart-ChAIr

## The link to the common folder is [here](https://drive.google.com/drive/folders/1vy685r2IYYLLrJJtexPPP0kG4qTNNphn?usp=sharing).

### Sebghat Yarzada Update on 05.07.2026

# FSR Sensor Implementation

## ESP32 Pin Assignment

For the current ESP32 board, the following GPIO pins have been reserved for the six FSR sensors:

| Sensor | GPIO Pin |
|--------|----------|
| Seat Front Left | GPIO34 |
| Seat Front Right | GPIO35 |
| Seat Back Left | GPIO32 |
| Seat Back Right | GPIO33 |
| Backrest Left | GPIO25 |
| Backrest Right | GPIO26 |

These pins are used throughout the FSR implementation.

---

## FSR Sensor Development

I implemented and tested the FSR pressure sensor system for the Smart Chair.

The first step was to connect and test a single FSR sensor with the ESP32 using a voltage divider circuit. During testing, we faced a practical issue: although the sensor worked correctly, the very small FSR terminals made it difficult to establish a stable connection using jumper wires. The wires were easily disconnected while testing, which made the process inconvenient. For the final Smart Chair implementation, each FSR should be soldered to wires to ensure a secure and reliable electrical connection when mounted on the chair.

After successfully testing one sensor, all six FSR sensors were connected and verified individually.

The ESP32 reads the output of each FSR sensor as an **ADC value (0–4095)**.

Although it is technically possible to convert these ADC values into an estimated physical pressure (for example, in **kPa**) by performing a proper calibration procedure, this is **not required for our current project**.

Our objective is not to measure the user's exact body pressure. Instead, we only need to compare the pressure distribution between different sensors in order to evaluate the sitting posture.

Therefore, using the raw ADC values is sufficient for our posture detection algorithm and keeps the implementation simpler.

---

# Version 1 – Raw FSR Data Acquisition

This version follows Arturo's proposed architecture.

The function reads all six FSR sensors and stores their ADC readings in an array.

No posture decision is made in this version.

The purpose of this implementation is simply to provide raw sensor data that can later be processed by a higher-level decision algorithm.

This design allows any future algorithm to analyze all sensor values together before making a posture decision.

---

# Version 2 – Independent Local Decision

This version implements local decision making for the FSR sensors.

Two independent functions are used:

- **evaluateSeatPosture()**
- **evaluateBackrestPosture()**

Each function is responsible for its own sensor group and returns only:

- `true`
- `false`

### Seat Logic

The seat function evaluates the four seat FSR sensors.

The comparison is performed as follows:

- Front Left ↔ Front Right
- Back Left ↔ Back Right

The front sensors are **not** compared with the back sensors because the thighs and hips naturally apply different pressure on the seat.

A percentage-based tolerance is used to determine whether each sensor pair is balanced.

### Backrest Logic

The backrest function evaluates the two backrest FSR sensors.

If both sensors detect pressure above the defined threshold, the function returns `true`; otherwise, it returns `false`.

---

# My Recommendation

In my opinion, **Version 2** is the better architecture for this project.

Each sensor group performs its own local evaluation and returns a simple Boolean result.

Later, a final decision function can combine these Boolean outputs together with the ToF sensors, PIR sensor, timing logic, and buzzer.

This modular design makes the software easier to understand, debug, maintain, and extend as the project grows.


___________________________________________________________________________________________________________________________________________________________________

### Michael Breyer Update on 21.05.2026

Regarding my search for measuring the user's posture I came across the following papers, that used similar approaches that we could build on: 

1. Paper [Destreza et al., 2023]: Sitting posture analysis using pressure sensors and camera based motion tracking (https://www.researchgate.net/publication/375111470_Sitting_Posture_Notification_and_Monitoring_System_A_sensor_application)
-> Overall interesting, because of build description. Basically using same components/sensors as us. We could try retaking the approach using a servo-mounted LiDAR instead of a camera for 2D back posture analysis.
   Also interesting ideas for user notification, e.g. notification straight to your laptop. 

2. Paper [Pistolesi et al., 2024]: Sitting posture analysis using 2D LiDAR scans of leg (movements) + Automation of posture recognition
(https://www.researchgate.net/publication/380789717_A_LiDAR-based_Recommendation_System_to_Improve_Sitting_Posture_Respecting_Privacy) 
-> Very well designed posture vizualization process, especially regarding posture over time (How long the user was seated in a certain position and how it may effect them).
   Paper makes the case, to use LiDAR instead of cameras in order to protect user privacy.  

3. Paper [Odesola et al., 2024]: Meta-Review of already used approaches in analysing sitting posture
(https://pmc.ncbi.nlm.nih.gov/articles/PMC11086066/)
-> Great overview of approaches other researchers already took and what sensor data they used. Chapter 4.3 is especially interesting as it names papers also using the IoT approach. Maybe we could gather some inspiration on what approach to finally take for our pitch! 


### Arturo Olivares Update on 19.05.2026

Regarding my investigation on how to communicate the computer with the Arduinos.
- I have found [this tutorial on how to use Python as a MQTT client](https://www.emqx.com/en/blog/how-to-use-mqtt-in-python) and [another one on how to use MQTT with Arduino](https://docs.arduino.cc/tutorials/uno-wifi-rev2/uno-wifi-r2-mqtt-device-to-device/). Both mention that there are several MQTT brokers available in the internet for free.


We should include in our presentation the following aspects to see what to the teachers think about it:
- Using MQTT for the communication between the computer and the Arduinos.
- Arduino and WiFi. Which option do they prefer.
- Usual and common electronic components (breadboard, wires, resistors, etc.) are not ncluded in the Part List, should we include them or not?
- Which chair are we going to use?
- Which sensor are we going to use to detect the movement of the legs? A PIR? I do not know if it has enough precision to detect the movement of the legs in such a short distance.
- Are the pressure sensors going to be enough, or will they always be saturated? (0.2-20 N range). Covering them with some foam or something like that could help to avoid saturation? If not, which other sensor could we use to detect the posture of the user?





### Sebghat Yarzada Update on 18.05.2026



Part List is Done!

Two versions have been prepared:
1. Original Version
    - 2 × Arduino Uno boards
    - 2 × ESP8266 Wi-Fi modules
2. Alternative ESP32 Version
    - 2 × ESP32 Development Boards

Both options are viable, so the teachers can choose which one they prefer.

### Meeting on 17.05.2026

#### What exactly are we going to do?

* We are going to use a chair with several  sensors to detect how well seated are you. Two measuring stations are going to be made.

  * In the Chair:

    * Pressure sensors to detect the posture of the user.
    * Measure the distance from the chair to your head.
    * Measure the movement in front of the chair to detect if you are moving the legs or not.

  * In the Table:

    * Measure the distance from the computer to your head.
    * Measure the pressure on the table to detect if you are leaning on it or not.
* Our computer will collect the information from both stations and:

  * It will show everything in a website in a user-friendly way. ***ONLY IF THE TEACHERS SAY THAT OUR PROJECT IS NOT ENOUGH FOR 4 PEOPLE.***
  * It will activate a buzzer in one of the stations if the user is not well seated for a long time.



#### What pieces do we need?

* A high chair to be able to add the distance sensor
* Pressure sensors (the more, the better). At least 4, but ideally 8 or more.
* 2 Distance sensors
* A buzzer
* 2 Arduino boards (one for each station)
* A PIR sensor (or other kind of movement sensor) to detect if the user is moving the legs or not.
* 2 WiFi modules to connect the Arduinos to the computer.



### Task to be split:

* **Sebghat** (22/05): Finding the pieces and prepare the Part List for the project
* **Atena** (28/05): Preparing the slides for the Pitch
* **Michael** (28/05): How to measure if a user is well seated or not with the information from the sensors.
* **Arturo** (28/05): How to communicate your computer with both Arduinos, process that information and send the activation of the Buzzer to one of them.

