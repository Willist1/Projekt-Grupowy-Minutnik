/*
 * PCF8574.c
 *
 * Created: 3/10/2019 2:56:13 PM
 *  Author: stefan
 */ 

#include <stdint.h>
#include <avr/io.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include "PCF8574.h"
#include "i2c.h"
#include "display.h"

ISR(INT0_vect)
{
	PCF8574_INT = true;
}

void PCF8574_Init() {
	I2C_Init();
	I2C_StartSelectWait(PCF8574_WRITE);
	I2C_SendByte(0xFF);				// I/Os should be high before being used as inputs
	I2C_Stop();
	PCF8574_PinState = 0xFF;		// pin state map set to 0xFF by default
	PCF8574_PrevPinState = 0xFF;
}

void PCF8574_INTInit() {
	
	EICRA |= _BV(ISC01);		// the falling edge of INT0 generates an interrupt request
	EIMSK |= _BV(INT0);			// INT0 external interrupt request enable
	PCF8574_INT = false;		// interrupt flag set to false by default
}

void PCF8574_ReadState() {
	I2C_StartSelectWait(PCF8574_READ);
	PCF8574_PrevPinState = PCF8574_PinState;
	PCF8574_PinState = I2C_ReceiveData_ACK();
	I2C_Stop();
}

void PCF8574_WriteState(uint8_t state) {
	I2C_SendStartAndSelect(PCF8574_WRITE);
	I2C_SendByte(state);
	I2C_Stop();
}

BUTTON PCF8574_ReadButtonPress() {
	uint8_t toggled = PCF8574_PrevPinState ^ PCF8574_PinState;	// extract toggled pins
	toggled &= ~PCF8574_PinState;								// extract pressed buttons
	toggled &= ~encoderSigMask;									// ignore encored lines
	for (uint8_t bit = 0; bit < 8; bit++) {
		if (toggled & (1<<bit)) {
			return (BUTTON)bit;
		}
	}
	return BUTTON_NONE;
}