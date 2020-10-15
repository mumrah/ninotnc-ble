#include <Arduino.h>
#include "BM70.h"
#include "SoftwareSerial.h"

//#define DEBUG

HardwareSerial bleSerial = Serial;
BM70 bm70;

SoftwareSerial tncSerial(2, 3);
uint8_t tncBuffer[100];

//const uint8_t KTS_SERVICE_UUID[16] = {0x00, 0x00, 0x00, 0x01, 0xba, 0x2a, 0x46, 0xc9, 0xae, 0x49, 0x01, 0xb0, 0x96, 0x1f, 0x68, 0xbb};
//const uint8_t KTS_RX_CHAR_UUID[16] = {0x00, 0x00, 0x00, 0x03, 0xba, 0x2a, 0x46, 0xc9, 0xae, 0x49, 0x01, 0xb0, 0x96, 0x1f, 0x68, 0xbb};
//const uint8_t KTS_TX_CHAR_UUID[16] = {0x00, 0x00, 0x00, 0x02, 0xba, 0x2a, 0x46, 0xc9, 0xae, 0x49, 0x01, 0xb0, 0x96, 0x1f, 0x68, 0xbb};

const uint8_t status[43] = {
  0xc0, 0x00, 0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80, 0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01, 
  0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20, 0x69, 
  0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0
};
const uint8_t statusLen = 43;

const uint16_t SERVICE_HANDLE = 0x8000;
const uint16_t RX_CHAR_HANDLE = 0x8003;
const uint16_t TX_CHAR_HANDLE = 0x8001;

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

  // Setup UART
#ifdef DEBUG
  Serial.begin(57600);
  Serial.println("Setup");
#else
  tncSerial.setTimeout(50UL); 
  tncSerial.begin(57600);
#endif

  bm70 = BM70(&Serial, 57600, rx_callback);
  
#ifdef DEBUG
  Serial.println("Resetting BM70");
#endif

  bm70.reset();
  lastBeaconMs = 0;
}

void maybeBeaconToTnc()
{
  uint64_t now = millis();
  if ((now - lastBeaconMs) > 60000)
  {
    if (bm70.status() == BM70_STATUS_CONNECTED)
    {
      bm70.send(status, statusLen);
      //blink(3, 50);
    }
    //tncSerial.write(status, statusLen);
    lastBeaconMs = now;
  }
}

void loop() 
{
  //delay(100);
  // Read off pending data and process events
  bm70.read();

  // Need to know our status
  if (bm70.status() == BM70_STATUS_UNKNOWN)
  {
#ifdef DEBUG
    Serial.println("Learning our status");
#endif
    bm70.updateStatus();
    //blink(1, 50);
    return;
  }

  // Start advertising
  if (bm70.status() == BM70_STATUS_IDLE)
  {
#ifdef DEBUG
    Serial.println("Enter standby mode");
#endif
    bm70.enableAdvertise();
    //blink(2, 50);
    return;
  }

    // No connection, no need to continue
  if (bm70.connection() == 0x00)
  {
    return;
  }

  maybeBeaconToTnc();

  if (tncSerial.available())
  {
    uint8_t read = tncSerial.readBytes(tncBuffer, 100);
    if (read > 0)
    {
      bm70.send(tncBuffer, read);
    }
  }
}