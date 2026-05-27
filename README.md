# IoT\_Smart-ChAIr

## The link to the common folder is [here](https://drive.google.com/drive/folders/1vy685r2IYYLLrJJtexPPP0kG4qTNNphn?usp=sharing).

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

