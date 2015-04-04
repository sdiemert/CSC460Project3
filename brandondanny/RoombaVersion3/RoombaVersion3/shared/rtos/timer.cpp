/**
 * \file timer.c
 * \brief Helper functions to create timers out of the OS <code>Now()</code> functionality.
 *
 *	@date 07/13
 *
 *	@author	Gordon Meyer
 *	@author Daniel McIlvaney
 *	@author Cale McNulty
 */

#include <avr/interrupt.h>
#include "timer.h"
#include "os.h"

/**
Retrieve the elapsed time.
	@param	timer	Address of an OS_TIMER.
	@return			The elapsed time since the last timer reading.( milliseconds)
*/
unsigned int timer_value (volatile OS_TIMER* timer) {

	unsigned int temp;
	uint8_t sreg;
	sreg = SREG;
	cli();
	unsigned int r = 0;

	if(timer->paused) {
		r = timer->elapsed_time;
	} else {
		unsigned int current_time = Now();
		temp = (current_time - timer->last_update);
		r = timer->elapsed_time + temp;
	}

	SREG = sreg;
	return r;
}

/**
Reset a timer to a paused state, use resume to start counting.
	@param timer	The address of the timer to be reset.
*/
void timer_reset(volatile OS_TIMER* timer) {

	uint8_t sreg;
	sreg = SREG;
	cli();

	timer->paused = 1;
	timer->elapsed_time = 0;

	SREG = sreg;
}

/**
Pauses a timer so it stops counting
	@param timer	The address of the timer to be paused.
*/
void timer_pause(volatile OS_TIMER* timer) {

	unsigned int current_time, temp;
	uint8_t sreg;
	sreg = SREG;
	cli();

	if(!(timer->paused)){
		timer->paused = 1;
		current_time = Now();
		temp = (current_time - timer->last_update);
		timer->elapsed_time += temp;
	}

	SREG = sreg;
}

/**
Resumes a timer so it continues counting from where it left off, skipping any
	time it was paused.
	@param timer	The address of the timer to be resumed.
*/
void timer_resume(volatile OS_TIMER* timer) {

	uint8_t sreg;
	sreg = SREG;
	cli();

	if(timer->paused){
		timer->paused = 0;
		timer->last_update = Now();
	}

	SREG = sreg;
}