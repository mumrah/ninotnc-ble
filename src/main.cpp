#define FOSC 7372800L // Clock Speed
#define NINO_BAUD 57600
#define NINO_UBRR FOSC/16/NINO_BAUD-1

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "BM70.h"
#include <avr/interrupt.h>

SoftwareSerial DebugSerial(PD6, PD7);  

BM70 bm70;

// Hard coded APRS status message
/*
const uint8_t status[43] = {
  0xc0, 0x00, 0x82, 0xa0, 0xb4, 0x84, 0x98, 0x8a, 0x80, 0x96, 0x68, 0x88, 0x84, 0xb4, 0x40, 0x01, 
  0x03, 0xf0, 0x3e, 0x4e, 0x69, 0x6e, 0x6f, 0x54, 0x4e, 0x43, 0x20, 0x42, 0x4c, 0x45, 0x20, 0x69, 
  0x73, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x21, 0xc0
};
const uint8_t statusLen = 43;
*/

unsigned long last_tick = 0L;
unsigned long last_nino_read = 0L;

uint8_t nino_rx_buffer[255]; 
uint8_t nino_rx_pos = 0;


uint8_t nino_tx_packet[255]; 
uint8_t nino_tx_pos = 0;
uint8_t nino_tx_len = 0;

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

void USART1_Init(unsigned int ubrr)
{
    UBRR1L = (unsigned char)ubrr; 
    UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1);  
    UCSR1C =  (3<<UCSZ00);
}

// BLE callback
void ble_callback(uint8_t * buffer, uint8_t len)
{
  if (buffer[0] == 0xC0 && buffer[len-1] == 0xC0)
  {
    DebugSerial.print("Writing "); DebugSerial.print(len); DebugSerial.println(" bytes to NinoTNC");
    for (uint8_t i = 0; i<len; i++)
      nino_tx_packet[i] = buffer[i];
    nino_tx_len = len;
    nino_tx_pos = 0;
    UCSR1B |= (1<<UDRIE1);
    DebugSerial.println("send");
  }
  else
  {
    DebugSerial.print("Skipping non-KISS data");
    printBytes(buffer, len);
  } 
}

// UART Interrupts
ISR(USART1_RX_vect)
{
  nino_rx_buffer[nino_rx_pos++] = UDR1;
}
ISR(USART1_UDRE_vect)
{ 
  if (nino_tx_pos < nino_tx_len)
    UDR1 = nino_tx_packet[nino_tx_pos++];
  else
    UCSR1B &= ~(1 << UDRIE1);
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
  DebugSerial.print("\n~~ NinoTNC + BLE by K4DBZ ~~\n");

  bm70 = BM70(&Serial, 19200, ble_callback);
  bm70.reset();

  delay(3000);
  
  USART1_Init(NINO_UBRR);
  sei();
  //USART1_Transmit(status, statusLen);
}

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

  // Check if we read a full packet from NinoTNC
  if ((now - last_nino_read) > 200)
  {
    if (nino_rx_pos > 0) 
    {
      if (nino_rx_buffer[nino_rx_pos-1] == 0xC0)
      {
        printBytes(nino_rx_buffer, nino_rx_pos);
        bm70.send(nino_rx_buffer, nino_rx_pos);
        nino_rx_pos = 0;
      }
      else if ((now - last_nino_read) > 1000)
      {
        DEBUG_START("Timed out reading KISS from NinoTNC"); DEBUG_END();
        nino_rx_pos = 0;
      }
    }
    last_nino_read = now;
  }

  // Manage the BLE state
  if (bm70.updateStatus())
    return;

  if (bm70.status() == BM70_STATUS_IDLE)
  {
    DebugSerial.println("Enable Advertise");
    bm70.enableAdvertise();
    return;
  }
}