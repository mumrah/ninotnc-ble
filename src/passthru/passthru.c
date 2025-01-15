#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "pico/multicore.h"
#include "passthru.h"

static critical_section_t uart_rx_lock;
static char uart_rx_buffer[PASSTHRU_UART_BUFFER_SIZE];
static uint16_t uart_rx_pos = 0;

static critical_section_t usb_rx_lock;
static char usb_rx_buffer[PASSTHRU_USB_BUFFER_SIZE];
static uint16_t usb_rx_pos = 0;

static Predicate uart_readable;
static IntSupplier uart_reader;

/*********************************************************
 *                                                       *
 *       ABSOLUTELY NO printf() ALLOWED IN HERE!         *
 *                                                       *
 *  VIOLATORS WILL BE TOSSED OUT OF THE NEAREST WINDOW.  *
 *                                                       *
 *********************************************************/

void passthru_init(Predicate uart_readable_fn, IntSupplier uart_reader_fn) {
    uart_readable = uart_readable_fn;
    uart_reader = uart_reader_fn;
    critical_section_init(&uart_rx_lock);  
    critical_section_init(&usb_rx_lock); 
}

void passthru_uart_irq(void) {
    critical_section_enter_blocking(&uart_rx_lock);
    while (uart_readable() && uart_rx_pos < PASSTHRU_UART_BUFFER_SIZE) {
        char ch = uart_reader();
        uart_rx_buffer[uart_rx_pos++] = ch;
    }
    critical_section_exit(&uart_rx_lock);
}

// Loop for handling USB input and output.
uint32_t passthru_usb_read(
    IntSupplier usb_read_available, 
    BufferReader usb_reader
) {
    // Check if the USB has data waiting. Read it to the USB buffer if we can.
    uint32_t usb_available = usb_read_available();
    if (usb_available > 0) {
        critical_section_enter_blocking(&usb_rx_lock);
        uint32_t size_to_read;
        if (usb_available < PASSTHRU_USB_BUFFER_SIZE - usb_rx_pos) {
            size_to_read = usb_available;
        } else {
            size_to_read = PASSTHRU_USB_BUFFER_SIZE - usb_rx_pos;
        }

        // Only issue a read if there is actually capacity
        uint32_t usb_read = 0;
        if (size_to_read > 0) {
            usb_read = usb_reader(&usb_rx_buffer[usb_rx_pos], size_to_read);
            usb_rx_pos += usb_read;
        }
        critical_section_exit(&usb_rx_lock);
        return usb_read;
    }    
    return 0;
}

uint32_t passthru_usb_write(    
    IntSupplier usb_write_available,
    BufferWriter usb_writer,
    IntSupplier usb_write_flusher
) {
    // Check if UART has received any data. Send to USB if we can.
    if (uart_rx_pos > 0) {
        critical_section_enter_blocking(&uart_rx_lock);
        uint32_t usb_write_capacity = usb_write_available();

        // Write only what we have capacity for. Fixup UART rx buffer.
        uint32_t size_to_write;
        if (uart_rx_pos < usb_write_capacity) {
            size_to_write = uart_rx_pos;
        } else {
            size_to_write = usb_write_capacity;
        }
        uint32_t written = usb_writer(uart_rx_buffer, size_to_write);

        // If we didn't write everything in the buffer, need to move the buffer
        if (written < uart_rx_pos) {
            memmove(uart_rx_buffer, &uart_rx_buffer[written], uart_rx_pos - written);
        }
        uart_rx_pos -= written;
        critical_section_exit(&uart_rx_lock);
        if (written) {
            usb_write_flusher();
        }
        return written;
    }
    return 0;
}

uint32_t passthru_uart_loop(BufferWriter uart_writer) {
    // Check if we read any USB data. If so, write it UART. This will block until 
    // the all of the bytes have been written
    if (usb_rx_pos > 0) {
        critical_section_enter_blocking(&usb_rx_lock);
        uint32_t written = uart_writer(usb_rx_buffer, usb_rx_pos);
        usb_rx_pos -= written;
        critical_section_exit(&usb_rx_lock);
        return written;
    }
    return 0;
}
