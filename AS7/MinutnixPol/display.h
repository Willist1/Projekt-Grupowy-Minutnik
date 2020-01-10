/*
 * display.h
 *
 * Created: 3/10/2019 12:25:21 PM
 *  Author: stefan
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>

#define LEDDISPNO	4			// Number of displays
#define MAX_BRIGHTNESS_VAL 100
#define BLANK_DISPLAY 16
#define NUM_OF_DIGITS 17
#define DP_BIT_MASK 0x01
// #define DP_BIT_MASK 0x80

//Cyfry 0,1,2,3,4,5,6,7,8,9 i symbol -
static const uint8_t __flash DIGITS[NUM_OF_DIGITS]={0xEE, 0x82, 0xDC, 0xD6, 0xB2, 0x76, 0x7E, 0xC2, 0xFE, 0xF6, 0xFA, 0x3E, 0x6C, 0x9E, 0x7C, 0x78, 0x00}; // extended to F
// prototype config
// static const uint8_t __flash DIGITS[NUM_OF_DIGITS]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7F, 0x39, 0x3F, 0x79, 0x71, 0x00}; // extended to F
static const uint8_t __flash DP = 0x80;

volatile uint8_t LEDDIGITS[LEDDISPNO];	// Table with data to display

void Timer1Init();
void displayInit();
void displaySetBrightness(uint8_t percentage);
void displayOff();
void displayOn();

#endif /* DISPLAY_H_ */