# NinoTNC BLE daughter board

This firmware is for an ATMega328pb which controls a BM70 module and relays data between
a BLE client (aprs.fi) and a serial TNC (NinoTNC).

Due to bugs in the mega core library for the 328pb, timers do not work right. This has been
worked around using the functions defined in timer1.h (millis1 and delay1).

Only some of the BLE spec is implemented. This code uses a hard coded service UUID as well as 
read and write characteristic UUIDs (defined in BM70.h). 

Fuses are: E:F5, H:D6, and L:FF