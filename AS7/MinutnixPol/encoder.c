/*
 * encoder.c
 *
 * Created: 3/13/2019 9:00:40 PM
 *  Author: stefan
 */ 

#include <stdint.h>
#include "PCF8574.h"

int8_t enc_delta;

#define SigAPin 5
#define SigBPin 4
#define SigAMask (uint8_t)(1 << SigAPin)
#define SigBMask (uint8_t)(1 << SigBPin)

void ReadEncoder()
{
	static int8_t last;
	int8_t newpos, diff;

	newpos=0;
	if((PCF8574_PinState & SigAMask)==0) newpos=3;
	if((PCF8574_PinState & SigBMask)==0) newpos^=1;  // konwersja kodu Graya na binarny
	diff=last-newpos;
	if(diff & 1)
	{									// bit 0 = krok
		last=newpos;
		enc_delta+=(diff & 2)-1;		//bit 1 - kierunek
	}
}

int8_t Read1StepEncoder()
{
	ReadEncoder();
	int8_t val=enc_delta;
	enc_delta=0;
	return val;
}

int8_t Read2StepEncoder()
{
	ReadEncoder();
	int8_t val=enc_delta;
	enc_delta=val & 1;
	return val>>1;
}

int8_t Read4StepEncoder()
{
	ReadEncoder();
	int8_t val=enc_delta;
	enc_delta=val & 3;
	return val>>2;
}
