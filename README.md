# ESP32 Step Counter with Bluetooth and MPU6050

This project uses an ESP32, an MPU6050 accelerometer, and a Bluetooth terminal app to create a step counter. The device tracks steps, prints acceleration values for the x, y, and z axes, displays the current date, month, year, and time, and stores the total number of steps in EEPROM. The step count resets every night at 12 AM.

## Features
- Step counting using MPU6050 accelerometer
- Bluetooth communication with a terminal app
- Display of current date, time, and step count
- Persistent storage of total step count in EEPROM
- Automatic reset of step count at midnight

## Components Used
- ESP32
- MPU6050 Accelerometer
- Bluetooth Terminal App

## Connections
- MPU6050 to ESP32 using I2C:
  - SDA to GPIO 21
  - SCL to GPIO 22

## Installation
1. **Clone the repository:**
   ```sh
   git clone https://github.com/yourusername/esp32-step-counter.git
