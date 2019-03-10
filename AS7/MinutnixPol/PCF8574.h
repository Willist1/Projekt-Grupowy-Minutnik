/*
 * PCF8574.h
 *
 * Created: 3/10/2019 1:28:11 PM
 *  Author: stefa
 */ 


#ifndef PCF8574_H_
#define PCF8574_H_

#include "i2c.h"
#include <stdbool.h>

#define PCF8574ADDR 0x40
#define PCF8574_WRITE PCF8574ADDR
#define PCF8574_READ PCF8574ADDR | 0x01

volatile bool PCF8574_INT;

void PCF8574_Init();
void PCF8574_INTInit();
uint8_t PCF8574_ReadState();
void PCF8574_WriteState(uint8_t state);

#endif /* PCF8574_H_ */