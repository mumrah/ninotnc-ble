/**
 * /Users/davidarthur/.pico-sdk/picotool/2.1.0/picotool/picotool load /Users/davidarthur/Code/ninoble/build/sniffer.elf -fx
 * 
 */
#include <stdio.h>
#include <stdbool.h>
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "oled.h"
#include "ax25.h"
#include "kiss.h"
#include "config.h"
#include "blink.h"

#define UART_TX_PIN 20    // Use 20 and 21 for v3 board
#define UART_RX_PIN 21

KissState tncKiss;
KissFrame kissFrame;
AX25Packet packet;
OLED oled;
LED led;

char line[16]; 
uint64_t async_now_ms;

void init_uart(irq_handler_t uart_irq, bool enable_fifos) {
    // UART init
     // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 57600);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn on FIFO's
    uart_set_fifo_enabled(UART_ID, enable_fifos);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, uart_irq);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
}

/**
 * UART RX interrupt handler. Called as we receive bytes from the TNC. We feed these into 
 * a KISS parser
 */
void on_uart_rx() {
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        //printf("uart: %02X\n", ch);
        // Only feed a byte to kiss parser if there is not a frame ready.
        if (!tncKiss.haveFrame) {
            kiss_feed(&tncKiss, ch, async_now_ms);
        }  
    }
}

void display_ax25_packet_oled(AX25Packet * packet, uint8_t * buffer, bool rx) {
    oled_clear(&oled);
    if (rx) {
        oled_write_string(&oled, 0, 0, "RX");
    } else {
        oled_write_string(&oled, 0, 0, "TX");
    }
    AX25FrameType frame_type = ax25_frame_type(packet, buffer);
    switch (frame_type) {
        case AX25_INFORMATION:
            oled_write_string(&oled, 24, 0, "I");
            break;
        case AX25_UNNUMBERED:
            oled_write_string(&oled, 24, 0, "U");
            AX25UnnumberedFrameType u_subtype = ax25_u_subtype(packet, buffer);
            oled_write_string(&oled, 40, 0, unnumbered_frame_type_names[u_subtype]);
            AX25PidType pid_type = ax25_packet_pid_field(packet, buffer);
            oled_write_string(&oled, 64, 0, pid_type_names[pid_type]);
            break;
        case AX25_SUPERVISORY:
            oled_write_string(&oled, 24, 0, "S");
            break;
    }

    memset(line, ' ', sizeof(line));
    uint8_t len = ax25_format_call(&line[5], &(packet->source_call));
    line[5 + len] = ' ';
    line[6 + len] = '>';
    oled_write_string(&oled, 0, 16, line);
    memset(line, ' ', sizeof(line));
    ax25_format_call(&line[5], &(packet->dest_call));
    oled_write_string(&oled, 0, 24, line);
    oled_render(&oled);
}

void display_rx_packet(AX25Packet * packet, uint8_t * buffer, uint16_t len) {
    display_ax25_packet_oled(packet, buffer, true);
}

void display_tx_packet(AX25Packet * packet, uint8_t * buffer, uint16_t len) {
    display_ax25_packet_oled(packet, buffer, false);
}

void wifi_led_toggle(bool on) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
}

int main(void) {
    stdio_init_all();
    if (cyw43_arch_init()) {
        return -1;
    }

    // OLED
    oled_init(&oled);
    oled_clear(&oled);
    oled_write_string(&oled, 5, 0, "NinoTNC BLE");
    oled_write_string(&oled, 5, 8, "David Arthur");
    oled_write_string(&oled, 5, 16, "K4DBZ");
    oled_write_string(&oled, 5, 24, "TARPN.NET");
    oled_render(&oled);

    async_now_ms = to_ms_since_boot(get_absolute_time());
    printf("init");

    kiss_init(&tncKiss, async_now_ms);

    init_uart(on_uart_rx, true);

    init_led(&led, wifi_led_toggle);
    blink_morse(&led, async_now_ms, 'S');

    // this is a forever loop in place of where user code would go.
    while(true) {
        async_now_ms = to_ms_since_boot(get_absolute_time());
        check_led_state(&led, async_now_ms);

        if (tncKiss.haveFrame) {
            kiss_read(&tncKiss, &kissFrame);
            printf("Got valid KISS from TNC\n");
            //printf_hexdump(kissFrame.buffer, kissFrame.len);
            printf("\n");
            
            if (ax25_init(&packet, kissFrame.buffer, kissFrame.len)) {
                display_rx_packet(&packet, kissFrame.buffer,  kissFrame.len);
            }

            blink_morse(&led, async_now_ms, 'R');
            kiss_init(&tncKiss, async_now_ms);
        }
    }

    return 0;
}