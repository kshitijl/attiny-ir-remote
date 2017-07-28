#include <avr/io.h>
#include <avr/interrupt.h>
#include <Servo.h>

Servo knob_servo;

const int timeout = 500;

void setup() {
  pinMode(9, OUTPUT);
  knob_servo.attach(9);
  
  Serial.begin(9600);
  pinMode(2, INPUT);

  OCR2A = 100;

  TCCR2A = (1 << WGM21);
  // Set to CTC Mode

  TIMSK2 = (1 << OCIE2A);
  //Set interrupt on compare match

  TCCR2B = (1 << CS22);
  // https://sites.google.com/site/qeewiki/books/avr-guide/timers-on-the-atmega328
  // set prescaler to 64 and starts PWM

  TCNT2 = 0;

  sei();
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

int volume = 1;
bool muted = false;

void volume_up() {
  if(volume < 180)
    volume+=5;
}

void volume_down() {
  if(volume >= 6)
    volume -= 5;
  else
    volume = 1;
  // Servo jitters at 0
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

void interpret_message() {
  int message_length = message_index;
  int decoded = 0;

  for(int ii = 0; ii < message_length; ii += 2) {
    int ll = message[ii];
    int bit = 0;
    // Serial.println(ll);
      
    if(ll > 750 and ll < 850) 
      bit = 1;
    if(ll > 350 and ll < 450)
      bit = 1;

    decoded += (bit << ii/2);
  }

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
    cli(); // delet this bug
    interpret_message();
    write_knob();
    current_state = STATE_IDLE;
    sei(); // delet
  }
    
  write_knob();
  delay(1);
}


ISR (TIMER2_COMPA_vect)  // timer0 overflow interrupt
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
