# Changelog

All notable changes to this project will be documented in this file.

## Contributors

The names listed in the image are:
- Robbie Cato
- Bartosz Krawczyk
- Ahmed Elbary
- Nivetha Sakthivel
- Kaung Myat
- Jagdish Patil
- Anyeh Ndi-Tah


## [23/01/2025] - Feature Changes

See Demonstrations folder for more videos.

### Attempted to replace Arduino with ESP32
- It was found to not have enough pins, therefore used Arduino ATMEGA and ESP32.
- ATMEGA controls motors and motor drivers.
- ESP32 receives MQTT messages, and translates them into motor control commands.

### Battery adjustments
- Increased the capacity of the secondary battery (that powers the Arduino ATMEGA and ESP32).

### Control software
- MQTT communication and Twist to JSON code has been tested by modulating LED intensities to prove core concept.
- For use with an ATMEGA, a WiFi shield was needed for testing on OpenScout. Works well with the ESP32 but lacks enough pins to control the robot.
- Implemented radio receiver which converts radio commands into throttle and angular values instead.
- Replaced `simple_pid.ino` with `refined_pid.ino`.

### Added simulation software
- A simulated version of the OpenScout was created.
- Implemented using Gazebo Harmonic and ROS2.

### Changes to documentation
- Updated demonstrations with changes.

