/**
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdbool.h>
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "tusb.h"

#include "oled.h"
#include "ax25.h"
#include "aprs.h"
#include "blink.h"
#include "shell.h"
#include "kiss.h"
#include "bt.h"
#include "pconfig.h"
#include "config.h"
#include "passthru.h"
#include "logger.h"

#define UART_TX_PIN 20    // Use GPIO 20 and 21 for v5 board
#define UART_RX_PIN 21
//#define UART_TX_PIN 4
//#define UART_RX_PIN 5

AX25Call n9600a = {
    "N9600A", 4, 0, false, false
};

AX25Call cqbeep5 = {
    "CQBEEP", 5, 0, false, false
};

uint8_t scratch[400];
KissState tncKiss;
KissState bleKiss;
KissState sppKiss;
KissFrame kissFrame;
ShellState shell;
LED led;
OLED oled;
AX25Packet packet;
char line[16]; // OLED text buffer

AX25Callback rx_callback;
AX25Callback tx_callback;

// Record last beacon data that was transmitted
bool saw_our_beacon = false;
Loc last_lat;
Loc last_lon;
AX25Call last_call;

uint8_t tnc_query_firmware[] = {FEND, KISS_N9600A_VER_CMD, 0x00, FEND};
bool n9600_present = false;
char n9600a_firmware_version[5];

// Only use this inside interrupts. Should only be used for coarse grained timing estimates since it 
// gets update in the main loop which has no timing guarantees.
uint32_t async_now_ms = 0;  
uint32_t async_now_us = 0;
uint32_t last_led_update_ms;
uint32_t led_refresh_ms = 3000;

int64_t init_tnc(alarm_id_t id, __unused void *user_data) {
    uart_write_blocking(UART_ID, tnc_query_firmware, 4);
    uart_tx_wait_blocking(UART_ID);
    return 0;
}

void pretty_uptime(char * buffer, uint32_t millis) {
    if (millis < 600000) {
        // seconds up to 10 mins (600s)
        sprintf(buffer, "%ds", millis / 1000);
    } else if (millis < 43200000) {
        // minutes up to 12 hours (720m)
        sprintf(buffer, "%dm", millis / 60000);
    } else if (millis < 2592000000) {
        // hours up to 30 days (720h)
        sprintf(buffer, "%dh", millis / 3600000);
    } else {
        // days
        sprintf(buffer, "%dd", millis / 86400000);
    }
}

void log_ax25_packet(AX25Packet packet, bool rx) {
    memset(scratch, '\0', 400);
    uint8_t pos = 0;
    pos += ax25_format_call(&scratch[pos], &(packet.source_call));
    scratch[pos++] = ' ';
    pos += ax25_format_call(&scratch[pos], &(packet.dest_call));
    scratch[pos++] = ' ';
    memcpy(&scratch[pos], kissFrame.buffer, kissFrame.len);
    if (rx) {
        logger_write_ascii(async_now_ms, "RX", scratch, strlen(scratch));
    } else {
        logger_write_ascii(async_now_ms, "TX", scratch, strlen(scratch));
    }
}

int64_t send_our_info_to_bt(alarm_id_t id, __unused void *user_data) {
    AX25Call dest_call = {"APZBLE", 0, 0, true, false};
    AX25Call src_call = {"N9600", 0, 0, false, true};
    char msg[62];
    memset(msg, '\0', 62);
    strcat(msg, "NinoTNC BLE ");
    //strcat(msg, " built on ");
    //strcat(msg, BUILD_DATE);
    
    char uptime[5];
    pretty_uptime(uptime, async_now_ms);
    strcat(msg, uptime);
    strcat(msg, " ");
    strcat(msg, GIT_SHORT_HASH);


    if (n9600_present) {
        strcat(msg, " ");
        strcat(msg, n9600a_firmware_version);
    }

    uint16_t len = aprs_status(kissFrame.buffer, &src_call, &dest_call, msg);
    kissFrame.len = len;
    kissFrame.hdlc = 0;
    kissFrame.type = KISS_DATA;
    len = kiss_encode(&kissFrame, scratch);
    handle_tnc_rx_data(scratch, len);
    return 0;
}

void ble_connection_callback(bool disconnected) {
    printf("ble_connection_callback(%d)\n", disconnected);
    if (!disconnected) {
        logger_write_ascii(async_now_ms, "BLE", "Connected", 10);
        add_alarm_in_ms(500, send_our_info_to_bt, NULL, true);
    } else {
        logger_write_ascii(async_now_ms, "BLE", "Disconnected", 13);
    }
}

void spp_pin_callback(uint32_t pin) {
    oled_clear(&oled);
    oled_write_string(&oled, 0, 0, "Bluetooth PIN");
    char pin_str[8];
    sprintf(pin_str, "%06lu", pin);
    oled_write_string(&oled, 5, 16, pin_str);
    oled_render(&oled);
}

void display_aprs_packet_oled(AX25Packet * packet, uint8_t * buffer, uint16_t len, bool rx) {
    oled_clear(&oled);
    memset(line, ' ', sizeof(line));
    if (rx) {
        memcpy(line, "RX", 2);
    } else {
        memcpy(line, "TX", 2);
    }
    AX25FrameType frame_type = ax25_frame_type(packet, buffer);
    bool found_aprs = false;
    char aprs_type;
    switch (frame_type) {
        case AX25_UNNUMBERED:
            AX25UnnumberedFrameType u_subtype = ax25_u_subtype(packet, buffer);
            if (u_subtype != AX25_U_UI) {
                memcpy(&line[3], "Not UI", 7);
                break;
            }
            AX25PidType pid_type = ax25_packet_pid_field(packet, buffer);
            if (pid_type != AX25_PID_NO_LAYER_3) {
                memcpy(&line[3], "Bad PID", 8);
                break;
            }
            aprs_type = aprs_data_type(packet, buffer);
            printf("APRS type: %02x\n", aprs_type);
            line[3] = aprs_type;
            if (aprs_type == APRS_TYPE_POS_TIME_NOMSG || aprs_type == APRS_TYPE_POS_TIME_MSG) {
                aprs_datetime_t dt = aprs_time(packet, buffer);
                sprintf(&line[7], "%02d:%02d:%02dZ", dt.hour, dt.min, dt.sec);
            }            
            found_aprs = true;
            break;
        case AX25_INFORMATION:
        case AX25_SUPERVISORY:
        default:
            memcpy(&line[3], "Not UI", 7);
            break;
    }
    oled_write_string(&oled, 0, 0, line);
    memset(line, ' ', sizeof(line));
    ax25_format_call(line, &(packet->source_call));

    oled_write_string(&oled, 0, 8, line);

    // If not APRS, bail out
    if (!found_aprs) {
        oled_render(&oled);
        return;
    }

    // If it looks like the Test TX, bail out 
    if (ax25_call_eq(&(packet->source_call), &n9600a) && ax25_call_eq(&(packet->dest_call), &cqbeep5)) {
        oled_write_string(&oled, 0, 16, "Test TX button");
        oled_write_string(&oled, 0, 24, n9600a_firmware_version);
        oled_render(&oled);

        add_alarm_in_ms(500, send_our_info_to_bt, NULL, true);
        return;
    }

    //aprs_comment(packet, buffer, len, scratch);
    memset(line, ' ', sizeof(line));

    // Parse the lat/lon
    Loc lat, lon;
    bool has_loc;
    if (aprs_type == APRS_TYPE_POS_NOTIME_NOMSG || aprs_type == APRS_TYPE_POS_NOTIME_MSG) {
        aprs_notime_latitude(packet, buffer, &lat);
        aprs_notime_longitude(packet, buffer, &lon);
        has_loc = true;
    } else if (aprs_type == APRS_TYPE_POS_TIME_NOMSG || aprs_type == APRS_TYPE_POS_TIME_MSG) {
        aprs_time_latitude(packet, buffer, &lat);
        aprs_time_longitude(packet, buffer, &lon);
        has_loc = true;
    } else {
        // ignore
        has_loc = false;
    }
    
    if (!rx) {
        // If BLE is sending a beacon, grab out callsign and position
        if (aprs_type == APRS_TYPE_POS_TIME_MSG || aprs_type == APRS_TYPE_POS_NOTIME_NOMSG) {
            saw_our_beacon = true;
            memcpy(&last_lat, &lat, sizeof(lat));
            memcpy(&last_lon, &lon, sizeof(lon));
            ax25_call_cpy(&last_call, &packet->source_call);
            printf("Update last call to %s-%d\n", last_call.callsign, last_call.ssid);
        }
    } else {
        // If receiving a beacon, print the lat/lon, distance, and heading
        if (has_loc) {
            // Print the lat/lon
            printf("lat: %d %d %d\n", lat.deg, lat.min, lat.sec);
            print_loc_dd(&lat, line);
            line[6] = ' ';
            printf("lon: %d %d %d\n", lon.deg, lon.min, lon.sec);
            print_loc_dd(&lon, &line[7]);
            oled_write_string(&oled, 0, 16, line);

            // Display the great circle distance
            if (saw_our_beacon) {
                double lat1_dd = loc_dd(&last_lat);
                double lon1_dd = loc_dd(&last_lon);
                double lat2_dd = loc_dd(&lat);
                double lon2_dd = loc_dd(&lon);
                double d_km = loc_distance(lat1_dd, lon1_dd, lat2_dd, lon2_dd);
                memset(line, ' ', sizeof(line));
                sprintf(line, "%0.2fkm", d_km);
                // TODO heading
                oled_write_string(&oled, 0, 24, line);
            }
        }
    }

    oled_render(&oled);
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
    display_aprs_packet_oled(packet, buffer, len, true);
}

void display_tx_packet(AX25Packet * packet, uint8_t * buffer, uint16_t len) {
    display_aprs_packet_oled(packet, buffer, len, false);
}


void wifi_led_toggle(bool on) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
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

/**
 * Called when BLE has data ready for us. This is called from the main loop.
 * 
 * Stream the data through the KISS parser to ensure a valid frame was received. 
 * This also allows us to handle a KISS packet that exceeds the BLE packet size.
 * 
 * @param buffer pointer to the BLE TX buffer
 * @param len    number of valid bytes in the buffer
 */
