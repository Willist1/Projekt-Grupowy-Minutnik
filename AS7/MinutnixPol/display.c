/*
 * display.c
 *
 * Created: 3/10/2019 12:25:09 PM
 *  Author: stefan
 */ 

#include "display.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

static inline void ShowOnLED(uint8_t val)
{
	uint8_t tmp = 0x00;
	if((val & 0x7F) <= 0xF) tmp = DIGITS[val & 0x7F];	// Read symbol code
	
	if(val & DP) PORTC |= 0x08;							// DP control
	else PORTC &= 0xF7;
	
	PORTB = (PORTB &= 0xE4) | (tmp & 0x1B);
	PORTC = (PORTC &= 0xF8) | ((tmp >> 5) & 0x07);
}

ISR(TIMER0_OVF_vect)
{
	static uint8_t LEDNO;
	PORTD &= 0xCF;		// set PD4-5 low (turn off displays)
	LEDNO=(LEDNO+1)%LEDDISPNO;
	ShowOnLED(LEDDIGITS[LEDNO]);
	PORTD = (PORTD &= 0xCF) | (1<<(LEDNO+4));	// Choose next display (either PB4 or PB5 set high)
}

ISR(TIMER0_COMPA_vect)
{
	PORTC &= 0xFC;	// Wylacz wszystkie wyswietlacze
}

static void Timer0Init()
{
	OCR0A   = MAX_BRIGHTNESS_VAL;	// Set max brightness as default
	TIMSK0 |=_BV(OCIE0A);			// Turn on Timer0 Compare Match A interrupt
	TCCR0B  =_BV(CS01 | CS00);		// Preskaler CLKIO/64  (for 8MHz switching freq 488 Hz - should be min 600 Hz)
	TIMSK0 |=_BV(TOIE0);			// Turn on Timer0 Overflow interrupt
}

static void displayIOInit()
{
	DDRB |= 0x1B;	// set PB1-2 + PB3-4 as outputs (segment a-d)
	DDRC |= 0x0F;	// set PC0-3 as outputs (segment e-g + DP)
	DDRD |= 0x30;	// set PD4-5 as outputs (digit 1 & 2)
	PORTD &= 0xCF;	// set PD4-5 low (turn off displays)
}

void displayInit()
{
	displayIOInit();
	Timer0Init();
}

void displaySetBrightness(uint8_t percentage) {
	OCR0A = percentage;
}