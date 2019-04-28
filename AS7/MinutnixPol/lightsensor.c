/*8 bit 
 * lightsensor.c
 *
 * Created: 4/27/2019 8:48:28 PM
 *  Author: stefa
 */ 

#include "lightsensor.h"
#include <avr/io.h>

#define VREF 5

void lightsensorInit() {
	ADMUX = _BV(REFS0) | _BV(ADLAR);	// Reference voltage equal to AVcc, multiplexer input 0, align result to the left (8-bit ADC)
	DIDR0 = _BV(ADC0D);					// Turn off digital function of pin nr 0
	ADCSRB = 0;							// Free running mode
	ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
	// Turn on ADC and external trigger, prescaler ADC - 128 (16 MHz / 128)
	ADCSRA |= _BV(ADSC);				// Start conversions in free running mode
}

uint8_t lightsensorRead() {
	return ADCH;
}

void lightsensorSuspend() {
	ADCSRA &= ~(_BV(ADEN) | _BV(ADATE));	// Turn off ADC and external trigger
}

void lightsensorResume() {
	ADCSRA |= _BV(ADEN) | _BV(ADATE);	// Turn on ADC and external trigger
	ADCSRA |= _BV(ADSC);				// Start conversions in free running mode
}