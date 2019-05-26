/*
 * main.h
 *
 * Created: 3/23/2019 2:34:19 PM
 *  Author: stefa
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include "PCF8574.h"

#define CNT_MAX_VAL 99
#define CNT_MIN_VAL 1
extern uint8_t WARN_MAX_VAL;
#define WARN_MIN_VAL 0
#define BRIGHT_MAX_VAL 10
#define BRIGHT_MIN_VAL 1
#define VOL_MAX_VAL 3
#define VOL_MIN_VAL 0

#define WARN_BEEP_DURATION_SECONDS 2

typedef enum {
	sSetting = 0,
	sRunning,
	sPause
} tSTATE;

typedef struct {
	uint8_t cntVal;
	uint8_t warnVal;
	uint8_t brightVal;
	uint8_t volumeVal;
} tCONFIG;

typedef struct
{
	tCONFIG config;
	uint16_t totalSeconds;
} tNVData;

volatile tNVData NVData; // Non-volatile config data copy in SRAM
extern volatile uint32_t ticks;

extern BUTTON memorizedButton;
extern uint8_t setVal;

#endif /* MAIN_H_ */