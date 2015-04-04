#ifndef __TRACE_UART_H__
#define __TRACE_UART_H__
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

/** modifications
	- moved the volatile uint8_t uart_rx variable from the h file to the c file
	- renamed to cpp file
	- changed the cli() and sei() functions to Disable_interrupts() and Enable_Interrupts()
	- removed UART_BPS code and hard-coded it to be 9600 baud
*/

#define TRACE_UART_BUFFER_SIZE 100			// size of Rx ring buffer.

void trace_uart_init();
void trace_uart_putchar(char c);
char trace_uart_getchar(int index);
void trace_uart_putstr(char *s);
int  trace_uart_write(uint8_t* const str, int len);

uint8_t trace_uart_bytes_recv(void);
void    trace_uart_reset_recv(void);

#endif
