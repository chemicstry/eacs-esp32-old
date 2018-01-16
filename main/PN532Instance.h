#ifndef _PN532INSTANCE_H_
#define _PN532INSTANCE_H_

#include <PN532_HSU.h>
#include <PN532.h>
#include <PN532Extended.h>

// Use Serial2 of ESP32
extern HardwareSerial PN532Serial;

// Serial interface
extern PN532_HSU PN532HSU;

// Original PN532 library for initialization
extern PN532 NFC;

// Extended library which allows card extensions
extern PN532Extended NFCEXT;

#endif
