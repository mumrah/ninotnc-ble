#include <Arduino.h>
#include "BM70.h"
#include "SoftwareSerial.h"

HardwareSerial bleSerial = Serial;
BM70 bm70;

SoftwareSerial tncSerial(2, 3);
uint8_t tncBuffer[256];

//const uint8_t KTS_SERVICE_UUID[16] = {0x00, 0x00, 0x00, 0x01, 0xba, 0x2a, 0x46, 0xc9, 0xae, 0x49, 0x01, 0xb0, 0x96, 0x1f, 0x68, 0xbb};
//const uint8_t KTS_RX_CHAR_UUID[16] = {0x00, 0x00, 0x00, 0x03, 0xba, 0x2a, 0x46, 0xc9, 0xae, 0x49, 0x01, 0xb0, 0x96, 0x1f, 0x68, 0xbb};
//const uint8_t KTS_TX_CHAR_UUID[16] = {0x00, 0x00, 0x00, 0x02, 0xba, 0x2a, 0x46, 0xc9, 0xae, 0x49, 0x01, 0xb0, 0x96, 0x1f, 0x68, 0xbb};

// APRS Status payload "NinoTNC BLE is working!"
const uint8_t status[43] = {
  0xc0, 0x00, 0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80, 0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01, 
  0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20, 0x69, 
  0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0
};
const uint8_t statusLen = 43;

// BLE handles
const uint16_t SERVICE_HANDLE = 0x8000;
const uint16_t RX_CHAR_HANDLE = 0x8003;
const uint16_t TX_CHAR_HANDLE = 0x8001;

// Arduino to BLE beacon, for testing
uint64_t lastBeaconMs;

void blink(uint8_t n, uint8_t t)
{
  for (uint8_t i=0; i<n; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(t);
    digitalWrite(LED_BUILTIN, LOW);
    delay(t);
  }
}

void rx_callback(uint8_t * buffer, uint8_t len)
{
  // process rx data from BLE
  tncSerial.write(status, statusLen);
}

void setup() 
{
  pinMode(LED_BUILTIN, OUTPUT);
  blink(2, 50);

  tncSerial.setTimeout(100);
  tncSerial.begin(57600);

  bm70 = BM70(&Serial, 57600, rx_callback);
  bm70.reset();

  lastBeaconMs = 0;
}

void maybeBeaconToBLE()
{
  uint64_t now = millis();
  if ((now - lastBeaconMs) > 60000)
  {
    if (bm70.status() == BM70_STATUS_CONNECTED)
    {
      bm70.send(status, statusLen);
    }
    //tncSerial.write(status, statusLen);
    lastBeaconMs = now;
  }
}

void loop() 
{
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

    // No connection, no need to continue
  if (bm70.connection() == 0x00)
  {
    return;
  }

  maybeBeaconToBLE();

  // If there is data waiting from the TNC, read it and pass to BLE
  if (tncSerial.available())
  {
    uint8_t read = tncSerial.readBytes(tncBuffer, 256);
    if (read > 0)
    {
      bm70.send(tncBuffer, read);
    }
  }
}