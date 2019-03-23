/*
 * display.h
 *
 * Created: 3/10/2019 12:25:21 PM
 *  Author: stefan
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>

#define LEDDISPNO	2			// Number of displays
#define MAX_BRIGHTNESS_VAL 100
#define BLANK_DISPLAY 16

//Cyfry 0,1,2,3,4,5,6,7,8,9 i symbol -
// static const uint8_t __flash DIGITS[17]={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7F, 0x39, 0x3F, 0x79, 0x71, 0x00}; // extended to F
static const uint8_t __flash DIGITS[17]={0x7B, 0x0A, 0xB3, 0x9B, 0xCA, 0xD9, 0xF9, 0x0B, 0xFB, 0xDB, 0xEB, 0xFB, 0x71, 0x7B, 0xF1, 0xE1, 0x00}; // mapping for brake at pin 2
static const uint8_t __flash DP = 0x80;

volatile uint8_t LEDDIGITS[LEDDISPNO];	// Table with data to display

void displayInit();
void displaySetBrightness(uint8_t percentage);

#endif /* DISPLAY_H_ */