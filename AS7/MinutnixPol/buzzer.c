/*
 * buzzer.c
 *
 * Created: 3/14/2019 9:33:13 PM
 *  Author: stefan
 */ 

#include <avr/io.h>

void buzzerInit() {
	DDRB &= ~_BV(PB2);								// pin OC1B set as input
	TCCR1A = _BV(COM1B1) | _BV(WGM11) | _BV(WGM10); // low output state at compare match, fast PWM mode 10-bit (max 0x3FF = 1024)
	// OCR1B = 0x3FF;									// by default: 100% duty cycle
	OCR1B = 0x000;									// by default: 0% duty cycle
	TCCR1B = _BV(WGM12) | _BV(CS11);				// prescaler 8 (16 MHz / 1024 / 8 = 2 kHz)
}

void buzzerOn() {
	DDRB |= _BV(PB2);								// pin OC1B set as output
}

void buzzerOff() {
	DDRB &= ~_BV(PB2);								// pin OC1B set as input
}

void buzzerSetVolume(uint8_t percentage) {
	OCR1B = 10*percentage;							// OCR1B max is 1024
}