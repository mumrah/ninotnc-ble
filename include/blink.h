#include <Arduino.h>

const uint8_t pulse_on_ms = 50;
const uint8_t pulse_off_ms = 100;

bool led_state = LOW;
unsigned long next_pulse_ms;

uint8_t pulse_timings[5];
uint8_t pulse_count;

const char *letters[] = {
  // The letters A-Z in Morse code  
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",    
  ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",  
  "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."         
};

void blink_morse(unsigned long now_ms, char letter)
{
  const char *morse = letters[letter - 'A'];
  pulse_count = strlen(morse);
  for (size_t i = 0; i<pulse_count; i++)
  {
    pulse_timings[i] = morse[pulse_count-i-1] == '.' ? pulse_on_ms : pulse_on_ms * 3;
  }
  next_pulse_ms = now_ms;
  led_state = LOW;
  digitalWrite(PD5, LOW);
}

void blink(unsigned long now_ms, uint8_t count, bool overwrite)
{
  // Overwrite any previous blinks
  if ((pulse_count == 0 && led_state == LOW) || overwrite)
  {
    next_pulse_ms = now_ms;
    pulse_count = count;
    for (size_t i = 0; i<pulse_count; i++)
      pulse_timings[i] = pulse_on_ms;
    led_state = LOW;
    digitalWrite(PD5, LOW);
  }
}

void blink_sync(uint8_t count)
{
  while(count-- > 0)
  {
      digitalWrite(PD5, HIGH);
      delay(pulse_on_ms);
      digitalWrite(PD5, LOW);
      delay(pulse_off_ms);
  }
}

void check_led_state(unsigned long now_ms)
{
  if (pulse_count > 0 && now_ms >= next_pulse_ms)
  {
    if (led_state == LOW)
    {
      digitalWrite(PD5, HIGH);
      led_state = HIGH;
      next_pulse_ms = now_ms + pulse_timings[pulse_count-1];
    }
    else
    {
      digitalWrite(PD5, LOW);
      led_state = LOW;
      next_pulse_ms = now_ms + pulse_off_ms;
      pulse_count--;
    }
  }
}