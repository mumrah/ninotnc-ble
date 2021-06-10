#include <util/atomic.h>
#include "HardwareSerial.h"

#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 64) 

volatile unsigned long timer1_millis;
unsigned long then_ms;


ISR(TIMER1_COMPA_vect) {
  timer1_millis++;
}

unsigned long millis1()
{
    unsigned long millis_return;
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        millis_return = timer1_millis;
    }
    return millis_return;
}

void delay1(unsigned long ms)
{
	uint32_t start = millis1();

	while ( (millis1() - start) < ms) {
		// do nothing
	}
}

uint8_t readBytes(HardwareSerial * serial, uint8_t * buffer, size_t length, uint8_t timeout_ms) {
  size_t count = 0;
  unsigned long _startMillis = millis1();
  while (count < length && (millis1() - _startMillis) < timeout_ms) {
    int c = serial->read();
    if (c < 0) break;
    *buffer++ = (char)c;
    count++;
  }
  return count;
}
