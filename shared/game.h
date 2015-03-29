/*
 * cops_and_robbers.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Daniel
 */


#ifndef COPS_AND_ROBBERS_H_
#define COPS_AND_ROBBERS_H_

#include "avr/io.h"

typedef enum _roomba_nums {COP1 = 0, COP2, ROBBER1, ROBBER2} COPS_AND_ROBBERS;
extern uint8_t ROOMBA_ADDRESSES[][5];

extern uint8_t ROOMBA_FREQUENCIES[];

typedef enum _ir_commands{
	SEND_BYTE,
	REQUEST_DATA,
	AIM_SERVO
} IR_COMMANDS;

typedef enum _roomba_statues{
	ROOMBA_ALIVE,
	ROOMBA_DEAD
}ROOMBA_STATUSES;

#endif /* COPS_AND_ROBBERS_H_ */
