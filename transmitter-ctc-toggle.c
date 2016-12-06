#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//avrdude -c arduino -p atmega328 -P COMPORT -b 19200 -U flash:w:filetoburn.hex

volatile int press_count = 2;

void main() {
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
  OCR0A = 104;

  // Table 11-6, row 2. Start timer with no prescaling. Set CS00 to 1 in
  // TCCR0B.
  TCCR0B = (1 << CS00);  

  GIMSK = 0b00100000; // turn on pin change interrupts 
  PCMSK = 0b00000010; // turn on interrupts on PB1/PCINT1/pin 6
  sei();              // enable interrupts

  int length_one = 25, length_zero = 10,
    length_timeout = 100, length_space = 10;
  
  for(;;) {

    for(int ii = 0; ii < 8; ++ii) {
      int bit = (press_count/2) & (1 << ii);
      // Turn LED on
      TCCR0A |=  (1 << COM0A0);
      if(bit)
        _delay_ms(length_one);
      else
        _delay_ms(length_zero);
      
      // Turn LED off
      TCCR0A &= ~(1 << COM0A0); 
      _delay_ms(length_space);
    }

    _delay_ms(length_timeout);
  }

}

ISR(PCINT0_vect) {
  press_count += 1;
}
