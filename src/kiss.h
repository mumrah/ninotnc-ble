#ifndef KISS_H
#define KISS_H

#include <stdint.h>
#include <stdbool.h>

#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

#define KISS_DATA 0x00
#define KISS_TX_DELAY 0x01
#define KISS_P 0x02
#define KISS_SLOT_TIME 0x03
#define KISS_TX_TAIL 0x04
#define KISS_FULL_DUPLEX 0x05
#define KISS_SET_HARDWARE 0x06
#define KISS_DATA_ACK_MODE 0x0C   // implemented by NinoTNC, maybe others
#define KISS_RETURN 0xFF

// Magic numbers for NinoTNC
#define KISS_N9600A_VER_CMD 0x08 
#define KISS_N9600A_HDLC_PORT 0xE


#define KISS_FEED_TIMEOUT_MS 3000


typedef struct {
  uint8_t type;
  uint8_t hdlc;
  uint8_t * buffer;
  uint16_t len;
} KissFrame;


typedef struct {
    uint8_t buffer[400];
    uint16_t pos;
    bool inFrame;
    bool inEscape;
    bool sawFend;
    bool haveFrame;
    uint32_t last_feed_ms;

} KissState;


void kiss_init(KissState * kiss, uint32_t now_ms);

bool kiss_feed(KissState * kiss, uint8_t byte, uint32_t now_ms);

void kiss_read(KissState * kiss, KissFrame * frame);

uint16_t kiss_encode(KissFrame * frame, uint8_t * buffer);

#endif // KISS_H
