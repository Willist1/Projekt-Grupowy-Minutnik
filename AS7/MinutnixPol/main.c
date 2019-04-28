/*
 *  main.c
 *
 *  Created: 2019-03-09 15:34
 *  Authors: Kluczek, Wegrzyn, Jopek
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdio.h>
#include "main.h"
#include "uart.h"
#include "display.h"
#include "i2c.h"
#include "PCF8574.h"
#include "encoder.h"
#include "buzzer.h"
#include "pwrFail.h"
#include "TPIC6C596.h"
#include "lightsensor.h"

#define MAX_NO_ACTIVITY_TICKS 10000//300000	// 5 min

tSTATE currentState = sSetting;
volatile uint32_t ticks = 0;
volatile uint8_t toggle_500ms = 0;
uint32_t lastActivityTime = 0;

void sysTickInit()
{
	//ASSR |= _BV(AS2);		// TIM2 tacted asynchronously
	TCNT2 = 0x00;			// counter set to 0
	//TCCR2B = _BV(CS22) | _BV(CS20); // Prescaler 128 (interrupt every 1s)
	TCCR2B = _BV(CS22);		// Preskaler CLKIO/64  (16 MHz / 256 / 64 = 1 kHz)
	while(ASSR & 0x1F);		// Wait for TIM2 update (busy flags must be cleared)
	TIMSK2 |= _BV(TOIE2);	// Unblock TIM2 overflow interrupt
}

void sysTickOn()
{
	TIMSK2 |= _BV(TOIE2);	// Unblock TIM2 overflow interrupt
}

void sysTickOff()
{
	TIMSK2 &= ~_BV(TOIE2);	// Block TIM2 overflow interrupt
}

ISR(TIMER2_OVF_vect)	// System clock
{
	ticks++;
	if (!(ticks % 500)) {
		toggle_500ms ^= 0x01;
	}
	if (!(ticks % 1000) && (currentState == sRunning)) {
		if(NVData.totalSeconds > 0) NVData.totalSeconds--;
	}
}

void wdtInit() {
	MCUSR &= ~_BV(WDRF);	// Clear watchdog system reset flag
	wdt_reset();
	WDTCSR = _BV(WDCE) | _BV(WDE);
	wdt_enable(WDTO_1S);
}

void wdtDeinit() {
	MCUSR &= ~_BV(WDRF);
	wdt_reset();
	wdt_disable();
}

// index of brightnessTable is ADC reading (max 255) divided by 13
// results range: 0-19 (20 values)
#define ADC_READING_DIVISOR 13
const uint8_t __attribute__((__progmem__)) brightnessTable[] = {
	5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100
};

#define SETTINGS_LED_PORT PORTC
#define LED_SET_CNT 1
#define LED_SET_WARN 2
#define LED_SET_VOL 3

void settingsLEDInit() {
	DDRC |= (_BV(LED_SET_CNT) | _BV(LED_SET_WARN) | _BV(LED_SET_VOL));	// LED pins as output
	PORTC |= (_BV(LED_SET_CNT) | _BV(LED_SET_WARN) | _BV(LED_SET_VOL));	// Set all pins high (turn off LEDs)
}

void settingsLEDTurnOff() {
	PORTC |= (_BV(LED_SET_CNT) | _BV(LED_SET_WARN) | _BV(LED_SET_VOL));	// Turn off all LEDs
}

void settingsLEDToggle(uint8_t LEDId) {
	settingsLEDTurnOff();
	if (LEDId >= LED_SET_CNT && LEDId <= LED_SET_VOL) PORTC &= ~_BV(LEDId);	// Turn given LED on
}


BUTTON pressedButton = BUTTON_NONE;
BUTTON memorizedButton = setCnt;
uint8_t setVal = 0;
uint8_t valMax = 0;
uint8_t valMin = 0;

int main()
{
	set_sleep_mode(SLEEP_MODE_IDLE);	// CPU clock turned off, peripherals operating normally
	
	wdtInit();
	
	sysTickInit();
	PCF8574_Init();
	TPIC6C596Init();
	buzzerInit ();
	pwrFailInit();
	settingsLEDInit();
	lightsensorInit();
	displayInit();
	
	USART_init();
	static FILE usartout = FDEV_SETUP_STREAM (put, get, _FDEV_SETUP_RW);
	stdout = &usartout;
	
	LEDDIGITS[0]= 0;
	LEDDIGITS[1]= 0;
	LEDDIGITS[2]= 0;
	LEDDIGITS[3]= 0;
	
	NVDataInit();
	if (NVData.config.cntVal > CNT_MAX_VAL) NVData.config.cntVal = CNT_MAX_VAL;
	if (NVData.config.warnVal > WARN_MAX_VAL) NVData.config.warnVal = WARN_MAX_VAL;
	if (NVData.config.brightVal > BRIGHT_MAX_VAL) NVData.config.brightVal = BRIGHT_MAX_VAL;
	if (NVData.config.volumeVal > VOL_MAX_VAL) NVData.config.volumeVal = VOL_MAX_VAL;
	if (NVData.totalSeconds > NVData.config.cntVal*60) NVData.totalSeconds = NVData.config.cntVal*60;
	displaySetBrightness(10*NVData.config.brightVal);
	buzzerSetVolume(10*NVData.config.volumeVal);
	settingsLEDToggle(LED_SET_CNT);
	setVal = NVData.config.cntVal;	// prepare user interaction
	
	sei();
	SPDR = 0;				// Initialize SPI interrupts by writing to SPI data register
	while (SeqNum != 3);	// Wait for first SPI transfer to finish before enabling display
	displayOn();
	
	while(1)
	{
		// HANDLE EXPANDER INTERRUPT
		if (PCF8574_INT) {
			_delay_ms(1);			// debouncing
			PCF8574_ReadState();
			ATOMIC_BLOCK (ATOMIC_FORCEON) {
				
				lastActivityTime = ticks;							// set last activity timestamp
				
				// READ BUTTONPRESS
				pressedButton = PCF8574_ReadButtonPress();
				
				if (pressedButton != BUTTON_NONE) {					// button was pressed

					if (currentState == sSetting) {
						
						if (pressedButton != memorizedButton) {	// ON BUTTON CHANGE
							
							// ESTABLISH NEW STATE
							switch(pressedButton)
							{
								case setCnt:
								case setWarning:
								case setVolume:
								case Stop:
									currentState = sSetting;
									break;
								case Play:
									currentState = sRunning;
									settingsLEDTurnOff();
									break;
								default:
									break;
							}
							
							// SAVE SETTINGS
							switch(memorizedButton)
							{
								case setCnt:
									NVData.config.cntVal = setVal;
									NVData.totalSeconds = NVData.config.cntVal*60;
									break;
								case setWarning:
									NVData.config.warnVal = setVal;
									settingsLEDToggle(LED_SET_WARN);
									break;
								case setVolume:
									NVData.config.volumeVal = setVal;
									buzzerOff();
									break;
								default:
									break;
							}
							
							// MEMORIZE BUTTON PRESS
							memorizedButton = pressedButton;
							
							// PREPARE INTERACTION WITH USER
							switch(pressedButton)
							{
								case setCnt:
									setVal = NVData.config.cntVal;
									settingsLEDToggle(LED_SET_CNT);
									break;
								case setWarning:
									setVal = NVData.config.warnVal;
									settingsLEDToggle(LED_SET_WARN);
									break;
								case setVolume:
									setVal = NVData.config.volumeVal;
									settingsLEDToggle(LED_SET_VOL);
									buzzerOn();
									break;
								case Stop:
									NVData.totalSeconds = NVData.config.cntVal*60;
									setVal = NVData.config.cntVal;
									settingsLEDToggle(LED_SET_CNT);
									buzzerOff();
									break;
								default:
									break;
							}
							
						}
						
					} else { // (currentState == sRunning || currentState == sIdle)
						
						// ESTABLISH NEW STATE
						switch(pressedButton)
						{
							case Stop:
								memorizedButton = Stop;
								currentState = sSetting;
								NVData.totalSeconds = NVData.config.cntVal*60;
								setVal = NVData.config.cntVal;
								settingsLEDToggle(LED_SET_CNT);
								buzzerOff();
								break;
							case Play:
								if (currentState == sRunning) {
									currentState = sIdle;
								} else { // currentState == sIdle
									currentState = sRunning;
								}
								break;
							default:
								break;
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
			case sSetting:
				ATOMIC_BLOCK (ATOMIC_FORCEON) {
					LEDDIGITS[0]= (uint8_t)(setVal/10);
					LEDDIGITS[1]= (uint8_t)((setVal%10));
					LEDDIGITS[2]= BLANK_DISPLAY;
					LEDDIGITS[3]= BLANK_DISPLAY;
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
				if (NVData.totalSeconds > 0) {
					ATOMIC_BLOCK (ATOMIC_FORCEON) {
						LEDDIGITS[0]= (uint8_t)((NVData.totalSeconds/60)/10);
						LEDDIGITS[1]= (uint8_t)((NVData.totalSeconds/60)%10);
						LEDDIGITS[2]= (uint8_t)((NVData.totalSeconds%60)/10);
						LEDDIGITS[3]= (uint8_t)((NVData.totalSeconds%60)%10);
						if (toggle_500ms) {
							LEDDIGITS[1] |= DP;				// colon blink
							LEDDIGITS[2] |= DP;
						}
						else {
							LEDDIGITS[1] &= ~DP;
							LEDDIGITS[2] &= ~DP;
						}
					};
				} else {	// NVData.totalSeconds == 0
					if (toggle_500ms) {						// digits blink
						ATOMIC_BLOCK (ATOMIC_FORCEON) {
							LEDDIGITS[0]= BLANK_DISPLAY;
							LEDDIGITS[1]= BLANK_DISPLAY;
							LEDDIGITS[2]= BLANK_DISPLAY;
							LEDDIGITS[3]= BLANK_DISPLAY;
						};
					}
					else {
						ATOMIC_BLOCK (ATOMIC_FORCEON) {
							LEDDIGITS[0]= 0;
							LEDDIGITS[1]= DP;
							LEDDIGITS[2]= DP;
							LEDDIGITS[3]= 0;
						};
					}
				}
				
				if (NVData.totalSeconds == 0) {
					buzzerOn();	// Perform continuous end beep
				} else if (NVData.totalSeconds == NVData.config.warnVal*60) {								// Start warn beep
					ATOMIC_BLOCK (ATOMIC_FORCEON) { buzzerOn(); };
				} else if (NVData.totalSeconds == NVData.config.warnVal*60 - WARN_BEEP_DURATION_SECONDS) {	// End warn beep
					ATOMIC_BLOCK (ATOMIC_FORCEON) { buzzerOff(); };
				}
				
				break;
				
			default:
				break;
				
		}
		
		// ADJUST BRIGHTNESS
		uint8_t brightness = 0;
		memcpy_P(&brightness,brightnessTable+(lightsensorRead()/ADC_READING_DIVISOR),1);
		NVData.config.brightVal = brightness;
		ATOMIC_BLOCK (ATOMIC_FORCEON) {
			displaySetBrightness(NVData.config.brightVal);
		}
		
		// SLEEP MODE CHECK
		if ((ticks - lastActivityTime > MAX_NO_ACTIVITY_TICKS) && (currentState != sRunning)) {
			ATOMIC_BLOCK (ATOMIC_FORCEON) {		// turn off all interrupt sources except INT0
				displayOff();
				buzzerOff();
				settingsLEDTurnOff();
				lightsensorSuspend();
				TPIC6C596Suspend();
				wdtDeinit();
				LEDDIGITS[1] &= ~DP;			// turn off colon
				LEDDIGITS[2] &= ~DP;
				sysTickOff();
				switch(memorizedButton)			// save temporary config
				{
					case setCnt:
						NVData.config.cntVal = setVal;
						break;
					case setWarning:
						NVData.config.warnVal = setVal;
						break;
					case setVolume:
						NVData.config.volumeVal = setVal;
						break;
					default:
						break;
				}
				EEPROMupdate();	// only writes to EEPROM when config changed in relation to last record
			};
			sleep_mode();
			ATOMIC_BLOCK (ATOMIC_FORCEON) {		// restore all turned off interrupt sources
				sysTickOn();
				wdtInit();
				currentState = sSetting;		// enforce setting state
				memorizedButton = setCnt;		// memorized button
				setVal = NVData.config.cntVal;	// prepare user interaction
				ATOMIC_BLOCK (ATOMIC_FORCEON) {
					LEDDIGITS[0]= (uint8_t)(setVal/10);
					LEDDIGITS[1]= (uint8_t)((setVal%10));
					LEDDIGITS[2]= BLANK_DISPLAY;
					LEDDIGITS[3]= BLANK_DISPLAY;
				};
				TPIC6C596Resume();
				lightsensorResume();
				settingsLEDToggle(LED_SET_CNT);
				displayOn();
				lastActivityTime = ticks;		// activity timestamp
				PCF8574_INT = false;			// ignore wake-up cause (button press / encoder movement)
			};
		}

		wdt_reset();	// reset watchdog
	}

}