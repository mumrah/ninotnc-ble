#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
  SoftwareSerial DebugSerial(PD6, PD7);  

  #define DEBUG_START(...) \
    DebugSerial.print(millis()); \
    DebugSerial.print(': '); \
    DebugSerial.print(__VA_ARGS__)
  #define DEBUG_PRINT(...) DebugSerial.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...) DebugSerial.println(__VA_ARGS__)
  #define DEBUG_END(...) DebugSerial.println(__VA_ARGS__)
  #define DEBUG_INIT() DebugSerial.begin();
#else
  #define DEBUG_START(...)
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
  #define DEBUG_END(...)
#endif

#endif