static void ble_write_callback(uint8_t * buffer, uint16_t len) {
    for (uint16_t i=0; i<len; i++) {
        //printf("BLE: %02X\n", buffer[i]);
        if (kiss_feed(&bleKiss, buffer[i], async_now_ms)) {
            kiss_read(&bleKiss, &kissFrame);

            // Copy the KISS frame into a separate buffer for writing to the UART
            uint16_t encoded_len = kiss_encode(&kissFrame, scratch);
            printf("Got valid KISS from BLE, sending to TNC\n");

            uart_write_blocking(UART_ID, scratch, encoded_len);
            uart_tx_wait_blocking(UART_ID);

            // decode and display the packet
            if (ax25_init(&packet, kissFrame.buffer, kissFrame.len)) {
                tx_callback(&packet, kissFrame.buffer,  kissFrame.len);
            }

            logger_write_pcap(async_now_ms, async_now_us, "TX", kissFrame.buffer,  kissFrame.len);
            log_ax25_packet(packet, false);

            kiss_init(&bleKiss, async_now_ms);
        }
    }

}

/**
 * Called when SPP has data ready for us. This is called from the BTStack thread.
 * 
 * Stream the data through the KISS parser to ensure a valid frame was received. 
 * 
 * @param buffer pointer to the SPP TX buffer
 * @param len    number of valid bytes in the buffer
 */
