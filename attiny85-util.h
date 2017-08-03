#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void set_all_pins_as_input_pins() {
  DDRB = 0;
}

void clear_and_stop_timers() {
  TCCR0A = 0;
  TCCR0B = 0;
  TCCR1 = 0;
  TIMSK = 0;
}

void clear_timers_and_io_pins() {
  set_all_pins_as_input_pins();
  clear_and_stop_timers();
}

void set_pin(const uint8_t which_pin) {
  PORTB |= (1 << which_pin);
}

void unset_pin(const uint8_t which_pin) {
  PORTB &= ~(1 << which_pin);
}

void toggle_pin(const int which_pin) {
  PORTB ^= (1 << which_pin);
}

uint8_t read_pin(const uint8_t which_pin) {
  if((PINB >> which_pin) & 1) {
    return 0xFF;
  }
  return 0;    
}

void set_as_output_pin(const uint8_t which_pin) {
  DDRB |= (1 << which_pin);
}

void put_timer1_in_ctc_mode(const uint8_t compare_value) {
  TCCR1 = (1 << CTC1);
  
  OCR1C = compare_value;
}

enum timer0_prescaler_mode {
  TIMER0_PRESCALER_256 = CS02
};
typedef enum timer0_prescaler_mode timer0_prescaler_mode_t;

enum timer0_output_pin {
  TIMER0_PIN1_ACTUALLY_6 = COM0B1
};
typedef enum timer0_output_pin timer0_output_pin_t;

void
put_timer0_in_fast_pwm_mode_with_top(timer0_prescaler_mode_t which_mode,
                                     timer0_output_pin_t which_pin,
                                     uint8_t count_up_to) {
  TCCR0A = (1 << which_pin) | (1 << WGM00) | (1 << WGM01);
  TCCR0B = (1 << which_mode)| (1 << WGM02);

  OCR0A = count_up_to;
}

void timer0_pwm_set_duty_cycle(uint8_t duty) {
  OCR0B = duty*OCR0A/100;
}
                                          

uint8_t TIMER1_OFF = ~((1 << CS10) |
                       (1 << CS11) |
                       (1 << CS12) |
                       (1 << CS13));

void turn_timer1_off() {
  TCCR1 &= TIMER1_OFF;
}

enum timer1_prescaler_mode 
{ TIMER1_PRESCALER_1   = (1 << CS10),
  TIMER1_PRESCALER_2   = (1 << CS11),
  TIMER1_PRESCALER_4   = ((1 << CS10) |
                          (1 << CS11)),
  TIMER1_PRESCALER_8   = (1 << CS12),
  TIMER1_PRESCALER_16  = ((1 << CS10) |
                          (1 << CS12)),
  TIMER1_PRESCALER_32  = ((1 << CS11) |
                          (1 << CS12)),
  TIMER1_PRESCALER_64  = ((1 << CS10) |
                          (1 << CS11) |
                          (1 << CS12) ),
  TIMER1_PRESCALER_128 = (1 << CS13)
       // TODO do the rest
};
typedef enum timer1_prescaler_mode timer1_prescaler_mode_t;

void start_timer1(timer1_prescaler_mode_t mode) {
  /* This takes 4 instructions. Hopefully you aren't dynamically
   * changing the prescaler all that often.
   */
  turn_timer1_off();
  TCCR1 |= mode;
}

void enable_timer1_overflow_interrupt() {
  TIMSK |= (1 << TOIE1);
}
