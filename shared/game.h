/*
 * cops_and_robbers.h
 *
 * Created: 2015-01-26 17:09:37
 *  Author: Daniel
 */


#ifndef GAME_H
#define GAME_H

#include "avr/io.h"

typedef enum _roomba_nums {PLAYER0=0,PLAYER1,PLAYER2,PLAYER3} PLAYERS;
extern uint8_t ROOMBA_ADDRESSES[][5];
extern uint8_t ROOMBA_FREQUENCIES[];

typedef struct _game_t{
    uint8_t sender_address[5];  /// The return address of the station sending this packet

    uint8_t velocity_x;
    uint8_t velocity_y;
    uint8_t button;

    //roomba state
    uint8_t game_player_id; // the id of the roomba
    uint8_t game_team; // 0 for zombie, 1 for human
    uint8_t game_state; // 0 for normal, 1 for stunned
    uint8_t game_hit_flag; // 0 for not hit, 1 for hit
    uint8_t game_enemy_id; // id of the enemy which shot you, only valid when hit_flag is 1
} pf_game_t;


#endif /* _GAME_H_ */