static void spp_write_callback(uint8_t * buffer, uint16_t len) {
    // TODO this is called from BTStack. Consider deferring the the UART write until our main loop
    for (uint16_t i=0; i<len; i++) {
        //printf("BLE: %02X\n", buffer[i]);
        if (kiss_feed(&sppKiss, buffer[i], async_now_ms)) {
            kiss_read(&sppKiss, &kissFrame);

            // Copy the KISS frame into a separate buffer for writing to the UART
            uint16_t encoded_len = kiss_encode(&kissFrame, scratch);
            printf("Got valid KISS from SPP, sending to TNC\n");
            
            uart_write_blocking(UART_ID, scratch, encoded_len);
            uart_tx_wait_blocking(UART_ID);

            // decode and display the packet
            if (ax25_init(&packet, kissFrame.buffer, kissFrame.len)) {
                tx_callback(&packet, kissFrame.buffer,  kissFrame.len);
            }

            logger_write_pcap(async_now_ms, async_now_us, "TX", kissFrame.buffer,  kissFrame.len);
            log_ax25_packet(packet, false);

            kiss_init(&sppKiss, async_now_ms);
        }
    }

}

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

static uint8_t spp_service_buffer[150];       // Used for SPP record

bool uart_readable() {
    return uart_is_readable(UART_ID);
}

