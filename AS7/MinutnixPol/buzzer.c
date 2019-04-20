/*
 * buzzer.c
 *
 * Created: 3/14/2019 9:33:13 PM
 *  Author: stefan
 */ 

#include <avr/io.h>
#include "display.h"

void buzzerInit() {
	DDRB &= ~_BV(PB1);		// pin OC1A set as input
	Timer1Init();
	TCCR1A |= _BV(COM1A1);	// low output state at compare match
	//OCR1A = 0x3FF;		// by default: 100% duty cycle
	OCR1A = 0x000;			// by default: 0% duty cycle
}

void buzzerOn() {
	DDRB |= _BV(PB1);								// pin OC1A set as output
}

void buzzerOff() {
	DDRB &= ~_BV(PB1);								// pin OC1A set as input
}

void buzzerSetVolume(uint8_t percentage) {
	OCR1A = 10*percentage;							// OCR1A max is 1024
}