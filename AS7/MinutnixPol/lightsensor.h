/*
 * lightsensor.h
 *
 * Created: 4/27/2019 8:48:38 PM
 *  Author: stefa
 */ 


#ifndef LIGHTSENSOR_H_
#define LIGHTSENSOR_H_

#include <stdint.h>

void lightsensorInit();
uint8_t lightsensorRead();
void lightsensorSuspend();
void lightsensorResume();

#endif /* LIGHTSENSOR_H_ */