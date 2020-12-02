#ifndef Config_h
#define Config_h

#include "Arduino.h"
#include "SoftwareSerial.h"

#define BLESerial Serial

#define NinoTNCSerial Serial1

extern SoftwareSerial DebugSerial;

void printBytes(uint8_t * bytes, size_t len);

#endif