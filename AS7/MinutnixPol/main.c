/*
 *  main.c
 *
 *  Created: 2019-03-09 15:34
 *  Authors: Kluczek, Wegrzyn, Jopek
 */ 
//#define F_CPU 8000000L

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


#define EXTERNAL_CLOCK

#ifdef EXTERNAL_CLOCK
#define MAX_NO_ACTIVITY_TICKS 600	// 5 min
#else
#define MAX_NO_ACTIVITY_TICKS 300000	// 5 min
#endif

tSTATE currentState = sSetting;
volatile uint32_t ticks = 0;
volatile uint8_t toggle_500ms = 0;
uint32_t lastActivityTime = 0;
uint8_t WARN_MAX_VAL = 15;

#define ALARM_MAX_DURATION 20 // [s]
volatile uint8_t alarmTick = 0;
volatile bool endBeep = false;

// index of brightnessTable is ADC reading (max 255) divided by 9
// results range: 0-28 (29 values)
#define ADC_READING_DIVISOR 9
const uint8_t __attribute__((__progmem__)) brightnessTable[] = {
	//100, 55, 35, 30, 26, 24, 22, 20, 19, 18, 17, 16, 15, 14, 9, 5, 2, 1, 1, 1
	100, 70, 55, 42, 35, 33, 30, 28, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 8, 5, 2, 1, 1, 1, 1
};

