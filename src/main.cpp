#define F_CPU 7372800L
#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 64) 

#define BLESerial Serial
#define NinoTNCSerial Serial1

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "timer1.h"
#include "BM70.h"

const uint8_t status[43] = {
  0xc0, 0x00, 0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80, 0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01, 
  0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20, 0x69, 
  0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0
};
const uint8_t statusLen = 43;

BM70 bm70;

SoftwareSerial debug(PD6, PD7); // PD6, PD7

uint8_t tncBuffer[256];

void rx_callback(uint8_t * buffer, uint8_t len)
{
  // process rx data from BLE
  debug.println("Got BLE data");
  NinoTNCSerial.write(buffer, len);
}

void setup() {
  cli();
  TCCR1A = 0; // Normal timer operation. User timer1 (16bit) for longer count
  TCCR1B = 0;
  TCNT1 = 0; // Clear timer counter register

  OCR1A = CTC_MATCH_OVERFLOW;  // Timer reset value, triggers the interupt

  TCCR1B |= (1 << WGM12); // CTC mode (Clear Timer on Compare)
  TCCR1B |= (1 << CS10); // Set C10 and C11 for 64 prescaler
  TCCR1B |= (1 << CS11);
  TIMSK1 |= (1 << OCIE1A); // Enable timer compare interrupt
  sei();

  pinMode(PD5, OUTPUT);

  digitalWrite(PD5, HIGH);
  delay1(100);
  digitalWrite(PD5, LOW);
  delay1(100);
  digitalWrite(PD5, HIGH);
  delay1(100);
  digitalWrite(PD5, LOW);
  delay1(100);

  bm70 = BM70(&BLESerial, 115200, rx_callback);
  bm70.reset();

  NinoTNCSerial.begin(57600);

  debug.begin(9600);
  debug.println("Setup");
}

void loop() {
  debug.println("tick");
  digitalWrite(PD5, HIGH);
  delay1(50);
  digitalWrite(PD5, LOW);
  delay1(50);

  // Read off pending data and process events
  bm70.read();

  // Need to know our status
  if (bm70.status() == BM70_STATUS_UNKNOWN)
  {
    bm70.updateStatus();
    return;
  }

  // Start advertising
  if (bm70.status() == BM70_STATUS_IDLE)
  {
    bm70.enableAdvertise();
    return;
  }

  // If there is data waiting from the TNC and we have a BLE connection, pass it through
  if (NinoTNCSerial.available() > 0) {
    debug.println("Reading TNC data");
    uint8_t read = readBytes(&NinoTNCSerial, tncBuffer, 256);
    debug.print("Read "); debug.print(read); debug.println(" bytes");
    if (read > 0) { 
      bm70.send(tncBuffer, read);
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
