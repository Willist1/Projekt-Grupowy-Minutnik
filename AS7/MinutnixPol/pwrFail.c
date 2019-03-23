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

#define DETECT_DELAY_TICKS 2000

// Data in EEPROM (declaration requires initialization)
EEMEM tNVData EEPROMData = {.config.cntVal=0xaa, .config.warnVal=0xbb, .config.brightVal=0xcc, .config.volumeVal=0xdd, .totalSeconds=0xffff};

ISR(ANALOG_COMP_vect)//, ISR_NAKED)
{
	if(ticks > DETECT_DELAY_TICKS) {
		// Turn off redundant generators and reduce FCLK
		// (NOT POSSIBLE)
		
		// Set all pins as inputs
		DDRB = 0x00;
		DDRC = 0x00;
		DDRD = 0x00;

		// Write non-volatile data to EEPROM
		eeprom_update_block((void*)&NVData, (void*)&EEPROMData, sizeof(tNVData)); // Copy data to EEPROM
		
		while(1); // Nothing left to do...
	}
}

void EEPROMwrite () {
	eeprom_update_block((void*)&NVData, (void*)&EEPROMData, sizeof(tNVData)); // Copy data to EEPROM
}

void pwrFailInit()
{
	DIDR1 =_BV(AIN1D) | _BV(AIN0D);   // Turn off digital ports used as comparator inputs
	ACSR =_BV(ACIE) | _BV(ACIS1);     // Turn on comparator and its interrupt caused by falling edge
}

void NVDataInit()
{
	eeprom_read_block((void*)&NVData, (void*)&EEPROMData, sizeof(tNVData));  // Copy data from EEPROM to SRAM
}

/*
int main(void)
{
	DDRB |=_BV(PB5);
	PORTB |= _BV(PB5);

	Data_init();
	AC_init();
	sei(); // Odblokuj przerwania
	
	DaneSRAM.Dane=0x4BCD;  // Modyfikujemy odtworzona kopie danych
	DaneSRAM.PID_P=0x0211; // Zostana one automatycznie zapisane po odlaczeniu zasilania
	DaneSRAM.Temperatura=0x1234;

	while(1)
	{
		// IT WORKED!
	}
}
*/