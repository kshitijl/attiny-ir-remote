#include <avr/io.h>
#include <avr/interrupt.h>
 
int main (void)
{
  // set pin 6/PB1/OC0B to be output
  // set pin 5/PB0/OC0A to be output
	DDRB = (1 << DDB1) | (1 << DDB0);

  // Toggle OC0B on compare match with OCR0B.
  // Toggle OC0A on compare match with OCR0A.
  TCCR0A = (1 << COM0B0) | (1 << COM0A0);

  // CTC mode.
  TCCR0A |= (1 << WGM01);

  // Prescaler: 256
  TCCR0B = (1 << CS02);

  // 20ms period
  OCR0A = ((F_CPU / 1000) * 20 / 256) / 2;
    
  while (1) {
  }
 
  return 0;
}
