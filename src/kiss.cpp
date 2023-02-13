#include "kiss.h"
#include "config.h"
#include "debug.h"

KISS::KISS()
{
  pos = 0;
  inFrame = false;
  sawFend = false;
  haveFrame = false;
}

bool KISS::feed(uint8_t b)
{
  // Reset our state if we previously had a complete frame
  if (haveFrame)
  {
    pos = 0;
    sawFend = false;
    inFrame = false;
    haveFrame = false;
  }

  // Sync on FEND
  if (pos == 0 && b != FEND)
    return false;

  if (b != FEND)
  {
    if (!inFrame)
      inFrame = true;
    buffer[pos++] = b;
  }
  else
  {
    if (!inFrame)
      sawFend = true; // consume sequential FENDs
    else
    {
      if (sawFend)
      {
        // decode buffer
        DEBUG_START("Got KISS frame"); DEBUG_END();
        frame.hdlc = (buffer[0] >> 4) & 0x0F;
        frame.type = buffer[0] & 0x0F;
        frame.buffer = &buffer[1];
        frame.len = pos-1;
        DEBUG_START("KISS port "); DEBUG_PRINTLN(frame.hdlc);
        DEBUG_PRINT("KISS command "); DEBUG_PRINTLN(frame.type, HEX);
        DEBUG_PRINT("KISS len "); DEBUG_PRINTLN(frame.len);
        printBytes(frame.buffer, frame.len);
        haveFrame = true;
      }
      else
      {
        DEBUG_START("Dropping partial KISS frame"); DEBUG_END();
        pos = 0;
        sawFend = false;
        inFrame = false;
        haveFrame = false;
      }
    }
  }
  return haveFrame;
}