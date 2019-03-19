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
#include "pwrFail.h"

#include <stdio.h>


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
	
	DDRC |= 0x04;  // wyjscie PC2 steruje kropka
	PORTC &= 0xFB; // ustaw stan niski na wyjsciu PC2
}

void sysTickInit()
{
	//ASSR |= _BV(AS2);		// TIM2 tacted asynchronously
	TCNT2 = 0x00;			// counter set to 0
	//TCCR2B = _BV(CS22) | _BV(CS20); // Prescaler 128 (interrupt every 1s)
	TCCR2B = _BV(CS22);		// Preskaler CLKIO/64  (16 MHz / 256 / 64 = 1 kHz)
	while(ASSR & 0x1F);		// Wait for TIM2 update (busy flags must be cleared)
	TIMSK2 |= _BV(TOIE2);	// Unblock TIM2 overflow interrupt
}

typedef enum {
	sIdle = 0,
	sSetting,
	sRunning
} tSTATE;

typedef struct {
	uint8_t cntVal;
	uint8_t warnVal;
	uint8_t brightVal;
	uint8_t volumeVal;
} tCONFIG;

tSTATE currentState = sIdle;
volatile tCONFIG config;
volatile uint16_t totalSeconds;

volatile uint32_t ticks = 0;
volatile uint8_t toggle = 0;

ISR(TIMER2_OVF_vect)
{
	ticks++;
	if (!(ticks % 500)) toggle ^= 0x01;
	if (!(ticks % 1000) && (currentState == sRunning)) {
		if(totalSeconds > 0) totalSeconds--;
	}
}

#define CNT_MAX_VAL 99
#define CNT_MIN_VAL 1
#define WARN_MAX_VAL 15
#define WARN_MIN_VAL 0
#define BRIGHT_MAX_VAL 10
#define BRIGHT_MIN_VAL 1
#define VOL_MAX_VAL 10
#define VOL_MIN_VAL 0

#define WARN_BEEP_DURATION_SECONDS 2

