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

typedef struct _game_t{
    uint8_t sender_address[5];  /// The return address of the station sending this packet

    // roomba command
    uint8_t roomba_command;
    uint8_t roomba_num_arg_bytes;
    uint8_t roomba_args[8];

    // if not zero send out
    uint8_t ir_fire;

    //roomba state
    uint8_t game_player_id; // the id of the roomba
    uint8_t game_team; // 0 for zombie, 1 for human
    uint8_t game_state; // 0 for normal, 1 for stunned
    uint8_t game_hit_flag; // 0 for not hit, 1 for hit
    uint8_t game_enemy_id; // id of the enemy which shot you, only valid when hit_flag is 1
} pf_game_t;


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
