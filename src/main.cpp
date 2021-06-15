#include <Arduino.h>
#include <SoftwareSerial.h>
#include "BM70.h"


SoftwareSerial DebugSerial(PD6, PD7);  

BM70 bm70;

// Hard coded APRS status message
const uint8_t status[43] = {
  0xc0, 0x00, 0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80, 0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01, 
  0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20, 0x69, 
  0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0
};
const uint8_t statusLen = 43;

/**
 * Pretty print a byte array as hex characters
 */
void printBytes(uint8_t * bytes, size_t len) 
{
  for (size_t i=0; i<len; i++) {
    DebugSerial.print(char(bytes[i]));
  }

  char hex[10];
  for (size_t i=0; i<len; i++) {
    sprintf(hex, "%02X ", bytes[i]);
    DebugSerial.print(hex);
  }
  DebugSerial.println();
}

void rx_callback(uint8_t * buffer, uint8_t len)
{
  DebugSerial.print("Got "); DebugSerial.print(len); DebugSerial.println(" bytes of BLE data: ");
  printBytes(buffer, len);
  Serial1.write(buffer, len);
  Serial1.flush();
}

void setup()
{
  pinMode(PD5, OUTPUT);

  // Flash the indicator LED
  digitalWrite(PD5, HIGH);
  delay(100);
  digitalWrite(PD5, LOW);
  delay(100);
  digitalWrite(PD5, HIGH);
  delay(100);
  digitalWrite(PD5, LOW);

  DebugSerial.begin(19200);
  DebugSerial.println("NinoTNC + BLE Setup");

  bm70 = BM70(&Serial, 19200, rx_callback);
  bm70.reset();

  Serial1.begin(57600);
  delay(3000);
  
  Serial1.write(status, statusLen);
}

unsigned long last_tick = 0L;
unsigned long last_nino_read = 0L;
uint8_t nino_read_pos = 0;
uint8_t nino_packet[400]; 

void loop()
{
  unsigned long now = millis();
  if ( (now - last_tick) > 1000) {
    DebugSerial.println("tick");
    digitalWrite(PD5, HIGH);
    delay(50);
    digitalWrite(PD5, LOW);
    delay(50);
    last_tick = now;
  }

  bm70.read();

  // NinoTNC
  if (Serial1.available() > 0) 
  {
    uint8_t b = Serial1.read();
    DebugSerial.print("NinoTNC "); DebugSerial.print(nino_read_pos);
    DebugSerial.print(" "); DebugSerial.print(b, HEX); DebugSerial.print(" "); DebugSerial.println(char(b));
    nino_packet[nino_read_pos++] = b;
    last_nino_read = now;
    return;
  }

  if (nino_read_pos > 0 && (now - last_nino_read) > 200)
  {
    DebugSerial.println("Writing to BM70");
    bm70.send(status, statusLen);
    nino_read_pos = 0;
  }

  if (bm70.updateStatus())
    return;

  if (bm70.status() == BM70_STATUS_IDLE)
  {
    DebugSerial.println("Enable Advertise");
    bm70.enableAdvertise();
    return;
  }
}