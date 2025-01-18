#ifndef CONFIG_H
#define CONFIG_H

#define UART_ID     uart1
#define BAUD_RATE   57600
#define DATA_BITS   8
#define STOP_BITS   1
#define PARITY      UART_PARITY_NONE

#define PASSTHRU_SW_PIN 6
#define SSD1306_ROT 1  // Any other value will rotate the screen 180º

#endif