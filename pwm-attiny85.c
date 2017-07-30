#include <avr/io.h>
#include <avr/interrupt.h>
 
int main (void)
{
  // set pin 6/PB1/OC0B to be output
  // Timer/Counter0 Compare Match  output
	DDRB = (1 << DDB1);

  // Table 11-5. Select fast PWM with a prescaler of 256. In mode 7
  // (fast PWM with TOP = OCRA), OCR0A is the TOP value and you can
  // only get a PWM on OC0B. Why would you want that? To control the
  // frequency as well as duty cycle.
  TCCR0A = (1 << COM0B1) | (1 << WGM00) | (1 << WGM01);
  TCCR0B = (1 << CS02)   | (1 << WGM02);

  // In mode 7, TCNT0 counts from 0 to OCR0A for a complete PWM cycle.
  // We want the time period to be 20ms.
  // OCR0A * tick_period = 20ms
  // OCR0A = 20ms/tick_period_in_ms = 20ms * tick_frequency_in_1000Hz
  // OCR0A = 20ms * (1000 KHz / 256)
  // OCR0A = 20 * F_CPU / 256000

  // I had to add a little extra to make the output come out exactly
  // 20ms. Why?
  OCR0A = (F_CPU / 256000) * 20 + 18;

  // 8% duty cycle.
  OCR0B = OCR0A * 0.08;
  
  while (1) {
  }
 
  return 0;
}
