/*
 * uart.h
 *
 * Created: 3/10/2019 12:17:22 PM
 *  Author: stefan
 */ 


#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

void USART_putchar(char ch);                                           //Wyslij znak
void USART_send(const char __memx *txt);                               //Wyslij lañcuch w formacie NULLZ
void USART_send_block(const uint8_t __memx *block, uint8_t size);      //Wyslij blok o dlugosci size z pamieci SRAM

static inline void waitforTx()            //Zaczekaj na koniec nadawania znaku
{
	while(!(UCSR0A & _BV(UDRE0)));
}

void USART_init();

int get(FILE *stream);
int put(char c, FILE *stream);

// #include <stdio.h>
// static FILE usartout = FDEV_SETUP_STREAM (put, get, _FDEV_SETUP_RW);
// stdout = &usartout;
// printf("...");

#endif /* UART_H_ */