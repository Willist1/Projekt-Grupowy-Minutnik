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
	TCCR1B = _BV(WGM12) | _BV(CS11);	// prescaler 8 (16 MHz / 1024 / 8 = 2 kHz)
}

void displayInit()
{
	DDRB &= ~_BV(PB1);		// pin OC1A set as input
	Timer1Init();
	//TCCR1A |= _BV(COM1A1);	// low output state at compare match
	OCR1A = 0x3FF;			// by default: 100% duty cycle (display turned off)
}

void displayOff()
{
	TCCR1A &= ~_BV(COM1A1);	// low output state at compare match disable
	DDRB |= _BV(PB1);		// pin OC0B set as output
	TPIC6C596_Set_OE();		// Disable output
}

void displayOn()
{
	TCCR1A |= _BV(COM1A1);	// low output state at compare match
}

void displaySetBrightness(uint8_t percentage) {
	OCR1A = (uint16_t)1023-10*percentage;		// OE pin of shift register is inverted
}