#include <stdint.h>

#define RINGFIFO_SIZE (64)              /* serial buffer in bytes (power 2) */
#define RINGFIFO_MASK (RINGFIFO_SIZE-1ul) /* buffer size mask */

/* Buffer read / write macros */
#define RINGFIFO_RESET(ringFifo)      {ringFifo.rdIdx = ringFifo.wrIdx = 0;}
#define RINGFIFO_WR(ringFifo, dataIn) {ringFifo.data[RINGFIFO_MASK & ringFifo.wrIdx++] = (dataIn);}
#define RINGFIFO_RD(ringFifo, dataOut){ringFifo.rdIdx++; dataOut = ringFifo.data[RINGFIFO_MASK & (ringFifo.rdIdx-1)];}
#define RINGFIFO_EMPTY(ringFifo)      (ringFifo.rdIdx == ringFifo.wrIdx)
#define RINGFIFO_FULL(ringFifo)       ((RINGFIFO_MASK & ringFifo.rdIdx) == (RINGFIFO_MASK & (ringFifo.wrIdx+1)))
#define RINGFIFO_COUNT(ringFifo)      (RINGFIFO_MASK & (ringFifo.wrIdx - ringFifo.rdIdx))

/* buffer type */
typedef struct{
    uint8_t size;
    uint8_t wrIdx;
    uint8_t rdIdx;
    uint8_t data[RINGFIFO_SIZE];
} RingFifo_t;
