#ifndef OLED_H
#define OLED_H

#ifndef SSD1306_HEIGHT
#define SSD1306_HEIGHT              32
#endif 

#ifndef SSD1306_WIDTH
#define SSD1306_WIDTH               128
#endif

#define SSD1306_PAGE_HEIGHT         _u(8)
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

typedef struct {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;
    uint8_t buffer[SSD1306_BUF_LEN];
    int render_buflen;
} OLED;

void oled_init(OLED * oled);

void oled_deinit(OLED * oled);

void oled_clear(OLED * oled);

void oled_write_char(OLED * oled, int16_t x, int16_t y, char ch);

void oled_write_string(OLED * oled, int16_t x, int16_t y, char * str);

void oled_render(OLED * oled);

#endif