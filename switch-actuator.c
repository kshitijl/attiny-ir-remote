#define F_CPU           1000000UL

#include "attiny85-util.h"

#define tick_size_in_us 512

// Toggles output state every time a pulse > 5ms is detected.
// Turns motor

const uint8_t input_pin  = PIN0; // pin 5
const uint8_t pwm_output_pin = PIN1; // pin 6
const uint8_t debug_out_pin = DDB3; // pin 2

// Number of timer overflows to block receipt of further messages.
// A second or 0.5 of cooloff seems appropriate.
const uint32_t cooloff_time = 500000 / tick_size_in_us;

enum receiver_state {
  STATE_IDLE,
  STATE_COOLING_OFF
};
typedef enum receiver_state receiver_state_t;

enum actuator_state {
  STATE_IDLE_ON,    // toggle comes in -> TURNING_OFF
  STATE_IDLE_OFF,   // toggle comes in -> TURNING_ON
  STATE_TURNING_ON, // ignore toggles. use timer to gradually turn on, then -> FROM_ON_TO_IDLE
  STATE_FROM_ON_TO_IDLE, // use timer to gradually go back to central position, then -> IDLE_ON
  STATE_TURNING_OFF,
  STATE_FROM_OFF_TO_IDLE  
};
typedef enum actuator_state actuator_state_t;

volatile uint8_t  pulse_length_so_far;
volatile uint32_t cooloff_counter;
volatile receiver_state_t current_receiver_state;
volatile actuator_state_t current_actuator_state;

/* Actual min and max are 2 and 8 */
const uint8_t duty_min = 4, duty_max = 6, duty_idle = 5;
int8_t movement_direction = 1;

void initialize_state() {
  pulse_length_so_far = 0;
  cooloff_counter     = 0;
  
  current_receiver_state = STATE_IDLE;
  current_actuator_state = STATE_IDLE_OFF;
}

void initialize_hardware() {
  clear_timers_and_io_pins();
  set_as_output_pin(pwm_output_pin);
  set_as_output_pin(debug_out_pin);
}

void configure_timer1_interrupt() {
  enable_timer1_overflow_interrupt();
  start_timer1(TIMER1_PRESCALER_2);

  sei();
}

void configure_pwm() {
  // In mode 7, TCNT0 counts from 0 to OCR0A for a complete PWM cycle.
  // We want the time period to be 20ms.
  // OCR0A * tick_period = 20ms
  // OCR0A = 20ms/tick_period_in_ms = 20ms * tick_frequency_in_1000Hz
  // OCR0A = 20ms * (1000 KHz / 256)
  // OCR0A = 20 * F_CPU / 256000

  put_timer0_in_fast_pwm_mode_with_top(TIMER0_PRESCALER_256,
                                       TIMER0_PIN1_ACTUALLY_6,
                                       20 * F_CPU / 256000);
}

void set_duty_ticks(uint8_t duty_in_ticks_not_percent) {
  if(duty_in_ticks_not_percent <= duty_max and
     duty_in_ticks_not_percent >= duty_min)
    OCR0B = duty_in_ticks_not_percent;
}

uint8_t get_current_duty() {
  return OCR0B;
}

void initialize_actuator() {
  set_duty_ticks(duty_idle);
}

int main (void) {
  initialize_hardware();
  initialize_state();

  configure_timer1_interrupt();
  configure_pwm();

  initialize_actuator();

  while (1) {
  }
 
  return 0;
}

uint32_t actuator_timer;
const uint32_t time_to_turn_horn = 300000/tick_size_in_us;

void set_actuator_timer() {
  actuator_timer = time_to_turn_horn;
}

void send_pulse_to_actuator_state_machine() {
  if(current_actuator_state == STATE_IDLE_ON) {
    set_duty_ticks(get_current_duty() - 1);
    current_actuator_state = STATE_TURNING_OFF;
    set_actuator_timer();
  }
  else if(current_actuator_state == STATE_IDLE_OFF) {
    set_duty_ticks(get_current_duty() + 1);
    current_actuator_state = STATE_TURNING_ON;
    set_actuator_timer();
  }
  // Ignore input pulses while in other states.
}

void do_pulse_received_action() {
  send_pulse_to_actuator_state_machine();
}

void enter_cooloff_state() {
  current_receiver_state = STATE_COOLING_OFF;
  cooloff_counter = cooloff_time;
}

void exit_cooloff_state() {
  current_receiver_state = STATE_IDLE;
  cooloff_counter = 0;
}

/*
  5ms = 5000us = 5000 cycles at 1MHz.
  
  With a prescaler of 2, an interrupt happens every 512 microseconds.
  5ms/512us = 9.7 interrupts to count out 5ms.
*/

const uint8_t accept_threshold = 10;

void advance_receiver_state(uint8_t sensor_value) {
  if(current_receiver_state == STATE_COOLING_OFF) {
    if(cooloff_counter == 0) {
      exit_cooloff_state();
    }
    else
      cooloff_counter--;
  }
  else if (current_receiver_state == STATE_IDLE) {
  
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

void advance_actuator_state() {
  /* If we are in an idle state, do nothing. */
  if(current_actuator_state == STATE_IDLE_ON or
     current_actuator_state == STATE_IDLE_OFF)
    return;

  /* If there is time left on the actuator timer, do nothing other
   * than decrementing that counter.
   */
  if(actuator_timer > 0)
    actuator_timer--;

  /* The timer has gone off. Transition to the next state. */
  else {
    if(current_actuator_state == STATE_TURNING_ON) {
      set_actuator_timer();

      if(get_current_duty() != duty_max) {
        set_duty_ticks(get_current_duty() + 1);
      } else {
        set_duty_ticks(duty_idle);
        current_actuator_state = STATE_FROM_ON_TO_IDLE;
      }
    }
    else if(current_actuator_state == STATE_TURNING_OFF) {
      set_actuator_timer();

      if(get_current_duty() != duty_min) {
        set_duty_ticks(get_current_duty() - 1);
      } else {
        set_duty_ticks(duty_idle);
        current_actuator_state = STATE_FROM_OFF_TO_IDLE;
      }      
    }
    else if(current_actuator_state == STATE_FROM_OFF_TO_IDLE) {
      current_actuator_state = STATE_IDLE_OFF;
    }
    else if(current_actuator_state == STATE_FROM_ON_TO_IDLE) {
      current_actuator_state = STATE_IDLE_ON;
    }
  }     
}

ISR (TIMER1_OVF_vect) {
  toggle_pin(debug_out_pin);
  advance_receiver_state(!(read_pin(input_pin)));
  advance_actuator_state();
}


void old_motor_code() {
  const int lower = 20, upper = 100;
  
  while (1) {
    for(int angle = lower; angle < upper; ++angle) {
      //OCR0B = angle_to_OCR0B(angle);
      _delay_ms(10);
    }
    //_delay_ms(1000);
    for(int angle = upper; angle > lower; --angle) {
      //OCR0B = angle_to_OCR0B(angle);
      _delay_ms(10);
    }
  }
}
