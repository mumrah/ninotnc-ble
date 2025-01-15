#ifndef PASSTHRU_H
#define PASSTHRU_H

#include <stdbool.h>
#include <stdint.h>

#define PASSTHRU_UART_BUFFER_SIZE 1024
#define PASSTHRU_USB_BUFFER_SIZE 1024

typedef uint32_t (*BufferReader)(void * buffer, uint32_t len);

typedef uint32_t (*BufferWriter)(const void * buffer, uint32_t len);

typedef uint32_t (*IntSupplier)(void);

typedef bool (*Predicate)(void);

typedef void (*Callable)(void);

void passthru_init(Predicate uart_read_available, IntSupplier uart_reader);

void passthru_uart_irq(void);

uint32_t passthru_usb_read(
    IntSupplier usb_read_available, 
    BufferReader usb_reader
);

uint32_t passthru_usb_write(
    IntSupplier usb_write_available,
    BufferWriter usb_writer,
    IntSupplier usb_write_flusher
);

uint32_t passthru_uart_loop(BufferWriter uart_writer);

#endif
