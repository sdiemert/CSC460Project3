/*
 * game.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Scott and Simon
 */


#ifndef GAME_H
#define GAME_H

#include "avr/io.h"

typedef enum _roomba_nums {ZOMBIE=0, HUMAN=1} TEAM; 

typedef enum _zombie_states {NORMAL=0, STUNNED} ZOMBIE_STATES;
typedef enum _human_states {SHIELDED=0, SHIELDLESS} HUMAN_STATES;

extern uint8_t ROOMBA_ADDRESSES[][5];

extern uint8_t ROOMBA_FREQUENCIES[];

#endif /* GAME_H */
