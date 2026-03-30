# autonomous-navigation-robot
Autonomous precision farming robot with CNN-based plant disease detection, sensor monitoring, and GSM alerts for real-time crop health management.
# AgriBot – Autonomous Precision Farming Robot

## Overview
AgriBot is an autonomous smart agriculture system designed for Dragon Fruit plantations. It monitors plant health in real time using sensors and a camera module, detects diseases using a CNN model, and sends SMS alerts to farmers via GSM — no internet required.

## Components Used
- Raspberry Pi 3B (main controller)
- Arduino Uno (motor control via L298N driver)
- USB Optical Mouse (odometry)
- MPU6050 IMU (orientation & heading)
- Camera Module (plant image capture)
- SIM800C GSM Module (SMS alerts)
- Soil moisture, temperature & humidity sensors

## How It Works
1. Sensors continuously monitor plant health parameters
2. Camera captures plant images for CNN-based disease detection (TensorFlow Lite, ~80% accuracy)
3. Raspberry Pi processes data and makes decisions
4. Arduino controls motors for autonomous navigation
5. GSM module sends SMS alert to farmer when anomaly detected

## Tech Stack
Python | Embedded C | Linux | I2C | UART | TensorFlow Lite

## Status
🔄 Ongoing
