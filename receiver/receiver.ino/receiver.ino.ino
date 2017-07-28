#include <avr/io.h>
#include <avr/interrupt.h>
#include <Servo.h>

// #define DEBUG_SERIAL 1

Servo knob_servo;

const int timeout = 1500;

void setup() {
  #ifdef DEBUG_SERIAL
  Serial.begin(9600);
  #endif
  
  
  pinMode(2, INPUT);
  // Set the Timer Mode to CTC
    TCCR0A = (1 << WGM01);

    // Set the value that you want to count to
    OCR0A = 1;

    TIMSK0 = (1 << OCIE0A);    //Set the ISR COMPA vect

    sei();         //enable interrupts


    TCCR0B = (1 << CS01) | (1 << CS00);
    // set prescaler to 256 and start the timer

    sei();
    // enable interrupts

     knob_servo.attach(7);

}

enum { STATE_IDLE, STATE_MARK, STATE_SPACE, STATE_MESSAGE_READY };
volatile int current_state = STATE_IDLE;

int read_sensor() {
  // https://www.arduino.cc/en/Reference/PortManipulation
  int val = PIND & _BV(PB2); //digitalRead(2);
  if(val == LOW)
      return HIGH;
   return LOW;
}

const int MAX_MESSAGE_LENGTH = 50;

volatile int counter = 0;
volatile int message[MAX_MESSAGE_LENGTH];
volatile int message_index = 0;

int volume = 5;
bool muted = false;

void volume_up() {
  #ifdef DEBUG_SERIAL
  Serial.println("Volume up");
  #endif
  volume = min(150, volume+1);
 //volume = 100;
}

void volume_down() {
  #ifdef DEBUG_SERIAL
  Serial.println("Volume down");
  #endif
  volume = max(5, volume-1);
}

void toggle_mute() {
    muted = !muted;
}

void write_knob() {
  if(muted)
      knob_servo.write(1);
   else
       knob_servo.write(volume);
}

enum { MSG_VOLUME_UP = 25, MSG_VOLUME_DOWN = 98,
       MSG_TOGGLE_MUTE = 121 };

bool is_close_to(int value, int target) {
  return abs(value-target) < target/5;
}

const int transmitter_millisecond = 125;
const int length_of_one = 5 * transmitter_millisecond;
const int length_of_zero 250;

void interpret_message() {
  int message_length = message_index;
    int decoded = 0;

    #ifdef DEBUG_SERIAL 
      Serial.print("Message length:");
      Serial.println(message_length);
      #endif

    for(int ii = 0; ii < message_length; ii += 2) {
      int ll = message[ii];
      int bit = 0;
      #ifdef DEBUG_SERIAL 
      Serial.print(ll);
      Serial.print(" ");
      #endif
      
      if(IS_CLOSE_TO(ll, LENGTH_OF_ONE)) 
          bit = 1;
       if(IS_CLOSE_TO(ll, LENGTH_OF_ZERO))
          bit = 0;

       decoded += (bit << ii/2);
    }

      #ifdef DEBUG_SERIAL 
      Serial.println("\n");
      Serial.print(decoded);
      Serial.flush();
      #endif    

    if(decoded == MSG_VOLUME_UP) {
      volume_up();
    }
  
    if(decoded == MSG_VOLUME_DOWN)
      volume_down();

    if(decoded == MSG_TOGGLE_MUTE)
      toggle_mute();
}

void loop() {
    if(current_state == STATE_MESSAGE_READY) {
    cli();
    interpret_message();
    write_knob();
    current_state = STATE_IDLE;
    sei();
  }
    
  write_knob();

}


ISR (TIMER0_COMPA_vect)  // timer0 overflow interrupt
{
  int value = read_sensor();

  if(current_state == STATE_MESSAGE_READY)
      return;

  if(message_index >= MAX_MESSAGE_LENGTH)
      message_index = 0;
  
  if(value == HIGH) {
      if(current_state == STATE_IDLE) {
          current_state = STATE_MARK;
          counter = 0;
          message_index = 0;
      }
      else if(current_state == STATE_MARK)
          counter++;
      else if(current_state == STATE_SPACE) {
        current_state = STATE_MARK;
        message[message_index++] = counter;
        counter = 0;
      }
  }
  else {
    if(current_state == STATE_MARK) {
      current_state = STATE_SPACE;
      message[message_index++] = counter;
      counter = 0;
    }
    else if(current_state == STATE_SPACE)
      counter++;
    if(counter > timeout and current_state != STATE_IDLE) {
      current_state = STATE_MESSAGE_READY;
    }
  }
}


