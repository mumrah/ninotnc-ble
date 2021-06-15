#define F_CPU 7372800L
#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 64) 

#include "config.h"
#include "timer1.h"
#include "BM70.h"

/**
 * Pretty print a byte array as hex characters
 */
void printBytes(uint8_t * bytes, size_t len) {
  char hex[2];
  for (size_t i=0; i<len; i++) {
    sprintf(hex, "%02X ", bytes[i]);
    DebugSerial.print(hex);
  }
}

// Hard coded APRS status message
const uint8_t status[43] = {
  0xc0, 0x00, 0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80, 0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01, 
  0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20, 0x69, 
  0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0
};
const uint8_t statusLen = 43;

BM70 bm70;

SoftwareSerial DebugSerial(PD6, PD7);  

uint8_t tncBuffer[100];

// This is called when we receive data from the BM70
void rx_callback(uint8_t * buffer, uint8_t len)
{
  DebugSerial.print("Got "); DebugSerial.print(len); DebugSerial.print(" bytes of BLE data: ");
  for (size_t i=0; i<len; i++) {
    DebugSerial.print(char(buffer[i]));
  }
  DebugSerial.println();

  //NinoTNCSerial.write(status, statusLen);
  //NinoTNCSerial.flush();
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

  // Flash the indicator LED
  digitalWrite(PD5, HIGH);
  delay1(100);
  digitalWrite(PD5, LOW);
  delay1(100);
  digitalWrite(PD5, HIGH);
  delay1(100);
  digitalWrite(PD5, LOW);
  delay1(100);
  
  DebugSerial.begin(19200);
  DebugSerial.println("Setup");

  bm70 = BM70(&BLESerial, 19200, rx_callback);
  bm70.reset();

  NinoTNCSerial.begin(57600);
  //UCSR1A |= (1 << U2X1); // Set dual speed mode for serial1

  //NinoTNCSerial.write(status, statusLen);  
  delay1(3000);
}

unsigned long last_tick = 0L;

unsigned long last_nino_read = 0L;
uint8_t nino_read_pos = 0;
uint8_t nino_packet[400]; 

void loop() {
  unsigned long now = millis1();
  if ( (now - last_tick) > 1000) {
    DebugSerial.println("tick");
    digitalWrite(PD5, HIGH);
    delay1(50);
    digitalWrite(PD5, LOW);
    delay1(50);
    last_tick = now;
  }

  bm70.read();
  
  if (NinoTNCSerial.available() > 0) 
  {
    uint8_t b = NinoTNCSerial.read();
    DebugSerial.print("NinoTNC "); DebugSerial.print(nino_read_pos);
    DebugSerial.print(" "); DebugSerial.print(b, HEX); DebugSerial.print(" "); DebugSerial.println(char(b));
    nino_packet[nino_read_pos++] = b;
    last_nino_read = now;
    return;
  }

  if (nino_read_pos > 0 && (now - last_nino_read) > 500) {
      DebugSerial.print("NinoTNC read: ");
      for (size_t i=0; i<nino_read_pos; i++) {
        DebugSerial.print(char(nino_packet[i]));
      }
      DebugSerial.println();
      nino_read_pos = 0;
  }

  // Maybe update our status
  if (bm70.updateStatus())
  {
    return;
  }

  // Start advertising if we are idle
  if (bm70.status() == BM70_STATUS_IDLE)
  {
    DebugSerial.println("Enable Advertise");
    bm70.enableAdvertise();
    return;
  }

}

int main()
{
  setup();
  while (1==1) 
    loop();
  return 0;
}
