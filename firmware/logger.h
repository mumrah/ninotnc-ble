
#ifndef LOGGER_H
#define LOGGER_H

bool logger_init(void);

void logger_write_pcap(uint32_t msec, uint32_t usec, char * key, char * buf, uint16_t len);

void logger_write_ascii(uint32_t millis, char * key, char * buf, uint16_t len);

void logger_deinit(void);

#endif
