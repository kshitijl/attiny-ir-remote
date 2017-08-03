#include "attiny85-util.h"

#define tick_size_in_us 512

// Toggles output pin every time a pulse > 5ms is detected.

const uint8_t input_pin  = PIN0; // pin 5
const uint8_t output_pin = PIN1; // pin 6
const uint8_t clock_out_pin = DDB3; // pin 2

const uint8_t cooloff_pin = DDB4; // pin 3

// Number of timer overflows to block receipt of further messages.
// A second or two of cooloff seems appropriate.
const uint32_t cooloff_time = 1000000 / tick_size_in_us;

enum receiver_state {
  STATE_IDLE,
  STATE_COOLING_OFF
};
typedef enum receiver_state receiver_state_t;

volatile uint8_t  pulse_length_so_far;
volatile uint32_t cooloff_counter;
volatile receiver_state_t current_state;

void initialize_state() {
  pulse_length_so_far = 0;
  cooloff_counter     = 0;
  
  current_state = STATE_IDLE;  
}

void initialize_hardware() {
  clear_timers_and_io_pins();
  set_as_output_pin(output_pin);
  set_as_output_pin(clock_out_pin);
  set_as_output_pin(cooloff_pin);
}

void configure_timer_interrupt() {
  enable_timer1_overflow_interrupt();
  start_timer1(TIMER1_PRESCALER_2);

  sei();
}

int main (void) {
  initialize_hardware();
  initialize_state();
  configure_timer_interrupt();
  
  while (1) {
  }
 
  return 0;
}

void do_pulse_received_action() {
  toggle_pin(output_pin);
}

void enter_cooloff_state() {
  current_state = STATE_COOLING_OFF;
  cooloff_counter = cooloff_time;
  set_pin(cooloff_pin);
}

void exit_cooloff_state() {
  current_state = STATE_IDLE;
  cooloff_counter = 0;
  unset_pin(cooloff_pin);
}

/*
  5ms = 5000us = 5000 cycles at 1MHz.
  
  With a prescaler of 2, an interrupt happens every 512 microseconds.
  5ms/512us = 9.7 interrupts to count out 5ms.
*/

const uint8_t accept_threshold = 10;

ISR (TIMER1_OVF_vect) {
  toggle_pin(clock_out_pin);
  
  if(current_state == STATE_COOLING_OFF) {
    if(cooloff_counter == 0) {
      exit_cooloff_state();
    }
    else
      cooloff_counter--;
  }
  else if (current_state == STATE_IDLE) {
    uint8_t sensor_value = !(read_pin(input_pin));
  
    if(sensor_value) {
      if(pulse_length_so_far >= accept_threshold) {
        do_pulse_received_action();

        pulse_length_so_far = 0;

        enter_cooloff_state();      
      } else
        pulse_length_so_far++;
    }
    else {
      pulse_length_so_far = 0;
    }
  }
}
