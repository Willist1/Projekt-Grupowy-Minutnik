/*
 * pwrFail.c
 *
 * Created: 3/14/2019 11:11:01 PM
 *  Author: stefan
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "main.h"

#include <stdio.h>

#define DETECT_DELAY_TICKS 4	// 2 sec
#define MULTIPLICATION    10	// number of memory blocks written to

// Data in EEPROM (?declaration requires initialization?)
EEMEM tNVData EEPROMDataBuffer[MULTIPLICATION];
EEMEM unsigned char pointers[MULTIPLICATION];

void clearPointers()
{
	for(int i = 0; i < MULTIPLICATION; i++) eeprom_write_byte(&pointers[i], 0xFF);
}

void writeEEPROMDataBuffer()
{
	int i = 0;
	while((i < MULTIPLICATION) && (eeprom_read_byte(&pointers[i]) != 0xFF)) i++;	// Search for first 0xFF pointer
	if(i == MULTIPLICATION) clearPointers();
	eeprom_write_byte(&pointers[i%MULTIPLICATION], 0);
	eeprom_update_block((void*)&NVData, (void*)&EEPROMDataBuffer[i%MULTIPLICATION], sizeof(tNVData));	// Copy data to EEPROM
}

void readEEPROMDataBuffer()
{
	int i=0;
	while((i < MULTIPLICATION) && (eeprom_read_byte(&pointers[i]) != 0xFF)) i++;	// Search for first 0xFF pointer
	i--;																			// Move back to last zeroed pointer
	eeprom_read_block((void*)&NVData, (void*)&EEPROMDataBuffer[i%MULTIPLICATION], sizeof(tNVData));		// Copy data from EEPROM to SRAM
}

ISR(ANALOG_COMP_vect)//, ISR_NAKED)
{
	if(ticks > DETECT_DELAY_TICKS) {
		// Turn off redundant generators and reduce FCLK
		// (NOT POSSIBLE)
		
		// set PD4-5 low (turn off displays)
		PORTD &= 0xCF;
		
		// Set all pins as inputs
		DDRB = 0x00;
		DDRC = 0x00;
		DDRD = 0x00;
		
		// Save temporary config
		switch(memorizedButton)
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

		// Write non-volatile data to EEPROM
		writeEEPROMDataBuffer();
		
		while(1); // Nothing left to do...
	}
}

void EEPROMupdate() {
	writeEEPROMDataBuffer();
}

void pwrFailInit()
{
	DIDR1 =_BV(AIN1D) | _BV(AIN0D);   // Turn off digital ports used as comparator inputs
	ACSR =_BV(ACIE) | _BV(ACIS1);     // Turn on comparator and its interrupt caused by falling edge
}

void NVDataInit()
{
	readEEPROMDataBuffer();
}