int main()
{
	IO_Init();
	
	Timer0InitWithDimmer();
	PCF8574_Init();
	PCF8574_INTInit();
	buzzerInit ();
	sysTickInit();
	
	//USART_init();
	//static FILE usartout = FDEV_SETUP_STREAM (put, get, _FDEV_SETUP_RW);
	//stdout = &usartout;
	
	LEDDIGITS[0]= 0;
	LEDDIGITS[1]= 0;
	BUTTON memorizedButton = BUTTON_NONE;
	BUTTON pressedButton = BUTTON_NONE;
	config.cntVal = 45;
	config.warnVal = 10;
	config.brightVal = 7;
	config.volumeVal = 6;
	displaySetBrightness(10*config.brightVal);
	buzzerSetVolume(10*config.volumeVal);
	totalSeconds = config.cntVal*60;
	uint8_t setVal = 0;
	uint8_t valMax = 0;
	uint8_t valMin = 0;
	
	sei();
	
	while(1)
	{
		// HANDLE EXPANDER INTERRUPT
		if (PCF8574_INT) {
			_delay_ms(1);			// debouncing
			PCF8574_ReadState();
			ATOMIC_BLOCK (ATOMIC_FORCEON) {
				
				// READ BUTTONPRESS
				pressedButton = PCF8574_ReadButtonPress();
				
				if (pressedButton != BUTTON_NONE) {					// button was pressed

					if (memorizedButton == BUTTON_NONE) {			// button is not memorized
						
						switch(pressedButton)
						{
							case setCnt:
							case setWarning:
							case setBrightness:
							case setVolume:
								currentState = sSetting;
								memorizedButton = pressedButton;
								break;
							case Play:
								currentState = sRunning;
								memorizedButton = pressedButton;
								break;
							case Stop:
								currentState = sIdle;
								break;
							default:
								break;
						}
						
						switch(pressedButton)
						{
							case setCnt:
								setVal = config.cntVal;
								break;
							case setWarning:
								setVal = config.warnVal;
								break;
							case setBrightness:
								setVal = config.brightVal;
								break;
							case setVolume:
								setVal = config.volumeVal;
								buzzerOn();
								break;
							case Stop:
								totalSeconds = config.cntVal*60;
								break;
							default:
								break;
						}
						
					} else {										// button is memorized
						
						if (pressedButton == memorizedButton) {		// ignore buttons other than memorized
							
							switch(pressedButton)
							{
								case setCnt:
								case setWarning:
								case setBrightness:
								case setVolume:
								case Play:
									currentState = sIdle;
									memorizedButton = BUTTON_NONE;
									break;
								default:
									break;
							}
							
							switch(pressedButton)
							{
								case setCnt:
									config.cntVal = setVal;
									totalSeconds = config.cntVal*60;
									break;
								case setWarning:
									config.warnVal = setVal;
									break;
								case setBrightness:
									config.brightVal = setVal;
									break;
								case setVolume:
									config.volumeVal = setVal;
									buzzerOff();
									break;
								default:
									break;
							}
						} else if ((memorizedButton == Play) && (pressedButton == Stop)) { // handle special case
							currentState = sIdle;
							memorizedButton = BUTTON_NONE;
							totalSeconds = config.cntVal*60;
							buzzerOff();
							LEDDIGITS[0] &= ~DP;	// Turn off dots
							LEDDIGITS[1] &= ~DP;
						}	
					}
					
				}
				
				// READ ENCODER
				switch (memorizedButton) {
					case setCnt:
						valMax = CNT_MAX_VAL;
						valMin = CNT_MIN_VAL;
						break;
					case setWarning:
						valMax = WARN_MAX_VAL;
						valMin = WARN_MIN_VAL;
						break;
					case setBrightness:
						valMax = BRIGHT_MAX_VAL;
						valMin = BRIGHT_MIN_VAL;
						break;
					case setVolume:
						valMax = VOL_MAX_VAL;
						valMin = VOL_MIN_VAL;
						break;
					default:
						valMax = 0;
						valMin = 0;
						break;
				}
				switch(Read2StepEncoder())
				{
					case -1 :	if(setVal>valMin) setVal-=1; break;
					case 0  :	break;
					case 1  :	if(setVal<valMax) setVal+=1; break;
				};
				
				PCF8574_INT = false;	// clear internal INT flag
			};
		}
		
		// DISPLAY UPDATE
		switch(currentState)
		{
			case sIdle:
				if (totalSeconds >= 60) {
					ATOMIC_BLOCK (ATOMIC_FORCEON) {
						LEDDIGITS[0]= (uint8_t)((totalSeconds/60)/10);
						LEDDIGITS[1]= (uint8_t)((totalSeconds/60)%10);
					};
					} else {
					ATOMIC_BLOCK (ATOMIC_FORCEON) {
						LEDDIGITS[0]= (uint8_t)((totalSeconds)/10);
						LEDDIGITS[1]= (uint8_t)((totalSeconds)%10);
					};
				}
				break;
			case sSetting:
				ATOMIC_BLOCK (ATOMIC_FORCEON) {
					LEDDIGITS[0]= (uint8_t)(setVal/10);
					LEDDIGITS[1]= (uint8_t)(setVal%10);
					
					if (toggle) LEDDIGITS[1] |= DP;
					else LEDDIGITS[1] &= ~DP;
				};
				switch (memorizedButton) {
					case setBrightness:
						ATOMIC_BLOCK (ATOMIC_FORCEON) { displaySetBrightness(10*setVal); };
						break;
					case setVolume:
						ATOMIC_BLOCK (ATOMIC_FORCEON) { buzzerSetVolume(10*setVal); };
						break;
					default:
						break;
				}
				break;
			case sRunning:
				if (totalSeconds >= 60) {											// display minutes when >= 60 sec
					ATOMIC_BLOCK (ATOMIC_FORCEON) {
						LEDDIGITS[0]= (uint8_t)((totalSeconds/60)/10);
						LEDDIGITS[1]= (uint8_t)((totalSeconds/60)%10);
					};
				} else {															// display seconds when < 60 sec
					ATOMIC_BLOCK (ATOMIC_FORCEON) {
						LEDDIGITS[0]= (uint8_t)((totalSeconds)/10);
						LEDDIGITS[1]= (uint8_t)((totalSeconds)%10);
					};
				}
				if (totalSeconds == 0) {
					ATOMIC_BLOCK (ATOMIC_FORCEON) {
						buzzerOn();				// Perform end beep
						LEDDIGITS[0] |= DP;		// Light up dots
						LEDDIGITS[1] |= DP;
					};
				} else if (totalSeconds == config.warnVal*60) {						// Start warn beep
					ATOMIC_BLOCK (ATOMIC_FORCEON) { buzzerOn(); };
				} else if (totalSeconds == config.warnVal*60 - WARN_BEEP_DURATION_SECONDS) {	// End warn beep
					ATOMIC_BLOCK (ATOMIC_FORCEON) { buzzerOff(); };
				}
				break;
			default:
				break;
		}
		
	}
	
	
	
	
#ifdef encoderRead
	while(1)
	{
		if (PCF8574_INT) {
			_delay_ms(1);			// debouncing
			PCF8574_ReadState();
			ATOMIC_BLOCK (ATOMIC_FORCEON) {
				switch(Read2StepEncoder())
				{
					case -1 :	if(setVal>0) setVal-=1; break;
					case 0  :	break;
					case 1  :	if(setVal<99) setVal+=1; break;
				};
				LEDDIGITS[0]= (uint8_t)(setVal/10);
				LEDDIGITS[1]= (uint8_t)(setVal%10);
				buzzerSetVolume(setVal);
				PCF8574_INT = false;	// clear internal INT flag
			};
		}
		if (toggle) buzzerSetVolume(setVal);
		else buzzerSetVolume(0);
	}
#endif

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