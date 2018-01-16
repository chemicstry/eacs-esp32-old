#include "PN532Instance.h"

// Use Serial2 of ESP32
HardwareSerial PN532Serial(2);

// Serial interface
PN532_HSU PN532HSU(PN532Serial);

// Original PN532 library for initialization
PN532 NFC(PN532HSU);

// Extended library which allows card extensions
PN532Extended NFCEXT(PN532HSU);
