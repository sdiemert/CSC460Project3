/**
 * \file timer.h
 * \brief Helper functions to create timers out of the OS <code>Now()</code> functionality.
 *
 *	@date 07/13
 *
 *	@author	Gordon Meyer
 *	@author Daniel McIlvaney
 *	@author Cale McNulty
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <avr/io.h>

typedef struct os_timer {
	volatile unsigned int elapsed_time, last_update;
	volatile uint8_t paused;
} OS_TIMER;

unsigned int timer_value (volatile OS_TIMER* timer);
void timer_reset(volatile OS_TIMER* timer);
void timer_pause(volatile OS_TIMER* timer);
void timer_resume(volatile OS_TIMER* timer);

#endif /* TIMER_H_ */