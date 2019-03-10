/*
 * PCF8574.c
 *
 * Created: 3/10/2019 2:56:13 PM
 *  Author: stefan
 */ 

#include "PCF8574.h"
#include "i2c.h"
#include <stdint.h>
#include <avr/io.h>
#include <stdbool.h>
#include "display.h"

void PCF8574_Init() {
	I2C_Init();
	I2C_StartSelectWait(PCF8574_WRITE);
	I2C_SendByte(0xFF); // I/Os should be high before being used as inputs
	I2C_Stop();
}

void PCF8574_INTInit() {
	
	EICRA |= _BV(ISC01);		// the falling edge of INT0 generates an interrupt request
	EIMSK |= _BV(INT0);			// INT0 external interrupt request enable
}

uint8_t PCF8574_ReadState() {
	uint8_t state;
	I2C_StartSelectWait(PCF8574_READ);
	state = I2C_ReceiveData_ACK();
	I2C_Stop();
	return state;
}

void PCF8574_WriteState(uint8_t state) {
	I2C_SendStartAndSelect(PCF8574_WRITE);
	I2C_SendByte(state);
	I2C_Stop();
}