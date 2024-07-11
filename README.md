# Smart Fidget Toy Device Software (Heartbeat and Activity Tracker)

This project is an Arduino-based system that tracks user interactions and heartbeat using a pulse sensor and a rotary encoder. It stores data on an SPI flash memory and provides a way to view and manage the recorded sessions.

NB in this repo you can find 2 versions of the code, where smartFidgetSmall can be used in setups with standart rotary encoders. The version we describe below in details is related to the folder smartFidget_flatRE.

## Table of Contents
- [Description](#description)
- [Features](#features)
- [Components Used](#components-used)
- [Wiring](#wiring)
- [Installation](#installation)
- [Usage](#usage)
- [License](#license)

## Description

The Heartbeat and Activity Tracker monitors and records heartbeat data using a pulse sensor and tracks physical activity through a rotary encoder. The data is stored in an SPI flash memory and can be retrieved or cleared via serial commands.

## Features

- **Heartbeat Monitoring**: Measures and records heartbeats per minute (BPM).
- **Activity Tracking**: Tracks rotational activities via a rotary encoder.
- **Data Storage**: Stores session data in SPI flash memory.
- **Real-time Clock**: Keeps track of time and date for recorded sessions.
- **LED Indicators**: Uses NeoPixel LED for status indication.

## Components Used

- [Adafruit Qt Py SAMD21](https://www.adafruit.com/product/4600)
- [PulseSensor Playground](https://pulsesensor.com/)
- [GD25Q16 - 2MB SPI Flash in 8-Pin SOIC package](https://www.adafruit.com/product/4763)
- [RotaryEncoder](https://www.adafruit.com/product/5001)

## Wiring

1. **Pulse Sensor**:
    - Signal: A3
    - Power: 3.3V
    - Ground: GND

2. **Rotary Encoder**:
    - CLK: Pin 0
    - DT: Pin 1
    - SW: Pin 2
    - Ground: GND

3. **SPI Flash Memory**:
    - Soldered at the back of Qt Py
    - Chip Select (CS): Pin 17

## Installation

1. **Clone the repository**

2. **Install Arduino Libraries**:
    - Adafruit NeoPixel
    - PulseSensor Playground
    - Adafruit SPIFlash
    - SPIMemory
    - ArduinoJson
    - RTCZero
    - RotaryEncoder

    You can install these libraries via the Arduino Library Manager.
 
3. **Upload the code**:
    Open the project in the Arduino IDE and upload the `main.ino` file to your Arduino board.

## Usage

1. **Power up the device**: Connect your Arduino to a power source.

2. **Serial Monitor**: Open the Serial Monitor in the Arduino IDE at a baud rate of 115200.

3. **Commands**:
    - `S`: Send data stored in the flash memory.
    - `D`: Delete data stored in the flash memory and after start collecting.

4. **Tracking**:
    - The device will automatically start collecting data upon detecting activity.
    - LED indicators:
        - Blue: Collecting data
        - Green: Sending data
        - Red: Deleting data
          
## License

This program is licensed under the GNU General Public License v3.0. You can redistribute it and/or modify it under the terms of the GNU GPL as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
