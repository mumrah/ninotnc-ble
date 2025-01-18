#ifndef LED_H
#define LED_H

#include <string.h>

const uint8_t pulse_on_ms = 50;
const uint8_t pulse_off_ms = 100;

// The letters A-Z in Morse code  
const char *letters[] = {
// A       B       C       D       E      F       G       H       I
  ".-",   "-...", "-.-.", "-..",  ".",   "..-.", "--.",  "....", "..", 
// J       K       L       M       N      O       P       Q       R
  ".---", "-.-",  ".-..", "--",   "-.",  "---",  ".--.", "--.-", ".-.",
// S       T       U       V       W      X       Y       Z
  "...",  "-",    "..-",  "...-", ".--", "-..-", "-.--", "--.."        
};

typedef void (*LEDCallback)(bool led_state);


typedef struct {
    bool led_state;
    LEDCallback led_callback;
    uint32_t next_pulse_ms;
    uint8_t pulse_timings[5];
    uint8_t pulse_count;
} LED;

void init_led(LED * led, LEDCallback led_callback) {
    led->led_state = false;
    led->led_callback = led_callback;
    led->next_pulse_ms = 0;
    led->pulse_count = 0;
    led_callback(false);
}

/**
 * Set internal LED state to asynchronously blink out a morse code letter. Optionally repeat this every N ms.
 */
void blink_morse(LED * led, uint32_t now_ms, char letter) {
  const char *morse = letters[letter - 'A'];
  led->pulse_count = strlen(morse);
  for (size_t i = 0; i<led->pulse_count; i++)
  {
    led->pulse_timings[i] = morse[led->pulse_count-i-1] == '.' ? pulse_on_ms : pulse_on_ms * 3;
  }
  led->next_pulse_ms = now_ms;
  led->led_state = false;
  led->led_callback(false);
}

bool blink(LED * led, uint32_t now_ms, uint8_t count, bool overwrite)
{
  // Overwrite any previous blinks
  if ((led->pulse_count == 0 && led->led_state == false) || overwrite)
  {
    led->next_pulse_ms = now_ms;
    led->pulse_count = count;
    for (size_t i = 0; i<led->pulse_count; i++)
      led->pulse_timings[i] = pulse_on_ms;
    led->led_state = false;
    led->led_callback(false);
  }
}

/*
bool blink_sync(LED * led, uint8_t count) {
    while(count-- > 0) {
        led->led_callback(true);
        sleep_ms(pulse_on_ms); // How to avoid library dep on sleep?
        led->led_callback(false);
        sleep_ms(pulse_off_ms);
    }
    led->led_state = false;
}
*/

bool check_led_state(LED * led, uint32_t now_ms)
{
    if (led->pulse_count > 0 && now_ms >= led->next_pulse_ms)
    {
        if (led->led_state == false)
        {
            led->led_state = true;
            led->led_callback(true);
            led->next_pulse_ms = now_ms + led->pulse_timings[led->pulse_count-1];
        }
        else
        {
            led->led_state = false;
            led->led_callback(false);
            led->next_pulse_ms = now_ms + pulse_off_ms;
            led->pulse_count--;
        }
    }
}

#endif
