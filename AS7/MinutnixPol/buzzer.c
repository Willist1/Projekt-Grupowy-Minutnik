/*
 * buzzer.c
 *
 * Created: 3/14/2019 9:33:13 PM
 *  Author: stefan, peter
 */ 

#include <avr/io.h>

void buzzerInit() {			// using the same timer as PWM display brightness control
	DDRB &= ~_BV(PB1);		// pin OC1A set as input
	TCCR1A |= _BV(COM1A1);	// low output state at compare match
	OCR1A = 0x00;			// by default: 100% duty cycle (buzzer turned off)
}

void buzzerOn() {
	TCCR1A |= _BV(COM1A1);	// low output state at compare match
	DDRB |= _BV(PB1);		// pin OC1A set as output
}

void buzzerOff() {
	TCCR1A &= ~_BV(COM1A1);	// normal port operation
	DDRB &= ~_BV(PB1);		// pin OC1A set as input
}

void buzzerSetVolume(uint8_t setting) {

	switch (setting)
	{
		if (setting == 0) buzzerOff();
		else buzzerOn();
		
		case 0:
			OCR1A = 10*0;	// OCR1A max is 1023, duty cycle in range 0-50%
			break;
		case 1:
			OCR1A = 10*1;
			break;
		case 2:
			OCR1A = 10*4;
			break;
		case 3:
			OCR1A = 10*36;
			break;
		default:
			break;
	}
}