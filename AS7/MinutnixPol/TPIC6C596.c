/*
 * TPIC6C596.c
 *
 * Created: 4/20/2019 3:09:34 PM
 *  Author: stefa
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "TPIC6C596.h"
#include "display.h"

inline void TPIC6C596_Set_SS()
{
	SHIFT_REG_PORT |= _BV(SHIFT_REG_RCK);
}

inline void TPIC6C596_Reset_SS()
{
	SHIFT_REG_PORT &= ~(_BV(SHIFT_REG_RCK));
}

inline void TPIC6C596_Set_OE()
{
	SHIFT_REG_PORT |= _BV(SHIFT_REG_G);
}

inline void TPIC6C596_Reset_OE()
{
	SHIFT_REG_PORT &= ~(_BV(SHIFT_REG_G));
}

#define MAX_INDEX LEDDISPNO-1
volatile uint8_t SeqNum = 0;

ISR(SPI_STC_vect)
{
	static uint8_t value = 0;
	static uint8_t output = 0;

	if(SeqNum == 0) {				// at the beginning of the sequence
		TPIC6C596_Set_SS();			// write register content to output buffers
		asm volatile ("nop");		// required by synchronizer
		TPIC6C596_Reset_SS();		// allowing input of new data
	}
	value = LEDDIGITS[MAX_INDEX-SeqNum];						// Displayed value
	if((value & 0x7F) < NUM_OF_DIGITS) output = DIGITS[value & 0x7F];	// Read symbol code
	if(value & DP) output |= DP_BIT_MASK;								// DP control
	SPDR = output;												// Sending output data to shift regs
	
	SeqNum=(SeqNum+1)%LEDDISPNO;	// incrementing sequence number
}

void TPIC6C596Init()
{
	TPIC6C596_Set_SS();							// Disable data transmission
	DDRB |= (_BV(SHIFT_REG_DI) | _BV(SHIFT_REG_RCK) | _BV(SHIFT_REG_SCK));	// Pins MOSI, SS, SCK set as output
	SPCR = _BV(SPIE) | _BV(SPE) | _BV(MSTR);	// Interrupt Enabled, SPI Enabled, Master mode
	SPCR |= _BV(SPR1) | _BV(SPR0);				// Prescaler CLK/128
	SPSR;										// Reading the SPI status register...
	SPDR;										// And than accessing the SPI data register to clear SPI Interrupt flag
}

void TPIC6C596Suspend()
{
	SPCR &= ~_BV(SPIE);		// Interrupt Disabled
}

void TPIC6C596Resume()
{
	SPCR |= _BV(SPIE);		// Interrupt Enabled
	SPSR;					// Reading the SPI status register...
	SPDR;					// And than accessing the SPI data register to clear SPI Interrupt flag
	SPDR = 0;				// Initialize SPI interrupts by writing to SPI data register
}