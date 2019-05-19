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
	TCCR0B = _BV(WGM02) | _BV(CS01) | _BV(CS00);	// TOP = OCR0A, prescaler 64 (16 MHz / 128 / 64 = 2 kHz)
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

void buzzerSetVolume(uint8_t percentage) {
	OCR1A = 51*(percentage/10);						// OCR0B max is OCR0A = 128, duty cycle in range 0-50% //OCR1A max is 1023
}