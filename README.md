# Outdated. This repository is based on old ESP-IDF version. Use [eacs-esp32](https://github.com/chemicstry/eacs-esp32) repository instead.

# eacs-esp32
Extensible Access Control System. ESP32 based door controller.

## Description

Software is written using as much C++11 (including threading) features as possible so it should be easily portable to other platforms. Two libraries (PN532 and WebSockets) are arduino only and may require substitutions on unsupported platforms.

## Specifications
* Communications
  * WiFi
  * Ethernet (tested with Olimex ESP32-EVB board)
* NFC reader
  * PN532 (default is pin 16 RX, 17 TX on ESP32 side)

## Requirements
- [eacs-tag-auth](https://github.com/chemicstry/eacs-tag-auth) service for RFID tag validation (cryptographically verifies tag authenticity)
- [eacs-user-auth](https://github.com/chemicstry/eacs-user-auth) service for user authentication (matches tag UID to user and checks permissions)

## Installation instructions

Requires [ESP-IDF](https://esp-idf.readthedocs.io/en/latest/get-started/index.html) to compile.

To configure (set serial port and other settings):

`make menuconfig`

To build:

`make`

To flash:

`make flash`

To open serial console (to exit `ctrl+]`):

`make monitor`
