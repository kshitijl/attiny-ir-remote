#include "attiny85-util.h"

// Copies from pin 5 to pin 6 every 255usec.

const uint8_t input_pin  = DDB0; // pin 5
const uint8_t output_pin = DDB1; // pin 6

int main (void)
{
  clear_timers_and_io_pins();
  set_as_output_pin(output_pin);

  start_timer1_with_no_prescaler();

  enable_timer1_overflow_interrupt();

  sei();
  
  while (1) {

  }
 
  return 0;
}

ISR (TIMER1_OVF_vect) {
  if(read_pin(input_pin))
    set_pin(output_pin);
  else
    unset_pin(output_pin);
}
