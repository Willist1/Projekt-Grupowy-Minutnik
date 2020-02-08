/*8 bit 
 * lightsensor.c
 *
 * Created: 4/27/2019 8:48:28 PM
 *  Author: stefa
 */ 

#include "lightsensor.h"
#include <avr/io.h>

#define VREF 5

#define MA_FILTER_LENGTH   16    // must be power of 2^n
#define MA_FILTER_SHIFT    4     // n from above comment

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

uint8_t lightsensorFilter( uint8_t input ) {
	
	static uint8_t filterMemory[MA_FILTER_LENGTH] = { [0 ... MA_FILTER_LENGTH-1] = DEFAULT_BRIGHTNESS }; 
	static uint8_t sampleIdx = 0;
	static uint16_t accumulator = MA_FILTER_LENGTH * DEFAULT_BRIGHTNESS;
	
	accumulator += input;
	if ( accumulator > filterMemory[sampleIdx] )
		accumulator -= filterMemory[sampleIdx];
	else
		accumulator = 0;
	
	filterMemory[sampleIdx] = input;
	sampleIdx++;
	if ( sampleIdx == MA_FILTER_LENGTH ) sampleIdx = 0;
	
	return ( accumulator >> MA_FILTER_SHIFT );
}