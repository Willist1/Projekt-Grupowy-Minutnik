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

#define PCF8574ADDR 0x40		// PCF8574N
//#define PCF8574ADDR 0x70		// PCF8574AT / PCF8574AP
#define PCF8574_WRITE PCF8574ADDR
#define PCF8574_READ PCF8574ADDR | 0x01

#define SigAPin 5
#define SigBPin 6
#define SigAMask (uint8_t)(1 << SigAPin)
#define SigBMask (uint8_t)(1 << SigBPin)
#define encoderSigMask (uint8_t)(SigAMask | SigBMask)

/*
// prototype config
typedef enum {
	setCnt = 0,
	setWarning,
	setVolume,
	Play,
	Stop,
	setBrightness = 7,	// to be removed
	BUTTON_CNT,
	BUTTON_NONE
} BUTTON;
*/
typedef enum {
	Stop = 0,
	Play,
	setVolume,
	setWarning,
	setCnt,
	BUTTON_CNT,
	BUTTON_NONE
} BUTTON;

volatile bool PCF8574_INT;
volatile uint8_t PCF8574_PinState;
volatile uint8_t PCF8574_PrevPinState;

void PCF8574_Init();
void PCF8574_ReadState();
void PCF8574_WriteState(uint8_t state);
BUTTON PCF8574_ReadButtonPress();

#endif /* PCF8574_H_ */