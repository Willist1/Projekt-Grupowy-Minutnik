/*
 * buzzer.c
 *
 * Created: 3/14/2019 9:33:13 PM
 *  Author: stefan
 */ 

#include <avr/io.h>

void Timer0Init() {
	TCCR0A |= _BV(WGM01) | _BV(WGM00);				// fast PWM mode 8-bit (TOP = OCR0A)
	OCR0A = 128;
	TCCR0B = _BV(WGM02) | _BV(CS01) | _BV(CS00);	// TOP = OCR0A, prescaler 64 (16 MHz / 128 / 64 = 2 kHz)
}

void buzzerInit() {
	DDRD &= ~_BV(PD5);		// pin OC0B set as input
	Timer0Init();
	TCCR0A |= _BV(COM0B1);	// low output state at compare match
	OCR0B = 0x00;			// by default: 100% duty cycle (display turned off)
}

void buzzerOn() {
	DDRD |= _BV(PB5);								// pin OC0B set as output
}

void buzzerOff() {
	DDRD &= ~_BV(PB5);								// pin OC0B set as input
}

void buzzerSetVolume(uint8_t percentage) {
	OCR0B = 6*(percentage/10);						// OCR0B max is OCR0A = 128, duty cycle in range 0-50%
}