#include <avr/io.h>
#include <util/delay.h>
 
int main (void)
{
  // set PB3 to be output
	DDRB = 0b00000010;
  while (1) {
    
    // flash# 1:
    // set PB3 high
    PORTB = 0b00000010; 
    _delay_ms(200);
    // set PB3 low
    PORTB = 0b00000000;
    _delay_ms(200);

    // flash# 2:
    // set PB3 high
    PORTB = 0b00000010; 
    _delay_ms(2000);
    // set PB3 low
    PORTB = 0b00000000;
    _delay_ms(2000);
  }
 
  return 1;
}
