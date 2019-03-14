/*
 *  main.c
 *
 *  Created: 2019-03-09 15:34
 *  Authors: Kluczek, Wegrzyn, Kowalczyk
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "uart.h"
#include "display.h"
#include "i2c.h"
#include "PCF8574.h"
#include "encoder.h"
#include "buzzer.h"

ISR(INT0_vect)
{
	PCF8574_INT = true;
}

void IO_Init() {
	
	// piny PD0-PD1 oraz PD3-PD7 portu D sa wyjsciem (sygnaly ustawiajace wskazywana liczbe)
	// pin PD2 portu D jest wejsciem (sygnal external interrupt od ekspandera)
	LEDDDR = 0b11111011;
	PORTD |= _BV(PORTD2);	// wlacz pull-up na pinie PD2

	DDRC = 0x03;   // wyjscia PC0-PC1 steruja wyswietlana cyfra
	PORTC &= 0xFC; // ustaw stan niski na wyjsciach sterujacych PC0-PC1
}

void SysTickInit()
{
	//ASSR |= _BV(AS2);		// TIM2 tacted asynchronously
	TCNT2 = 0x00;			// counter set to 0
	//TCCR2B = _BV(CS22) | _BV(CS20); // Prescaler 128 (interrupt every 1s)
	TCCR2B = _BV(CS22);		// Preskaler CLKIO/64  (16 MHz / 256 / 64 = 1 kHz)
	while(ASSR & 0x1F);		// Wait for TIM2 update (busy flags must be cleared)
	TIMSK2 |= _BV(TOIE2);	// Unblock TIM2 overflow interrupt
}

volatile uint32_t ticks = 0;
volatile uint8_t toggle = 0;

ISR(TIMER2_OVF_vect)
{
	ticks++;
	if (!(ticks % 1000)) toggle ^= 0x01;
}

int main()
{
	IO_Init();
	
	Timer0InitWithDimmer();
	PCF8574_Init();
	PCF8574_INTInit();
	buzzerInit ();
	SysTickInit();
	
	sei();
	
	LEDDIGITS[0]= 0;
	LEDDIGITS[1]= 0;
	uint8_t setValue = 0;
	
	while(1)
	{
		if (PCF8574_INT) {
			_delay_ms(1);			// debouncing
			PCF8574_ReadState();
			ATOMIC_BLOCK (ATOMIC_FORCEON) {
				switch(Read2StepEncoder())
				{
					case -1 :	if(setValue>0) setValue-=1; break;
					case 0  :	break;
					case 1  :	if(setValue<99) setValue+=1; break;
				};
				LEDDIGITS[0]= (uint8_t)(setValue/10);
				LEDDIGITS[1]= (uint8_t)(setValue%10);
				buzzerSetVolume(setValue);
				PCF8574_INT = false;	// clear internal INT flag
			};
		}
		if (toggle) buzzerSetVolume(setValue);
		else buzzerSetVolume(0);
	}

#ifdef stateDisplay
	volatile uint8_t x = 0xff;
	while(1)
	{
		if (PCF8574_INT) {
			_delay_ms(1);  // debouncing
			x = PCF8574_ReadState();
			ATOMIC_BLOCK (ATOMIC_FORCEON) {
				LEDDIGITS[0]= (uint8_t)((x & 0xF0) >> 4);
				LEDDIGITS[1]= (uint8_t)(x & 0x0F);
				PCF8574_INT = false;
			};
		}
	}
#endif

#ifdef brightness
	LEDDIGITS[0]=1;
	LEDDIGITS[1]=2;
	
	uint8_t i = 0;
	while(1)
	{
		for(i = 0; i < MAX_BRIGHTNESS_VAL; i++) {
			ATOMIC_BLOCK (ATOMIC_FORCEON) {OCR0A=i;}
			_delay_ms(20);
		}
		for(i = MAX_BRIGHTNESS_VAL; i > 0; i--) {
			ATOMIC_BLOCK (ATOMIC_FORCEON) {OCR0A=i;}
			_delay_ms(20);
		}
	};
#endif

}