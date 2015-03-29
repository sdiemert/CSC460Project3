/**
 * @file    trace.c
 *
 * @brief   Testing module for creating traces.
 *          Used for testing an RTOS
 *
 *          NOTE: Traces are stored in string like this
 *
 *          T001;1;2;3;1235
 *
 * CSC 460/560 Real Time Operating Systems - Mantis Cheng
 * @author Scott Craig
 * @author Justin Tanner
 */

#include "trace.h"
#include "../trace_uart/trace_uart.h"

/** boolean value to check whether we have printed the trace or not */
uint8_t trace_number = 0;

/** boolean value to check whether we have printed the trace or not */
uint8_t trace_printed = 0;

/** array that holds all the elements of the trace */
uint16_t trace_array[TRACE_ARRAY_SIZE];

/** current element in the trace_array */
uint16_t volatile trace_counter = 0;

/** big buffer to hold all the trace */
char trace_buffer[TRACE_BUFFER_SIZE];

/** print event setup in the main test */
//extern EVENT* print_event;

/**
 * Dump the entire trace array on UART
 *
 */
void print_trace()
{
    int i;
    int returned_chars;

	if (!trace_printed)
	{
        /* the test header */
        returned_chars = sprintf((char*)&trace_buffer, "T%03d;", trace_number);
        trace_uart_write((uint8_t*)trace_buffer, returned_chars);

        /* all traces as 5 digit padded decimals
        for(i = 0; i < TRACE_ARRAY_SIZE; i++)*/
		for(i = 0; i < (int)trace_counter; i++)
        {
            returned_chars = sprintf((char*)&trace_buffer, "%u;", trace_array[i]);
            trace_uart_write((uint8_t*)trace_buffer, returned_chars);
            //_delay_ms(1);
        }

        trace_printed = 1;
        trace_counter = 0;
    }
}

/**
 * Add a number to the trace array and increment the trace counter
 *
 * @param number unisigned integer to add
 */
void add_to_trace(uint16_t number)
{
    if (trace_counter < TRACE_ARRAY_SIZE)
    {
        trace_array[trace_counter] = number;
        trace_counter++;
    }
}

/**
 * Set the test number of this set, that will be displayed in the trace header
 *
 * @param number
 */
void set_trace_test(uint8_t number)
{
    trace_number = number;
}


int is_trace_full(){
    return (trace_counter == TRACE_ARRAY_SIZE);
}

void reset_trace(){
    trace_counter = 0;
    trace_printed = 0;
}