uint32_t uart_read() {
    return uart_getc(UART_ID);
}

uint32_t uart_writer(const void * buffer, uint32_t len) {
    uart_write_blocking(UART_ID, buffer, len);
    //uart_tx_wait_blocking(UART_ID);
    return len;
}

bool check_passthru_enabled() {
    bool passthrough_sw = false;

    gpio_init(PASSTHRU_SW_PIN);
    gpio_set_dir(PASSTHRU_SW_PIN, 0); // input

    absolute_time_t time = get_absolute_time();
    uint32_t start_ms = to_ms_since_boot(time);
    uint32_t now_ms;
    while (gpio_get(PASSTHRU_SW_PIN)) {
        // If GPIO 26 is held high for 100ms, enter passthrough mode
        time = get_absolute_time();
        now_ms = to_ms_since_boot(time);
        if (now_ms - start_ms > 100) {
            passthrough_sw = true;
            break;
        }
    }

    return passthrough_sw;
}

void passthru_usb_entry(void) {
    while (1) {
        passthru_usb_read(tud_cdc_available, tud_cdc_read);
        passthru_usb_write(tud_cdc_write_available, tud_cdc_write, tud_cdc_write_flush);
    }
}

/**
 * Main run loop. After initializing peripherals, run a loop that checks for decoded UART
 * data or waiting BLE/SPP data. In either case, run a synchronous callback do relay the frame
 * to the appropriate device. 
 */
