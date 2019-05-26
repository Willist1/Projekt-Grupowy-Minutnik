/*
 * buzzer.c
 *
 * Created: 3/14/2019 9:33:13 PM
 *  Author: stefan, peter
 */ 

#include <avr/io.h>

void Timer0Init() {
	TCCR1A |= _BV(WGM01) | _BV(WGM00);				// fast PWM mode 8-bit (TOP = OCR0A)
	OCR1A = 128;
	TCCR0B = _BV(WGM02) | _BV(CS01);				// TOP = OCR0A, prescaler 8 (16 MHz / 128 / 8 = 15.6 kHz)
}

void buzzerInit() {
	DDRB &= ~_BV(PB1);		// pin OC0B set as input
	//Timer0Init();
	TCCR1A |= _BV(COM1A1);	// low output state at compare match
	OCR1A = 0x00;			// by default: 100% duty cycle (buzzer turned off)
}

void buzzerOn() {
	DDRB |= _BV(PB1);								// pin OC0B set as output
}

void buzzerOff() {
	DDRB &= ~_BV(PB1);								// pin OC0B set as input
}

void buzzerSetVolume(uint8_t setting) {
	
	if (setting == 0) {
		TCCR1A &= ~(_BV(COM1A1) | _BV(WGM01) | _BV(WGM00));
		TCCR0B &= ~_BV(WGM02);		// disable PWM
		PORTB &= ~_BV(PB1);			// set pin OC0B low
	}
	else {
		TCCR1A |= _BV(COM1A1) | _BV(WGM01) | _BV(WGM00);
		TCCR0B = _BV(WGM02);		// enable PWM again
	}
	
	switch (setting)
	{
		case 1:
			OCR1A = 10*0;	// OCR1A max is 1023, duty cycle in range 0-50%
			break;
		case 2:
			OCR1A = 10*46;
			break;
		case 3:
			OCR1A = 10*36;
			break;
		default:
			break;
	}
}