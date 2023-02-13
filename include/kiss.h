#ifndef KISS_H
#define KISS_H

#include <stdint.h>

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
#define KISS_RETURN 0xFF

struct KissFrame {
  uint8_t type;
  uint8_t hdlc;
  uint8_t * buffer;
  uint16_t len;
};

class KISS
{
  public:
    KISS();
    bool feed(uint8_t b);

  private:
    uint8_t buffer[400];
    uint16_t pos;
    bool inFrame;
    bool sawFend;
    bool haveFrame;
    KissFrame frame;
};

#endif // KISS_H