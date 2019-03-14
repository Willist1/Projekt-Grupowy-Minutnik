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
	uint8_t tmp=0xFF;
	// if((val & 0x7F)<11) tmp=DIGITS[val & 0x7F];  //Odczytaj kod znaku
	if((val & 0x7F) <= 0xF) tmp=DIGITS[val & 0x7F];  //Odczytaj kod znaku
	if(val & DP) tmp&=~DP;      //Sterowanie kropka dziesietna na LED
	LEDPORT=tmp;
}

ISR(TIMER0_OVF_vect)
{
	static uint8_t LEDNO;

	PORTC &= 0xFC;		//Wylacz wszystkie wywietlacze
	LEDNO=(LEDNO+1)%LEDDISPNO;
	ShowOnLED(LEDDIGITS[LEDNO]);
	PORTC = (PORTC & 0xFC) | ((1<<LEDNO) & 0x03);	//Wybierz kolejny wyswietlacz
}

ISR(TIMER0_COMPA_vect)
{
	PORTC &= 0xFC;	// Wylacz wszystkie wyswietlacze
}

void Timer0Init()
{
	TCCR0B=_BV(CS01 | CS00);	// Preskaler CLKIO/64  (przy 8MHz przelaczanie 488 Hz - powinno byc 600 Hz okolo)
	TIMSK0|=_BV(TOIE0);			// Odblokuj przerwanie nadmiaru timera 0
}

void Timer0InitWithDimmer()
{
	TIMSK0|=_BV(OCIE0A);	// Wlacz przerwanie Compare Match A
	OCR0A = MAX_BRIGHTNESS_VAL;
	Timer0Init();
}