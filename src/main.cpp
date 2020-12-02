#define F_CPU 7372800L
#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 64) 

#include "config.h"
#include "timer1.h"
#include "BM70.h"

void printBytes(uint8_t * bytes, size_t len) {
  char hex[2];
  for (size_t i=0; i<len; i++) {
    sprintf(hex, "%02X ", bytes[i]);
    DebugSerial.print(hex);
  }
}

const uint8_t status[43] = {
  0xc0, 0x00, 0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80, 0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01, 
  0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20, 0x69, 
  0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0
};
const uint8_t statusLen = 43;

BM70 bm70;

SoftwareSerial DebugSerial(PD6, PD7);  


uint8_t tncBuffer[256];

void rx_callback(uint8_t * buffer, uint8_t len)
{
  // process rx data from BLE
  DebugSerial.print("Got "); DebugSerial.print(len); DebugSerial.print(" bytes of BLE data: ");
  for (size_t i=0; i<len; i++) {
    DebugSerial.print(char(buffer[i]));
  }
  DebugSerial.println();

  NinoTNCSerial.write(buffer, len);
  NinoTNCSerial.flush();
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

  DebugSerial.begin(9600);
  DebugSerial.println("Setup");

  delay1(1000);
  NinoTNCSerial.begin(57600);
  NinoTNCSerial.write(status, statusLen);
  
}

unsigned long last_tick = 0L;

void loop() {
  delay1(100);

  unsigned long now = millis1();
  if ( (now - last_tick) > 1000) {
    DebugSerial.println("tick");
    digitalWrite(PD5, HIGH);
    delay1(50);
    digitalWrite(PD5, LOW);
    delay1(50);
    //bm70.readCharacteristicValue(KTS_TX_CHAR_UUID);
    last_tick = now;
  }
  

  // Read off pending data and process events
  bm70.read();

  // Maybe update our status
  bm70.updateStatus();

  // Start advertising
  if (bm70.status() == BM70_STATUS_IDLE)
  {
    bm70.enableAdvertise();
    return;
  }

  // If there is data waiting from the TNC and we have a BLE connection, pass it through
  if (NinoTNCSerial.available() > 0) {
    DebugSerial.println("Reading TNC data");
    uint8_t read = readBytes(&NinoTNCSerial, tncBuffer, 256);
    if (read > 0) { 
      bm70.send(status, statusLen);
      //bm70.send(tncBuffer, read);
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
