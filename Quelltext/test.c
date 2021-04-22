
           


#define F_CPU 8000000UL	/* CPU clock in Hertz */
#include <inttypes.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Display2x8.h"




//--------------------------------------------------------
int main(void){
//--------------------------------------------------------


U8 i;

  DDRB= BIT0| BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6;
  DDRC= 0;
  DDRD= BIT0| BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7;

  PORTD=0; //HOLD fuer den TLE4267 auf GND



_delay_ms(500);
	lcdInit();
	_delay_ms(100);

	//clearLcd();
	_delay_ms(100);


	while(1){
	i++;

	_delay_ms(100);
	sendStringToLCD("Hello World ");
	home2Lcd();
	sendStringToLCD(i);
	_delay_ms(1000);
	clearLcd();
	}

}