void sysTickInit()
{
	#ifdef EXTERNAL_CLOCK
	ASSR |= _BV(AS2);		// TIM2 tacted asynchronously
	TCCR2B = _BV(CS22);		// Preskaler CLKIO/64  (32.768 kHz / 256 / 64 = 2 Hz)
	#else //internal
	TCCR2A = _BV(WGM20);	//PWM, Phase Correct TOP = OCRA, OVint at BOTTOM
	OCR2A = 124;
	TCCR2B = _BV(WGM22) | _BV(CS21) | _BV(CS20);	// Preskaler CLKIO/32  (8 MHz / (124+1) / 32 /2 = 1 kHz)
	#endif
	
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
	
	#ifdef EXTERNAL_CLOCK
	toggle_500ms ^= 0x01;
	if (!(ticks % 2) && (currentState == sRunning)) {
		if(NVData.totalSeconds > 0) NVData.totalSeconds--;
		if(endBeep && (alarmTick < ALARM_MAX_DURATION)) alarmTick++;
	}
	#else
	if (!(ticks % 500)) {
		toggle_500ms ^= 0x01;
	}
	if (!(ticks % 1000) && (currentState == sRunning)) {
		if(NVData.totalSeconds > 0) NVData.totalSeconds--;
		if(endBeep && (alarmTick < ALARM_MAX_DURATION)) alarmTick++;
	}
	#endif
	
	// ADJUST BRIGHTNESS
	uint8_t brightness = 0;
	memcpy_P(&brightness,brightnessTable+(lightsensorRead()/ADC_READING_DIVISOR),1);
	brightness = lightsensorFilter(brightness);
	displaySetBrightness(brightness);
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
	displayOff();
	set_sleep_mode(SLEEP_MODE_IDLE);	// CPU clock turned off, peripherals operating normally
	
	//USART_init();
	//static FILE usartout = FDEV_SETUP_STREAM (put, get, _FDEV_SETUP_RW);
	//stdout = &usartout;
	
	PCF8574_Init();
	
	sysTickInit();
	TPIC6C596Init();
	pwrFailInit();
	settingsLEDInit();
	lightsensorInit();
	displayInit();
	buzzerInit();

	LEDDIGITS[0]= BLANK_DISPLAY;
	LEDDIGITS[1]= BLANK_DISPLAY;
	LEDDIGITS[2]= BLANK_DISPLAY;
	LEDDIGITS[3]= BLANK_DISPLAY;
	
	NVDataInit();
	if (NVData.config.cntVal > CNT_MAX_VAL) NVData.config.cntVal = CNT_MAX_VAL;
	WARN_MAX_VAL = NVData.config.cntVal;
	if (NVData.config.warnVal > WARN_MAX_VAL) NVData.config.warnVal = WARN_MAX_VAL;
	if (NVData.config.volumeVal > VOL_MAX_VAL) NVData.config.volumeVal = VOL_MAX_VAL;
	if (NVData.totalSeconds > NVData.config.cntVal*60) NVData.totalSeconds = NVData.config.cntVal*60;
	displaySetBrightness(DEFAULT_BRIGHTNESS);
	buzzerSetVolume(NVData.config.volumeVal);
	settingsLEDToggle(LED_SET_CNT);
	setVal = NVData.config.cntVal;	// prepare user interaction
	
	sei();
	SPDR = 0;				// Initialize SPI interrupts by writing to SPI data register
	while (SeqNum != 3);	// Wait for first SPI transfer to finish before enabling display
	while (SeqNum != 0);
	displayOn();
	
	wdtInit();
	wdt_reset();	// reset watchdog
	
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
									// prevent warning time from being greater than total time
									WARN_MAX_VAL = NVData.config.cntVal;
									if (NVData.config.warnVal > NVData.config.cntVal) NVData.config.warnVal = NVData.config.cntVal;
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
									memorizedButton = setCnt;
									buzzerOff();
									break;
								default:
									break;
							}
							
						}
						
					} else { // (currentState == sRunning || currentState == sPause)
						
						// ESTABLISH NEW STATE
						switch(pressedButton)
						{
							case Stop:
								memorizedButton = Stop;
								currentState = sSetting;
								NVData.totalSeconds = NVData.config.cntVal*60;
								setVal = NVData.config.cntVal;
								settingsLEDToggle(LED_SET_CNT);
								memorizedButton = setCnt;
								endBeep = false;
								alarmTick = 0;
								buzzerOff();
								break;
							case Play:
								if (currentState == sRunning) {
									if (NVData.totalSeconds > 0)
										currentState = sPause;
									else {
										memorizedButton = Stop;
										currentState = sSetting;
										NVData.totalSeconds = NVData.config.cntVal*60;
										setVal = NVData.config.cntVal;
										settingsLEDToggle(LED_SET_CNT);
										memorizedButton = setCnt;
										endBeep = false;
										alarmTick = 0;
										buzzerOff();
									}
										
								} else { // currentState == sPause
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
					case setVolume:
						ATOMIC_BLOCK (ATOMIC_FORCEON) { buzzerSetVolume(setVal); };
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
						LEDDIGITS[0]= 0;
						LEDDIGITS[1]= DP;
						LEDDIGITS[2]= DP;
						LEDDIGITS[3]= 0;
					};
				}
				else {
					ATOMIC_BLOCK (ATOMIC_FORCEON) {
						LEDDIGITS[0]= BLANK_DISPLAY;
						LEDDIGITS[1]= BLANK_DISPLAY;
						LEDDIGITS[2]= BLANK_DISPLAY;
						LEDDIGITS[3]= BLANK_DISPLAY;
					};
				}
			}
			
			if (NVData.totalSeconds == 0) {
				if (alarmTick < ALARM_MAX_DURATION)
				{
					buzzerOn();	// Perform continuous end beep
					endBeep = true;
				}
				else
				{
					buzzerOff();
				}
			} else if (NVData.totalSeconds == NVData.config.warnVal*60) {								// Start warn beep
				ATOMIC_BLOCK (ATOMIC_FORCEON) { buzzerOn(); };
			} else if (NVData.totalSeconds == NVData.config.warnVal*60 - WARN_BEEP_DURATION_SECONDS) {	// End warn beep
				ATOMIC_BLOCK (ATOMIC_FORCEON) { buzzerOff(); };
			}
			
			break;
			
			case sPause: //copy of old sRunning
			if (NVData.totalSeconds > 0) {
				ATOMIC_BLOCK (ATOMIC_FORCEON) 
					if (toggle_500ms) {						// digits blink
						ATOMIC_BLOCK (ATOMIC_FORCEON) {
							LEDDIGITS[0]= (uint8_t)((NVData.totalSeconds/60)/10);
							LEDDIGITS[1]= (uint8_t)((NVData.totalSeconds/60)%10) | DP;
							LEDDIGITS[2]= (uint8_t)((NVData.totalSeconds%60)/10) | DP;
							LEDDIGITS[3]= (uint8_t)((NVData.totalSeconds%60)%10);
						};
					}
					else {
						ATOMIC_BLOCK (ATOMIC_FORCEON) {
							LEDDIGITS[0]= BLANK_DISPLAY;
							LEDDIGITS[1]= BLANK_DISPLAY;
							LEDDIGITS[2]= BLANK_DISPLAY;
							LEDDIGITS[3]= BLANK_DISPLAY;
						};
					}
				};
			
			break;
			
			default:
				break;
				
		}
		
		// SLEEP MODE CHECK
		if ((ticks - lastActivityTime > MAX_NO_ACTIVITY_TICKS) && (currentState == sSetting)) {
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