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

int timedRead(HardwareSerial * serial, int timeout)
{
  int c;
  unsigned long _startMillis = millis1();
  do {
    c = serial->read();
    if (c >= 0) return c;
  } while(millis1() - _startMillis < timeout);
  return -1;     // -1 indicates timeout
}

size_t readBytes(HardwareSerial * serial, uint8_t * buffer, size_t length, int timeout_ms) {
  size_t count = 0;
  while (count < length) {
    int c = timedRead(serial, timeout_ms);
    if (c < 0) break;
    *buffer++ = (char)c;
    count++;
  }
  return count;
}
