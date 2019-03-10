/*
 * uart.c
 *
 * Created: 3/10/2019 12:17:10 PM
 *  Author: stefan
 */ 

#include "uart.h"
#include <avr/pgmspace.h>


void USART_putchar(char ch)
{
	while(!(UCSR0A & _BV(UDRE0)));  // Zaczekaj na miejsce w buforze nadawczym
	UDR0=ch;
}

void USART_send(const char __memx *txt)
{
	while(*txt)
	{
		USART_putchar(*txt);
		++txt;
	}
}

void USART_send_block(const uint8_t __memx *block, uint8_t size)
{
	while(size--)
	{
		USART_putchar(*block);
		++block;
	}
}

static void uart_9600()
{
	#define BAUD 9600
	#include <util/setbaud.h>
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	#if USE_2X
	UCSR0A |= (1 << U2X0);
	#else
	UCSR0A &= ~(1 << U2X0);
	#endif
}

void USART_init()
{
	uart_9600();						// Ustal szybkosc transferu na 9600 bps
	UCSR0B=_BV(TXEN0);					// Odblokuj nadajnik
	UCSR0C=_BV(UCSZ00) | _BV(UCSZ01);	// 8 bitów danych + 1 bit stopu
}


int get(FILE *stream)
{
	while (!(UCSR0A & _BV(RXC0)));	// Zaczekaj na odbiór znaku
	return UDR0;
}

int put(char c, FILE *stream)
{
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = c;
	return 0;
}