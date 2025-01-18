#include <stdio.h>
#include <stdbool.h>

#include "kiss.h"

void kiss_init(KissState * kiss, uint32_t now_ms) {
    kiss->pos = 0;
    kiss->inFrame = false;
    kiss->inEscape = false;
    kiss->sawFend = false;
    kiss->haveFrame = false;
    kiss->last_feed_ms = now_ms;
}

bool kiss_feed(KissState * kiss, uint8_t byte, uint32_t now_ms) {
    //printf("kiss_feed: %02X\n", byte);
    if ((now_ms - kiss->last_feed_ms) > KISS_FEED_TIMEOUT_MS) {
        kiss_init(kiss, now_ms);
    }

    kiss->last_feed_ms = now_ms;

    if (byte == FEND) {
        if (kiss->inFrame) {
            if (kiss->sawFend) {
                kiss->haveFrame = true;
                return true;
            } else {
                kiss_init(kiss, now_ms);
                kiss->sawFend = true;
                return false;
            }            
        } else {
            // Keep consuming sequential FEND
            kiss->sawFend = true;
            return false;
        }
    } else {
        // Handle data bytes
        if (!kiss->inFrame) {
            kiss->inFrame = true;
        }

        if (byte == FESC) {
            if (kiss->inEscape) {
                // Protocol violation. What to do?
            }

            kiss->inEscape = true;
            return false;
        }

        if (kiss->inEscape) {
            switch (byte) {
                case TFEND:
                    byte = FEND;
                    kiss->inEscape = false;
                    break;
                case TFESC:
                    byte = FESC;
                    kiss->inEscape = false;
                    break;
                default:
                    break;
            }
        }

        kiss->buffer[kiss->pos++] = byte;
        return false;
    }
}


/**
 * Finish decoding the complete KISS frame. This does not copy any data between
 * the KissState and KissFrame. The KissFrame buffer is valid until the next
 * call to kiss_init (which resets the buffer);
 */
void kiss_read(KissState * kiss, KissFrame * frame) {
    frame->hdlc = (kiss->buffer[0] >> 4) & 0x0F;
    frame->type = kiss->buffer[0] & 0x0F;
    frame->buffer = &(kiss->buffer[1]);
    frame->len = kiss->pos - 1;
}

/**
 * Encode a KissFrame into a given buffer. 
 * 
 * @return the number of bytes written
 */
uint16_t kiss_encode(KissFrame * frame, uint8_t * buffer) {
    buffer[0] = FEND;
    buffer[1] = (frame->hdlc << 4) | frame->type;
    uint16_t pos = 2;
    for (uint16_t i=0; i<frame->len; i++) {
        uint8_t byte = frame->buffer[i];
        switch (byte) {
            case FEND:
                buffer[pos++] = FESC;
                buffer[pos++] = TFEND;
                break;
            case FESC:
                buffer[pos++] = FESC;
                buffer[pos++] = TFESC;
                break;
            default:
                buffer[pos++] = byte;
        }
    }
    buffer[pos++] = FEND;
    return pos;
}
