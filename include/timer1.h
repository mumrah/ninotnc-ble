/**
 * The current number of milliseconds since the program began running
 */
unsigned long millis1();

/**
 * Pause the execution for some time
 */
void delay1(unsigned long ms);

/**
 * Read some bytes off a serial device with a timeout
 */
int readBytes(HardwareSerial * serial, uint8_t * buffer, size_t length, uint8_t timeout_ms);