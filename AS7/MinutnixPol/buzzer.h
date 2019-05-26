/*
 * buzzer.h
 *
 * Created: 3/14/2019 9:33:34 PM
 *  Author: stefa
 */ 


#ifndef BUZZER_H_
#define BUZZER_H_

void buzzerInit();
void buzzerOn();
void buzzerOff();
void buzzerSetVolume(uint8_t setting);

#endif /* BUZZER_H_ */