int run_main_loop(void) {
    // OLED
    oled_init(&oled);
    oled_clear(&oled);
    oled_write_string(&oled, 5, 0, "NinoTNC BLE");
    oled_write_string(&oled, 5, 8, "K4DBZ");
    oled_write_string(&oled, 5, 16, GIT_SHORT_HASH);
    oled_write_string(&oled, 5, 24, "TARPN.NET");
    oled_render(&oled);

    absolute_time_t now = get_absolute_time();
    async_now_ms = to_ms_since_boot(now);
    async_now_us = to_ms_since_boot(now);
    printf("init %d\n", async_now_ms);

    kiss_init(&tncKiss, async_now_ms);
    kiss_init(&bleKiss, async_now_ms);

    // BT setup
    l2cap_init();
    sm_init();
    sdp_init();
    
    // SPP setup
    memset(spp_service_buffer, 0, sizeof(spp_service_buffer));
    spp_create_sdp_record(spp_service_buffer, sdp_create_service_record_handle(), 1, "NinoTNC SPP");
    sdp_register_service(spp_service_buffer);
    gap_set_local_name("SPP KISS 00:00:00:00:00:00");
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_YES_NO);
    gap_discoverable_control(1);
    init_spp(spp_write_callback, NULL, spp_pin_callback);

    // BLE setup
    init_ble(ble_connection_callback);

    // UART setup
    init_uart(on_uart_rx, true);

    // SD setup
    logger_init();

    init_led(&led, wifi_led_toggle);

    async_now_ms = to_ms_since_boot(get_absolute_time());
    blink_morse(&led, async_now_ms, 'S');

    PackedConfig config;
    read_config_from_mem(&config);
    if (!check_config(&config)) {
        printf("config not initialized.\n");
        init_config(&config);
    }
    print_config(&config);

    rx_callback = display_rx_packet;
    tx_callback = display_tx_packet;

    // Query the TNC for its params
    add_alarm_in_ms(100, init_tnc, NULL, true);
    //add_alarm_in_ms(5000, send_our_info_to_bt, NULL, true);
    
    // this is a forever loop in place of where user code would go.
    while(true) {
        // Update this each loop to avoid wasting time inside interrupts.
        async_now_ms = to_ms_since_boot(get_absolute_time());
        if ((async_now_ms - last_led_update_ms) > led_refresh_ms) {
            if (has_connection()) {
                blink_morse(&led, async_now_ms, 'E');
                led_refresh_ms = 10000;
            } else {
                blink_morse(&led, async_now_ms, 'M');
                led_refresh_ms = 3000;
            }
            last_led_update_ms = async_now_ms;
        }
        check_led_state(&led, async_now_ms);

        bool did_work = false;
        // Check if TNC finished sending a KISS frame. If it's valid, pass it to BLE
        if (tncKiss.haveFrame) {
            kiss_read(&tncKiss, &kissFrame);
            printf("Got valid KISS from TNC\n");
            printf_hexdump(kissFrame.buffer, kissFrame.len);
            printf("\n");
            
            if (kissFrame.type == KISS_DATA && kissFrame.hdlc == KISS_N9600A_HDLC_PORT) {
                // Handle non-AX25 data from TNC
                memcpy(n9600a_firmware_version, kissFrame.buffer, MIN(kissFrame.len, 5));
                printf("Got info from TNC: %s\n", n9600a_firmware_version);
                logger_write_ascii(async_now_ms, "TNC", kissFrame.buffer, kissFrame.len);
                n9600_present = true;
                kiss_init(&tncKiss, async_now_ms);
            } else if (ax25_init(&packet, kissFrame.buffer, kissFrame.len)) {
                // Handle AX.25 data from TNC
                blink_morse(&led, async_now_ms, 'R');
                rx_callback(&packet, kissFrame.buffer,  kissFrame.len);
                logger_write_pcap(async_now_ms, async_now_us, "RX", kissFrame.buffer, kissFrame.len);
                log_ax25_packet(packet, true);

                // copy into BLE buffer
                uint16_t len = kiss_encode(&kissFrame, scratch);
                handle_tnc_rx_data(scratch, len);
                kiss_init(&tncKiss, async_now_ms);
                did_work = true;
            }            
        }

        // Check if the BLE has data waiting
        if (ble_has_tx_data()) {
            printf("BLE client has data for us\n");
            blink_morse(&led, async_now_ms, 'T');
            uint16_t len = handle_ble_tx_data(ble_write_callback);
            did_work = true;
        }

        // Check if the USB has data waiting
        while (tud_cdc_available()) {
            char byte = getchar();  // read from stdin (uart0/USB)
            if (shell_feed(&shell, byte)) {
                shell_complete(&shell);
                switch (shell.cmd) {
                    case CMD_HELP:
                        printf("This is the help output\n");
                        break;
                    case CMD_CONFIG:
                        printf("Entering config mode\n");
                        break;
                    case CMD_SAVE:
                        printf("Saving config to flash.\n");
                        break;
                    case CMD_UNKNOWN:
                    default:
                        printf("Unknown command: %s\n", shell.cmd_buffer);
                        break;
                }
                shell_init(&shell);
            }
            did_work = true;
        }

        // Avoid a tight loop in cases where nothing happened
        if (!did_work) {
            sleep_ms(1);
        }
    }

    logger_deinit();
    return 0;
}

/**
 * Program Entry Point. 
 * 
 * Only edit this very carefully. We don't want to mess with the passthru mode.
 */
int main() {
    // Minimal boot-up before checking run mode
    stdio_init_all();
    if (cyw43_arch_init()) {
        return -1;
    }

    if (!check_passthru_enabled()) {
        // If not in "passthru" mode, run the main loop
        return run_main_loop();
    } else {
        // If we detect the passthru mode, clear the OLED
        oled_init(&oled);
        oled_deinit(&oled);

        // Then flash the LED and leave it solid
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(50);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(50);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

        // Init the UART for passthru mode
        passthru_init(uart_readable, uart_read);
        init_uart(passthru_uart_irq, false);

        // Use second core for USB loop
        multicore_launch_core1(passthru_usb_entry);

        // Main core for UART loop
        while (1) {
            passthru_uart_loop(uart_writer);
        }
        return 0;
    }
}
