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
  //NinoTNCSerial.write(status, statusLen);  
  delay1(3000);
}

unsigned long last_tick = 0L;

Result result;
uint8_t pos;
unsigned long lastRead = 0L;

void handleResult(Result * result) {
  if (result->opCode == 0x81) { // Status
    DebugSerial.print("BLE Status Report: "); DebugSerial.println(result->buffer[4]);
    if (result->buffer[4] == 9) { 
      bm70.enableAdvertise();
    }
  }
}

void parsePayload(Result * result, uint16_t len) {
  DebugSerial.print("Got payload: ");
    for (uint8_t i = 0; i<pos; i++) {
      DebugSerial.print(result->buffer[i], HEX); DebugSerial.print(" ");
    }
    DebugSerial.println();
    uint16_t length = (((short) result->buffer[1]) << 8 ) + result->buffer[2] + 4;
    if (length == len) {
      DebugSerial.println("Good length");
      result->len = length;
      result->opCode = result->buffer[3];
      handleResult(result);
    } else {
      DebugSerial.println("Bad length");
    }
}

// AA 0 2 81 3 7A

void feed(uint8_t b) {
  if (pos == 0) {
    if (b == 0xAA) {
      result.buffer[pos++] = 0xAA;
      result.opCode = -1;
      result.len = -1;
    } else {
      DebugSerial.println("Continue reading until sync");
    }
  } else {
    if (b == 0xAA) {
      // beginning of next packet!
      DebugSerial.println("Out of sync");
      parsePayload(&result, pos);
      pos = 0;
      result.buffer[pos++] = 0xAA;
      result.opCode = -1;
      result.len = -1;
    } else {
      result.buffer[pos++] = b;
      if (pos == 3) {
        result.len = (((short) result.buffer[1]) << 8 ) + result.buffer[2] + 4;
      } else if (pos == 4) {
        result.opCode = b;
      } else if (pos == result.len) {
        DebugSerial.println("Read full payload");
        parsePayload(&result, pos);
        pos = 0;
        result.opCode = -1;
        result.len = -1;
      }
    }
  }
}

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

/*
  if (pos > 0 && now - lastRead > 1000) {
    // Read timeout
    DebugSerial.println("Timed out reading");
    parsePayload(&result, pos);
    pos = 0;
    result.len = -1;
    result.opCode =-1;
    lastRead = now;
  }

  if (BLESerial.available()) {
    uint8_t b = BLESerial.read();
    DebugSerial.print("Read -> ");  DebugSerial.println(b, HEX);
    lastRead = now;
    feed(b);
  }
  */

  bm70.read();

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

  return;

  // If there is data waiting from the TNC and we have a BLE connection, pass it through
  if (NinoTNCSerial.available() > 0) {
    DebugSerial.println("Reading TNC data");
    uint8_t read = readBytes(&NinoTNCSerial, tncBuffer, 100, 200);
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
