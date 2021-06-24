#ifndef Config_h
#define Config_h
#include "Arduino.h"
#include "SoftwareSerial.h"

#ifndef GIT_REV
#define GIT_REV "deadbeef"
#endif

#ifndef BUILD_DATE
#define BUILD_DATE "2021-01-01T00:00:00Z"
#endif

#define BLESerial Serial

#define NinoTNCSerial Serial1

extern SoftwareSerial DebugSerial;

void printBytes(uint8_t * bytes, size_t len);

#endif