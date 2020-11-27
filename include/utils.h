#include <util/atomic.h>

volatile unsigned long timer1_millis;

ISR(TIMER1_COMPA_vect) {
  timer1_millis++;
}

unsigned long millis()
{
    unsigned long millis_return;
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
        millis_return = timer1_millis;
    }
    return millis_return;
}