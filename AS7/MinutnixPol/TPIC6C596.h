/*
 * TPIC6C596.h
 *
 * Created: 4/20/2019 3:10:09 PM
 *  Author: stefa
 */ 


#ifndef TPIC6C596_H_
#define TPIC6C596_H_

#define SHIFT_REG_PORT PORTB

#define SHIFT_REG_G 2		// (not)OE (output enable) - low unblocks the buffers
#define SHIFT_REG_DI 3		// SER (data in) - connected to uC's MOSI
#define SHIFT_REG_RCK 1		// RCLK (latch) - low activates SCK input / high causes transfer of reg content to output buffers - connected to uC's SS
#define SHIFT_REG_SCK 5		// SRCLK (clock) - connected to uC's SCK
							// SCL - constant high - zeroing reg

extern volatile uint8_t SeqNum;

void TPIC6C596_Set_SS();
void TPIC6C596_Reset_SS();
void TPIC6C596_Reset_OE();
void TPIC6C596_Set_OE();
void TPIC6C596Init();
void TPIC6C596Suspend();
void TPIC6C596Resume();

#endif /* TPIC6C596_H_ */