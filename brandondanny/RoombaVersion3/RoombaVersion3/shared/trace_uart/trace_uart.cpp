#include <avr/io.h>
#include <avr/interrupt.h>		// ISR handling.

#include "trace_uart.h"

/*
 Global Variables:
 Variables appearing in both ISR/Main are defined as 'volatile'.
*/
static volatile int rxn; // buffer 'element' counter.
static volatile char rx[TRACE_UART_BUFFER_SIZE]; // buffer of 'char'.
static volatile char uart_rx;

void trace_uart_putchar (char c)
{
	asm volatile ("cli"::);
	while ( !( UCSR0A & (1<<UDRE0)) ); // Wait for empty transmit buffer
	UDR0 = c;  // Putting data into the buffer, forces transmission
	asm volatile ("sei"::);
}

char trace_uart_getchar (int index)
{
	if (index < TRACE_UART_BUFFER_SIZE) {
		return rx[index];
	}
	return 0;
}

void trace_uart_putstr(char *s)
{
	while(*s) trace_uart_putchar(*s++);

}

int trace_uart_write(uint8_t* const str, int len){
	trace_uart_putstr((char*)str);
	return 0;
}

void trace_uart_init(){

	//DDRB = 0xff;
	//PORTB = 0xff;

	rxn = 0;
	uart_rx = 0;

	/* Set baud rate, code from the atmega2560 datasheet */
	uint16_t baud_rate = (F_CPU/16/9600-1);
	UBRR0H = (unsigned char) baud_rate>>8;
	UBRR0L = (unsigned char) baud_rate;

	/* Enable receiver and transmitter */
	UCSR0B = _BV(RXEN0)|_BV(TXEN0) | _BV(RXCIE0);

	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (1<<USBS0)|(1<<UCSZ00) | _BV(UCSZ01);
}

uint8_t trace_uart_bytes_recv(void)
{
	return rxn;
}

void trace_uart_reset_recv(void)
{
	rxn = 0;
}

/*
 Interrupt Service Routine (ISR):
*/

ISR(USART0_RX_vect)
{
	while ( !(UCSR0A & (1<<RXC0)) );

	//PORTB ^= _BV(PB7);

	rx[rxn] = UDR0;
	rxn = (rxn + 1) % TRACE_UART_BUFFER_SIZE;
	uart_rx = 1; // notify main of receipt of data.
	//PORTB = PORTB | _BV(PINB1);
}


