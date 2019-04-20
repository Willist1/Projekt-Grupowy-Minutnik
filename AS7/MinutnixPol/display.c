/*
 * display.c
 *
 * Created: 3/10/2019 12:25:09 PM
 *  Author: stefan
 */ 

#include "display.h"
#include "TPIC6C596.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

void Timer1Init() {
	TCCR1A |= _BV(WGM11) | _BV(WGM10);	// fast PWM mode 10-bit (max 0x3FF = 1024)
	TCCR1B = _BV(WGM12) | _BV(CS11);	// prescaler 8 (16 MHz / 1024 / 8 = 2 kHz
}

void displayInit()
{
	DDRB &= ~_BV(PB2);		// pin OC1A set as input
	Timer1Init();
	TCCR1A |= _BV(COM1B1);	// low output state at compare match
	OCR1B = 0x3FF;			// by default: 100% duty cycle
	//OCR1B = 0x000;		// by default: 0% duty cycle
}

void displayOff()
{
	OCR1B = 0x000;			// by default: 0% duty cycle
	// TRY TPIC DEINIT
}

void displayOn()
{
	OCR1B = 1000;
	// TRY TPIC INIT
}

void displaySetBrightness(uint8_t percentage) {
	OCR1B = 1024-10*percentage;		// OE pin of shift register is inverted
}