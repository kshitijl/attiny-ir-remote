// This file is WIP

#include "attiny85-util.h"

// Toggles output pin every time an NEC packet is received.

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

enum { STATE_IDLE,
       STATE_POSSIBLY_PRELUDE_MARK,
       STATE_POSSIBLY_PRELUDE_SPACE,
       STATE_IN_PACKET_MARK,
       STATE_IN_PACKET_SPACE,
       STATE_MESSAGE_READY
};
volatile int current_state = STATE_IDLE;

ISR (TIMER1_OVF_vect) {
  uint8_t sensor_value = ~(read_pin(input_pin));

  /*
    We will transition from IDLE to POSSIBLY_PRELUDE_MARK
    indiscriminately, but only transition to POSSIBLY_PRELUDE_SPACE if
    we were in POSSIBLY_PRELUDE_MARK for 10ms or so.

    We will transition from POSSIBLY_PRELUDE_SPACE to IN_PACKET_MARK
    only if we were in that state for 3ms or so.

    The preludes help us synchronize our clock: length and
    edge-position.
   */
  
}
