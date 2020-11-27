#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "SoftwareSerial.h"
#include "Arduino.h"
#include "BM70.h"

BM70 bm70;

HardwareSerial bleSerial = Serial;
HardwareSerial tncSerial = Serial1;
SoftwareSerial debug(10, 11);

// APRS Status payload "NinoTNC BLE is working!"
const uint8_t status[43] = {
  0xc0, 0x00, 0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80, 0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01, 
  0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20, 0x69, 
  0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0
};
const uint8_t statusLen = 43;

uint8_t tncBuffer[256];
unsigned long last_tnc_write;

#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 64) 

unsigned long then_ms;

typedef struct {
    int durations[10];
    int duration_len;
    int duration_idx;
} blink_t; 

blink_t blink;

void blinkRx()
{
  // . _ . TODO fix these
  blink_t r = {{1, 1, 3, 1, 1, 7}, 6, 0};
  blink = r;
  PORTD &= ~(1 << PD5);
}

void blinkTx()
{
  // - 
  blink_t two = {{3, 7}, 2, 0};
  blink = two;
  PORTD &= ~(1 << PD5);
}

void blinkThree()
{
  blink_t three = {{3, 1, 1, 1, 1, 1, 1, 7}, 8, 0};
  blink = three;
  PORTD &= ~(1 << PD5);
}

int blink_tick(blink_t * blink)
{
  if (blink->duration_idx >= blink->duration_len)
    return -1;

  if (--blink->durations[blink->duration_idx] == 0) {
      PORTD ^= (1 << PD5);
      blink->duration_idx++;
      return 1;
  }
  return 0;
}

void rx_callback(uint8_t * buffer, uint8_t len)
{
  // process rx data from BLE
  tncSerial.write(status, statusLen);
}

void setup()
{
  cli();
  TCCR1A = 0; // Normal timer operation. User timer1 (16bit) for longer count
  TCCR1B = 0;
  TCNT1 = 0; // Clear timer counter register

  OCR1A = CTC_MATCH_OVERFLOW;  // Timer reset value, triggers the interupt

  TCCR1B |= (1 << WGM12); // CTC mode (Clear Timer on Compare)
  TCCR1B |= (1 << CS10); // Set C10 and C11 for 64 prescaler
  TCCR1B |= (1 << CS11);
  TIMSK1 |= (1 << OCIE1A); // Enable timer compare interrupt
  
  DDRD |= (1 << PD5);
  
  sei();

  PORTD |= (1 << PD5);
  blink = {{}, 0, 0};

  debug.begin(57600);

  bm70 = BM70(&bleSerial, 57600, rx_callback);
  bm70.reset();
  
  tncSerial.begin(57600);
}

void loop() {
  unsigned long now = millis();
  if (now - then_ms > 100) {
      then_ms = now;
      if (blink_tick(&blink) == -1) // -1 is done sending last blink
      {
        //blinkThree();
        debug.println("three");
      } 
  }

  if (Serial1.available()) {
    Serial1.readBytes(tncBuffer, 256);
    blinkRx();
  }

  if (now - last_tnc_write > 10000) {
    if (Serial1.availableForWrite()) {
      Serial1.write(status, statusLen);
      blinkTx();
      last_tnc_write = now;
    }
  }
}

int main()
{
  setup();
  while (1==1) 
    loop();
  return 0;
}
