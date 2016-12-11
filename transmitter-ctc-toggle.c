#include <avr/io.h>
#include <util/delay.h>

#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdint.h>

volatile int volume_up = 0, volume_down = 0;

void send_message(uint8_t message) {
  const int length_one = 5, length_zero = 2,
    length_timeout = 100, length_space = 2;
  for(int ii = 0; ii < 8; ++ii) {
    // Turn LED on
    TCCR0A |=  (1 << COM0A0);
    if(message & (1 << ii))
      _delay_ms(length_one);
    else
      _delay_ms(length_zero);

    // Turn LED off
    TCCR0A &= ~(1 << COM0A0);
    _delay_ms(length_space);
  }

  _delay_ms(length_timeout);
}

enum { MSG_VOLUME_UP = 25, MSG_VOLUME_DOWN = 98,
       MSG_TOGGLE_MUTE = 121 };

enum { BUTTON_WAS_PRESSED, BUTTON_WASNT_PRESSED };

int read_pins_queue_messages() {
  uint8_t pins = PINB;

  if(pins & (1 << PB1)) {
    volume_up = 3;
    return BUTTON_WAS_PRESSED;
  }
  else if(pins & (1 << PB2)) {
    volume_down = 3;
    return BUTTON_WAS_PRESSED;
  }

  return BUTTON_WASNT_PRESSED;
}

void go_to_sleep() {
  // https://bigdanzblog.wordpress.com/2014/08/10/attiny85-wake-from-sleep-on-pin-state-change-code-example/

  // Section 9.3.2. Turn on pin change interrupts
  GIMSK = (1 << PCIE);

  // Section 9.3.4. Turn on pin change interrupts on PB1/PCINT1/pin 6
  // and PB2/PCINT2/pin 7.  
  PCMSK = (1 << PCINT1) | (1 << PCINT2);

  // Turn off ADC.
  ADCSRA &= ~_BV(ADEN);

  // We want super low power down mode.
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  sleep_enable();
  sei();
  sleep_cpu();

  // We get here once we wake from sleep.

  // Entering critical section - disable interrupts.
  cli();

  // Turn off pin change interrupts.
  PCMSK &= (~(1 << PCINT1)) & (~(1 << PCINT2));
  sleep_disable();

  // Turn on ADC.
  ADCSRA |= _BV(ADEN);

  // Enable interrupts.
  sei();
}

void setup_38khz_carrier() {
  DDRB = (1 << PB0); // set LED pin as an output pin

  // Table 11-2, row 2. Configure OC0A (pin 0) to be toggled on compare match.
  // Set COM0A0 to 1, COM0A1 to 0.
  TCCR0A = (1 << COM0A0);

  // Table 11-5, row 3. CTC mode. Set WGM01 to 1, WGM00 and WGM02 to
  // 0.
  TCCR0A |= (1 << WGM01);

  // Section 11.9.5. Set the CTC match value. Timer will run this many
  // ticks at 8Mhz (because we didn't use any prescaling) before
  // toggling pin 0. We want 38KHz, and the LED needs to be toggled
  // both on then off. So we want to toggle every 8Mhz/(2*38Khz) = 105
  // ticks. Since the timer counter starts at 0, we subtract 1 from
  // this to get our match value.
  OCR0A = 105;

  // Table 11-6, row 2. Start timer with no prescaling. Set CS00 to 1 in
  // TCCR0B.
  TCCR0B = (1 << CS00);
}

void main() {
  setup_38khz_carrier();
  
  for(;;) {
    if(read_pins_queue_messages() == BUTTON_WASNT_PRESSED &&
       volume_up == 0 && volume_down == 0)
      go_to_sleep();
    
    if(volume_up > 0) {
      send_message(MSG_VOLUME_UP);
      volume_up--;
    }
    else if (volume_down > 0) {
      send_message(MSG_VOLUME_DOWN);
      volume_down--;
    }

    _delay_ms(1);
  }
}


ISR(PCINT0_vect) {
  // Called on pin-change, after waking from sleep.
}
